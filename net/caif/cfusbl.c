/*
 * CAIF USB handler
 * Copyright (C) ST-Ericsson AB 2011
 * Author:	Sjur Brendeland/sjur.brandeland@stericsson.com
 * License terms: GNU General Public License (GPL) version 2
 *
 */

#define pr_fmt(fmt) KBUILD_MODNAME ":%s(): " fmt, __func__

#include <linux/netdevice.h>
#include <linux/slab.h>
#include <linux/netdevice.h>
#include <linux/mii.h>
#include <linux/usb.h>
#include <linux/usb/usbnet.h>
#include <net/netns/generic.h>
#include <net/caif/caif_dev.h>
#include <net/caif/caif_layer.h>
#include <net/caif/cfpkt.h>
#include <net/caif/cfcnfg.h>

MODULE_LICENSE("GPL");

#define CAIF_USB 0x88b5		/* Protocol used for CAIF-USB */
#define USB_HEADLEN 19		/* Overhead of ethernet header. */
#define CFNCM_HEAD_SZ 14	/* Overhead of ethernet header. */
#define CFNCM_HEADPAD_SZ 1	/* Number of bytes to align. */
#define CFNCM_HEADPAD 16	/* Number of bytes to align. TODO: sysfs? */

struct cfusbl {
	struct cflayer layer;
	u8 tx_eth_hdr[14];
};

static bool pack_added;

static int cfusbl_receive(struct cflayer *layr, struct cfpkt *pkt)
{
	u8 hpad;

	/* Remove padding. */
	cfpkt_extr_head(pkt, &hpad, 1);
	cfpkt_extr_head(pkt, NULL, hpad);
	return layr->up->receive(layr->up, pkt);
}

static int cfusbl_transmit(struct cflayer *layr, struct cfpkt *pkt)
{
	struct caif_payload_info *info;
	u8 hpad;
	u8 zeros[CFNCM_HEADPAD];
	struct sk_buff *skb;
	struct cfusbl *usbl = container_of(layr, struct cfusbl, layer);

	skb = cfpkt_tonative(pkt);

	/* Without this the network stack complains about buggy protocol. */
	skb_reset_network_header(skb);
	skb->protocol = htons(ETH_P_IP);

	info = cfpkt_info(pkt);
	hpad = (info->hdr_len + CFNCM_HEADPAD_SZ) & (CFNCM_HEADPAD - 1);

	if (skb_headroom(skb) < CFNCM_HEAD_SZ + CFNCM_HEADPAD_SZ + hpad) {
		pr_warn("Headroom to small\n");
		kfree_skb(skb);
		return -EIO;
	}
	memset(zeros, 0, hpad);

	cfpkt_add_head(pkt, zeros, hpad);
	cfpkt_add_head(pkt, &hpad, 1);
	cfpkt_add_head(pkt, usbl->tx_eth_hdr, sizeof(usbl->tx_eth_hdr));
	return layr->dn->transmit(layr->dn, pkt);
}

static void cfusbl_ctrlcmd(struct cflayer *layr, enum caif_ctrlcmd ctrl,
					int phyid)
{
	if (layr->up && layr->up->ctrlcmd)
		layr->up->ctrlcmd(layr->up, ctrl, layr->id);
}

struct cflayer *cfusbl_create(int phyid, u8 ethaddr[6])
{
	struct cfusbl *this = kmalloc(sizeof(struct cfusbl), GFP_ATOMIC);

	if (!this) {
		pr_warn("Out of memory\n");
		return NULL;
	}
	caif_assert(offsetof(struct cfusbl, layer) == 0);

	memset(this, 0, sizeof(struct cflayer));
	this->layer.receive = cfusbl_receive;
	this->layer.transmit = cfusbl_transmit;
	this->layer.ctrlcmd = cfusbl_ctrlcmd;
	snprintf(this->layer.name, CAIF_LAYER_NAME_SZ, "usb%d", phyid);
	this->layer.id = phyid;

	/*
	 * Construct TX ethernet header:
	 *	0-5	destination address (based on source address)
	 *	5-11	source address
	 *	12-13	protocol type
	 */
	memcpy(this->tx_eth_hdr, ethaddr, 6);
	this->tx_eth_hdr[4] += 1;
	memcpy(&this->tx_eth_hdr[6], ethaddr, 6);
	this->tx_eth_hdr[12] = (CAIF_USB >> 8) & 0xff;
	this->tx_eth_hdr[13] = CAIF_USB & 0xff;
	pr_debug("caif ethernet TX-header dst:%pM src:%pM type:%02x%02x\n",
			this->tx_eth_hdr,this->tx_eth_hdr + 6,
			this->tx_eth_hdr[12],this->tx_eth_hdr[13]);

	return (struct cflayer *) this;
}

static struct packet_type caif_usb_type __read_mostly = {
	.type = cpu_to_be16(CAIF_USB),
};

static int cfusbl_device_notify(struct notifier_block *me, unsigned long what,
			      void *arg)
{
	struct net_device *dev = arg;
	struct caif_dev_common common;
	struct cflayer *layer, *link_support;
	struct usbnet	*usbnet = netdev_priv(dev);
	struct usb_device	*usbdev = usbnet->udev;
	struct ethtool_drvinfo drvinfo;

	if (what != NETDEV_REGISTER)
		return 0;

	if (dev->ethtool_ops == NULL || dev->ethtool_ops->get_drvinfo == NULL)
		return 0;

	dev->ethtool_ops->get_drvinfo(dev, &drvinfo);
	if (strncmp(drvinfo.driver, "cdc_ncm", 7) != 0)
		return 0;

	pr_debug("USB NCM Net device (Device number:%d): 0x%4.4x:0x%4.4x:0x%4.4x",
		usbdev->devnum,
		le16_to_cpu(usbdev->descriptor.idVendor),
		le16_to_cpu(usbdev->descriptor.idProduct),
		le16_to_cpu(usbdev->descriptor.bcdDevice));

	/* Check for STE Bridge build */
	if (!(usbdev->descriptor.idVendor == 0x04cc &&
			(usbdev->descriptor.idProduct == 0x2306 ||
	/* FIXME: 0x2306 is PC-card and used only temporary! */
					usbdev->descriptor.idProduct == 0x230f)))
		return 0;

	memset(&common, 0, sizeof(common));
	common.use_frag = false;
	common.use_fcs = false;
	common.use_stx = false;
	common.link_select = CAIF_LINK_HIGH_BANDW;
	common.flowctrl = NULL;

	link_support = cfusbl_create(dev->ifindex, dev->dev_addr);
	if (!link_support) {
		pr_warn("Out of memory\n");
		return -ENOMEM;
	}

	if (dev->num_tx_queues > 1)
		pr_warn("USB device uses more than one tx queue\n");

	caif_enroll_dev(dev, &common, link_support, USB_HEADLEN,
			&layer, &caif_usb_type.func);
	if (!pack_added)
		dev_add_pack(&caif_usb_type);
	pack_added = 1;

	strncpy(layer->name, dev->name,
			sizeof(layer->name) - 1);
	layer->name[sizeof(layer->name) - 1] = 0;

	return 0;
}

static struct notifier_block caif_device_notifier = {
	.notifier_call = cfusbl_device_notify,
	.priority = 0,
};

void cfusbl_init(void)
{
	register_netdevice_notifier(&caif_device_notifier);
}

void cfusbl_exit(void)
{
	unregister_netdevice_notifier(&caif_device_notifier);
	dev_remove_pack(&caif_usb_type);
}
