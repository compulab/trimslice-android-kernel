/*
 * drivers/video/tegra/dc/edid.c
 *
 * Copyright (C) 2010 Google, Inc.
 * Author: Erik Gilling <konkers@android.com>
 *
 * Copyright (C) 2010-2011 NVIDIA Corporation
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */


#include <linux/debugfs.h>
#include <linux/fb.h>
#include <linux/i2c.h>
#include <linux/seq_file.h>
#include <linux/vmalloc.h>

#include "edid.h"

struct tegra_edid_pvt {
	struct kref			refcnt;
	struct tegra_edid_hdmi_eld	eld;
	bool				support_stereo;
	bool				support_underscan;
	/* Note: dc_edid must remain the last member */
	struct tegra_dc_edid		dc_edid;
};

struct tegra_edid {
	struct i2c_client	*client;
	struct i2c_board_info	info;
	int			bus;

	struct tegra_edid_pvt	*data;

	struct mutex		lock;
	int filter;
	int status;
};

struct established_timing_bitmap {
	unsigned int xres;
	unsigned int yres;
	unsigned int refresh;
	unsigned int byte;
	unsigned int bit;
};

static struct established_timing_bitmap* tegra_edid_etb_find(struct fb_videomode *mode);

#define EDID_ETB_OFFSET 35
#define EDID_STI_OFFSET 38

#if defined(DEBUG) || defined(CONFIG_DEBUG_FS)
static int tegra_edid_show(struct seq_file *s, void *unused, int type)
{
	struct tegra_edid *edid = s->private;
	struct tegra_dc_edid *data;
	u8 *buf;
	int i;

	data = tegra_edid_get_data(edid);
	if (!data) {
		seq_printf(s, "No EDID\n");
		return 0;
	}
	buf = data->buf;
	switch (type) {
	case 0: /* ASCII Dump */
		for (i = 0; i < data->len; i++) {
			if (i % 16 == 0)
				seq_printf(s, "edid[%03x] =", i);

			seq_printf(s, " %02x", buf[i]);

			if (i % 16 == 15)
				seq_printf(s, "\n");
		}
		break;
	case 1: /* Hex Dump */
		seq_write(s ,data->buf, data->len);
		break;
	default:
		break;
	}
	tegra_edid_put_data(data);

	return 0;
}

static int tegra_edid_show_ascii(struct seq_file *s, void *unused){
	return tegra_edid_show(s,unused,0);
}

static int tegra_edid_show_hex(struct seq_file *s, void *unused){
	return tegra_edid_show(s,unused,1);
}

static int tegra_edid_show_filter(struct seq_file *s, void *unused){
	struct tegra_edid *edid = s->private;
	seq_printf(s,"%x\n",edid->filter);
	return 0;
}

static int tegra_edid_show_status(struct seq_file *s, void *unused){
	struct tegra_edid *edid = s->private;
	seq_printf(s,"%x\n",edid->status);
	return 0;
}
#endif

#ifdef CONFIG_DEBUG_FS
static int tegra_edid_debug_open_ascii(struct inode *inode, struct file *file)
{
	return single_open(file, tegra_edid_show_ascii, inode->i_private);
}

static int tegra_edid_debug_open_hex(struct inode *inode, struct file *file)
{
	return single_open(file, tegra_edid_show_hex, inode->i_private);
}

static int tegra_edid_debug_open_filter(struct inode *inode, struct file *file)
{
	return single_open(file, tegra_edid_show_filter, inode->i_private);
}

static int tegra_edid_debug_open_status(struct inode *inode, struct file *file)
{
	return single_open(file, tegra_edid_show_status, inode->i_private);
}

static int tegra_edid_write(struct file *file, const char __user *u, size_t s, loff_t *o){
	struct tegra_edid *edid = ((struct seq_file *)file->private_data)->private;

	unsigned char *buffer = kmalloc(s, GFP_KERNEL);
	size_t len = 0;
	struct tegra_dc_edid *data;

	if (buffer==NULL) { /* An allocation error, give up, nothing to do */
		goto done;
	}
	memcpy(buffer,u,s);

	data = tegra_edid_get_data(edid);
	if (!data)
		goto done;

	len = ((data->len - *o) < s) ? (data->len - *o) : s;
	memcpy((data->buf + *o),buffer,len);
	tegra_edid_put_data(data);

done:
	if (buffer)
		kfree(buffer);
	*o += len;
	return len;
}

static int tegra_edid_write_filter(struct file *file, const char __user *u, size_t s, loff_t *o){
	struct tegra_edid *edid = ((struct seq_file *)file->private_data)->private;
	char **last = NULL;

	edid->filter = simple_strtoul(u, last, 0);
	*o += s;
	return s;
}

static int tegra_edid_write_status(struct file *file, const char __user *u, size_t s, loff_t *o){
	struct tegra_edid *edid = ((struct seq_file *)file->private_data)->private;
	char **last = NULL;

	edid->status = simple_strtoul(u, last, 0);
	*o += s;
	return s;
}

static const struct file_operations tegra_edid_debug_fops_ascii = {
	.open		= tegra_edid_debug_open_ascii,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static const struct file_operations tegra_edid_debug_fops_hex = {
	.open		= tegra_edid_debug_open_hex,
	.read		= seq_read,
	.write		= tegra_edid_write,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static const struct file_operations tegra_edid_debug_fops_filter = {
	.open		= tegra_edid_debug_open_filter,
	.read		= seq_read,
	.write		= tegra_edid_write_filter,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static const struct file_operations tegra_edid_debug_fops_status = {
	.open		= tegra_edid_debug_open_status,
	.read		= seq_read,
	.write		= tegra_edid_write_status,
	.llseek		= seq_lseek,
	.release	= single_release,
};

void tegra_edid_debug_add(struct tegra_edid *edid)
{
	char name[] = "edidX";

	snprintf(name, sizeof(name), "edid%1d", edid->bus);
	debugfs_create_file(name, S_IRUGO, NULL, edid, &tegra_edid_debug_fops_ascii);

}

void tegra_edid_debug_add_hex(struct tegra_edid *edid)
{
	char name[] = "edidX.hex";

	snprintf(name, sizeof(name), "edid%1d.hex", edid->bus);
	debugfs_create_file(name, (S_IRUGO|S_IWUSR), NULL, edid, &tegra_edid_debug_fops_hex);
}

void tegra_edid_debug_add_filter(struct tegra_edid *edid)
{
	char name[] = "edidX.filter";

	snprintf(name, sizeof(name), "edid%1d.filter", edid->bus);
	debugfs_create_file(name, (S_IRUGO|S_IWUSR), NULL, edid, &tegra_edid_debug_fops_filter);
}

void tegra_edid_debug_add_status(struct tegra_edid *edid)
{
	char name[] = "edidX.status";

	snprintf(name, sizeof(name), "edid%1d.status", edid->bus);
	debugfs_create_file(name, (S_IRUGO|S_IWUSR), NULL, edid, &tegra_edid_debug_fops_status);
}

#else
void tegra_edid_debug_add(struct tegra_edid *edid)
{
}
#endif

#ifdef DEBUG
static char tegra_edid_dump_buff[16 * 1024];

static void tegra_edid_dump(struct tegra_edid *edid)
{
	struct seq_file s;
	int i;
	char c;

	memset(&s, 0x0, sizeof(s));

	s.buf = tegra_edid_dump_buff;
	s.size = sizeof(tegra_edid_dump_buff);
	s.private = edid;

	tegra_edid_show(&s, NULL, 0);

	i = 0;
	while (i < s.count ) {
		if ((s.count - i) > 256) {
			c = s.buf[i + 256];
			s.buf[i + 256] = 0;
			printk("%s", s.buf + i);
			s.buf[i + 256] = c;
		} else {
			printk("%s", s.buf + i);
		}
		i += 256;
	}
}
#else
static void tegra_edid_dump(struct tegra_edid *edid)
{
}
#endif

int tegra_edid_read_block(struct tegra_edid *edid, int block, u8 *data)
{
	u8 block_buf[] = {block >> 1};
	u8 cmd_buf[] = {(block & 0x1) * 128};
	int status;
	struct i2c_msg msg[] = {
		{
			.addr = 0x30,
			.flags = 0,
			.len = 1,
			.buf = block_buf,
		},
		{
			.addr = 0x50,
			.flags = 0,
			.len = 1,
			.buf = cmd_buf,
		},
		{
			.addr = 0x50,
			.flags = I2C_M_RD,
			.len = 128,
			.buf = data,
		}};
	struct i2c_msg *m;
	int msg_len;

	if (block > 1) {
		msg_len = 3;
		m = msg;
	} else {
		msg_len = 2;
		m = &msg[1];
	}

	status = i2c_transfer(edid->client->adapter, m, msg_len);

	if (status < 0)
		return status;

	if (status != msg_len)
		return -EIO;

	return 0;
}

int tegra_edid_parse_ext_block(const u8 *raw, int idx,
			       struct tegra_edid_pvt *edid)
{
	const u8 *ptr;
	u8 tmp;
	u8 code;
	int len;
	int i;
	bool basic_audio = false;

	ptr = &raw[0];

	/* If CEA 861 block get info for eld struct */
	if (edid && ptr) {
		if (*ptr <= 3)
			edid->eld.eld_ver = 0x02;
		edid->eld.cea_edid_ver = ptr[1];

		/* check for basic audio support in CEA 861 block */
		if(raw[3] & (1<<6)) {
			/* For basic audio, set spk_alloc to Left+Right.
			 * If there is a Speaker Alloc block this will
			 * get over written with that value */
			basic_audio = true;
		}
	}

	if (raw[3] & 0x80)
		edid->support_underscan = 1;
	else
		edid->support_underscan = 0;

	ptr = &raw[4];

	while (ptr < &raw[idx]) {
		tmp = *ptr;
		len = tmp & 0x1f;

		/* HDMI Specification v1.4a, section 8.3.2:
		 * see Table 8-16 for HDMI VSDB format.
		 * data blocks have tags in top 3 bits:
		 * tag code 2: video data block
		 * tag code 3: vendor specific data block
		 */
		code = (tmp >> 5) & 0x7;
		switch (code) {
		case 1:
		{
			edid->eld.sad_count = len;
			edid->eld.conn_type = 0x00;
			edid->eld.support_hdcp = 0x00;
			for (i = 0; (i < len) && (i < ELD_MAX_SAD); i ++)
				edid->eld.sad[i] = ptr[i + 1];
			len++;
			ptr += len; /* adding the header */
			/* Got an audio data block so enable audio */
			if(basic_audio == true)
				edid->eld.spk_alloc = 1;
			break;
		}
		/* case 2 is commented out for now */
		case 3:
		{
			int j = 0;

			if ((ptr[1] == 0x03) &&
				(ptr[2] == 0x0c) &&
				(ptr[3] == 0)) {
				edid->eld.port_id[0] = ptr[4];
				edid->eld.port_id[1] = ptr[5];
			}
			if ((len >= 8) &&
				(ptr[1] == 0x03) &&
				(ptr[2] == 0x0c) &&
				(ptr[3] == 0)) {
				j = 8;
				tmp = ptr[j++];
				/* HDMI_Video_present? */
				if (tmp & 0x20) {
					/* Latency_Fields_present? */
					if (tmp & 0x80)
						j += 2;
					/* I_Latency_Fields_present? */
					if (tmp & 0x40)
						j += 2;
					/* 3D_present? */
					if (j <= len && (ptr[j] & 0x80))
						edid->support_stereo = 1;
				}
			}
			if ((len > 5) &&
				(ptr[1] == 0x03) &&
				(ptr[2] == 0x0c) &&
				(ptr[3] == 0)) {

				edid->eld.support_ai = (ptr[6] & 0x80);
			}

			if ((len > 9) &&
				(ptr[1] == 0x03) &&
				(ptr[2] == 0x0c) &&
				(ptr[3] == 0)) {

				edid->eld.aud_synch_delay = ptr[10];
			}
			len++;
			ptr += len; /* adding the header */
			break;
		}
		case 4:
		{
			edid->eld.spk_alloc = ptr[1];
			len++;
			ptr += len; /* adding the header */
			break;
		}
		default:
			len++; /* len does not include header */
			ptr += len;
			break;
		}
	}

	return 0;
}

int tegra_edid_mode_support_stereo(struct fb_videomode *mode)
{
	if (!mode)
		return 0;

	if (mode->xres == 1280 &&
		mode->yres == 720 &&
		((mode->refresh == 60) || (mode->refresh == 50)))
		return 1;

	/* Disabling 1080p stereo mode due to bug 869099. */
	/* Must re-enable this to 1 once it is fixed. */
	if (mode->xres == 1920 && mode->yres == 1080 && mode->refresh == 24)
		return 0;

	return 0;
}

static void data_release(struct kref *ref)
{
	struct tegra_edid_pvt *data =
		container_of(ref, struct tegra_edid_pvt, refcnt);
	vfree(data);
}

int tegra_edid_get_monspecs_test(struct tegra_edid *edid,
			struct fb_monspecs *specs, unsigned char *edid_ptr)
{
	int i, j, ret;
	int extension_blocks;
	struct tegra_edid_pvt *new_data, *old_data;
	u8 *data;

	new_data = vmalloc(SZ_32K + sizeof(struct tegra_edid_pvt));
	if (!new_data)
		return -ENOMEM;

	kref_init(&new_data->refcnt);

	new_data->support_stereo = 0;
	new_data->support_underscan = 0;

	data = new_data->dc_edid.buf;
	memcpy(data, edid_ptr, 128);

	memset(specs, 0x0, sizeof(struct fb_monspecs));
	memset(&new_data->eld, 0x0, sizeof(new_data->eld));
	fb_edid_to_monspecs(data, specs);
	if (specs->modedb == NULL) {
		ret = -EINVAL;
		goto fail;
	}

	memcpy(new_data->eld.monitor_name, specs->monitor,
					sizeof(specs->monitor));

	new_data->eld.mnl = strlen(new_data->eld.monitor_name) + 1;
	new_data->eld.product_id[0] = data[0x8];
	new_data->eld.product_id[1] = data[0x9];
	new_data->eld.manufacture_id[0] = data[0xA];
	new_data->eld.manufacture_id[1] = data[0xB];

	extension_blocks = data[0x7e];
	for (i = 1; i <= extension_blocks; i++) {
		memcpy(data+128, edid_ptr+128, 128);

		if (data[i * 128] == 0x2) {
			fb_edid_add_monspecs(data + i * 128, specs);

			tegra_edid_parse_ext_block(data + i * 128,
					data[i * 128 + 2], new_data);

			if (new_data->support_stereo) {
				for (j = 0; j < specs->modedb_len; j++) {
					if (tegra_edid_mode_support_stereo(
						&specs->modedb[j]))
						specs->modedb[j].vmode |=
#ifndef CONFIG_TEGRA_HDMI_74MHZ_LIMIT
						FB_VMODE_STEREO_FRAME_PACK;
#else
						FB_VMODE_STEREO_LEFT_RIGHT;
#endif
				}
			}
		}
	}

	new_data->dc_edid.len = i * 128;

	mutex_lock(&edid->lock);
	old_data = edid->data;
	edid->data = new_data;
	mutex_unlock(&edid->lock);

	if (old_data)
		kref_put(&old_data->refcnt, data_release);

	tegra_edid_dump(edid);
	return 0;
fail:
	vfree(new_data);
	return ret;
}

int tegra_edid_get_monspecs(struct tegra_edid *edid, struct fb_monspecs *specs)
{
	int i;
	int j;
	int ret;
	int extension_blocks;
	struct tegra_edid_pvt *new_data, *old_data;
	u8 *data;

	new_data = vmalloc(SZ_32K + sizeof(struct tegra_edid_pvt));
	if (!new_data)
		return -ENOMEM;

	kref_init(&new_data->refcnt);

	new_data->support_stereo = 0;

	data = new_data->dc_edid.buf;

	ret = tegra_edid_read_block(edid, 0, data);
	if (ret)
		goto fail;

	memset(specs, 0x0, sizeof(struct fb_monspecs));
	memset(&new_data->eld, 0x0, sizeof(new_data->eld));
	fb_edid_to_monspecs(data, specs);
	if (specs->modedb == NULL) {
		ret = -EINVAL;
		goto fail;
	}
	memcpy(new_data->eld.monitor_name, specs->monitor, sizeof(specs->monitor));
	new_data->eld.mnl = strlen(new_data->eld.monitor_name) + 1;
	new_data->eld.product_id[0] = data[0x8];
	new_data->eld.product_id[1] = data[0x9];
	new_data->eld.manufacture_id[0] = data[0xA];
	new_data->eld.manufacture_id[1] = data[0xB];

	extension_blocks = data[0x7e];

	for (i = 1; i <= extension_blocks; i++) {
		ret = tegra_edid_read_block(edid, i, data + i * 128);
		if (ret < 0)
			break;

		if (data[i * 128] == 0x2) {
			fb_edid_add_monspecs(data + i * 128, specs);

			tegra_edid_parse_ext_block(data + i * 128,
					data[i * 128 + 2], new_data);

			if (new_data->support_stereo) {
				for (j = 0; j < specs->modedb_len; j++) {
					if (tegra_edid_mode_support_stereo(
						&specs->modedb[j]))
						specs->modedb[j].vmode |=
#ifndef CONFIG_TEGRA_HDMI_74MHZ_LIMIT
						FB_VMODE_STEREO_FRAME_PACK;
#else
						FB_VMODE_STEREO_LEFT_RIGHT;
#endif
				}
			}
		}
	}

	new_data->dc_edid.len = i * 128;

	mutex_lock(&edid->lock);
	old_data = edid->data;
	edid->data = new_data;
	mutex_unlock(&edid->lock);

	if (old_data)
		kref_put(&old_data->refcnt, data_release);

	tegra_edid_dump(edid);
	return 0;

fail:
	vfree(new_data);
	return ret;
}

int tegra_edid_underscan_supported(struct tegra_edid *edid)
{
	if ((!edid) || (!edid->data))
		return 0;

	return edid->data->support_underscan;
}

int tegra_edid_get_eld(struct tegra_edid *edid, struct tegra_edid_hdmi_eld *elddata)
{
	if (!elddata || !edid->data)
		return -EFAULT;

	memcpy(elddata,&edid->data->eld,sizeof(struct tegra_edid_hdmi_eld));

	return 0;
}

struct tegra_edid *tegra_edid_create(int bus)
{
	struct tegra_edid *edid;
	struct i2c_adapter *adapter;
	int err;

	edid = kzalloc(sizeof(struct tegra_edid), GFP_KERNEL);
	if (!edid)
		return ERR_PTR(-ENOMEM);

	mutex_init(&edid->lock);
	strlcpy(edid->info.type, "tegra_edid", sizeof(edid->info.type));
	edid->bus = bus;
	edid->info.addr = 0x50;
	edid->info.platform_data = edid;

	adapter = i2c_get_adapter(bus);
	if (!adapter) {
		pr_err("can't get adpater for bus %d\n", bus);
		err = -EBUSY;
		goto free_edid;
	}

	edid->client = i2c_new_device(adapter, &edid->info);
	i2c_put_adapter(adapter);

	if (!edid->client) {
		pr_err("can't create new device\n");
		err = -EBUSY;
		goto free_edid;
	}

	/* Use filter */
	edid->filter = 1;

	tegra_edid_debug_add(edid);
	tegra_edid_debug_add_hex(edid);
	tegra_edid_debug_add_filter(edid);
	tegra_edid_debug_add_status(edid);

	return edid;

free_edid:
	kfree(edid);

	return ERR_PTR(err);
}

void tegra_edid_destroy(struct tegra_edid *edid)
{
	i2c_release_client(edid->client);
	if (edid->data)
		kref_put(&edid->data->refcnt, data_release);
	kfree(edid);
}

struct tegra_dc_edid *tegra_edid_get_data(struct tegra_edid *edid)
{
	struct tegra_edid_pvt *data;

	mutex_lock(&edid->lock);
	data = edid->data;
	if (data)
		kref_get(&data->refcnt);
	mutex_unlock(&edid->lock);

	return data ? &data->dc_edid : NULL;
}

void tegra_edid_put_data(struct tegra_dc_edid *data)
{
	struct tegra_edid_pvt *pvt;

	if (!data)
		return;

	pvt = container_of(data, struct tegra_edid_pvt, dc_edid);

	kref_put(&pvt->refcnt, data_release);
}

static int edid_dti=1;
static int __init tegra_edid_dti_setup(char *options)
{
	/*
	Default value is 0
	0 -- Do nothing, edid[54-71] is untouched
	1 -- Clean Up descriptor blocks edid[54-71]=0
	*/
	char **last = NULL;
	edid_dti = simple_strtoul(options, last, 0);
	return 0;
}
__setup("edid_dti=", tegra_edid_dti_setup);

static int edid_sti=1;
static int __init tegra_edid_sti_setup(char *options)
{
	/*
	Default value is 1
	0 -- Do nothing, edid[38-53] is untouched
	1 -- Clean Up Standard timing information 
	*/
	char **last = NULL;
	edid_sti = simple_strtoul(options, last, 0);
	return 0;
}
__setup("edid_sti=", tegra_edid_sti_setup);

static int edid_dti_off=18;
static int __init tegra_edid_dti_off_setup(char *options)
{
	/* 
	Default value is 18, 
	clean up the entire dti array except for the 1-st detailed timing descriptor.
	*/ 
	char **last = NULL;
	int _edid_dti_off = simple_strtoul(options, last, 0);
	edid_dti_off = (_edid_dti_off < 72) ? _edid_dti_off : edid_dti_off;
	return 0;
}
__setup("edid_dti_off=", tegra_edid_dti_off_setup);

static int edid_ext=0;
static int __init tegra_edid_ext_setup(char *options)
{
	/* 
	Default value is 0 
	0 -- Do nothing, edid[126] is untouched
	1 -- Clean Up extensions edid[126]=0
	*/ 
	char **last = NULL;
	edid_ext = simple_strtoul(options, last, 0);
	return 0;
}
__setup("edid_ext=", tegra_edid_ext_setup);

void tegra_edid_modes_init(struct tegra_dc_edid *data)
{
	/* Clean Up modes bitmap */
	data->buf[35] = 0x0;
	data->buf[36] = 0x0;
	data->buf[37] = 0x0;
	/* Clean Up standard timing information */
	if (edid_sti)
		memset(&data->buf[38],1,16);
	/* Clean Up Detailed timing descriptors' blocks */
	/* Preferred timing mode specified in descriptor block 1 */
	if (edid_dti)
		memset(&data->buf[54+edid_dti_off],0,72-edid_dti_off);
	/* Clean Up extensions */
	if (edid_ext)
		data->buf[126] = 0;
}
EXPORT_SYMBOL(tegra_edid_modes_init);

static int tegra_edid_mode_to_sti(struct fb_videomode *mode, unsigned char *sti) {
	/* X:Y pixel ratio: 00=16:10; 01=4:3; 10=5:4; 11=16:9 */
	struct ratio {
		int x;
		int y;
		unsigned char ratio_mask;
	} a_ratio[] = {{16,10,0x0}, {4,3,0x40}, {5,4,0x80}, {16,9,0xC0}};
	int i, len = sizeof(a_ratio);
	for ( i = 0 ; i < len ; i++ ) {
		if ((mode->xres / a_ratio[i].x - mode->yres / a_ratio[i].y) == 0) {
			sti[0] = (mode->xres >> 3) - 31;
			sti[1] = (mode->refresh > 60)  ? (mode->refresh - 60) : 0;
			sti[1] |= a_ratio[i].ratio_mask;
			return 0;
		}
	}
	return 1;
}

static unsigned char* tegra_edid_sti_find(struct tegra_dc_edid *data , unsigned char *sti) {
	int i=0;
	unsigned char *buf = &data->buf[EDID_STI_OFFSET];
	for (i = 0 ; i < 8 ; i++ ) {
		if ((buf[i*2] == sti[0]) && (buf[(i*2)+1] == sti[1]))
			return &buf[i*2];
	}
	return NULL;
}

void tegra_edid_mode_add(struct tegra_dc_edid *data, struct fb_videomode *mode) {
	unsigned char sti[2] = { 1 , 1 };
	unsigned char empty_sti[2] = { 1 , 1 };
	unsigned char *sti_position = NULL;
	struct established_timing_bitmap* etb = tegra_edid_etb_find(mode);

	if (etb) {
		data->buf[etb->byte] |= (1 << etb->bit);
		return;
	}

	if (tegra_edid_mode_to_sti(mode,sti))
		return;

	if (tegra_edid_sti_find(data,sti))
		return;

	sti_position = tegra_edid_sti_find(data,empty_sti);

	if (sti_position) {
		sti_position[0] = sti[0];
		sti_position[1] = sti[1];
	}
}
EXPORT_SYMBOL(tegra_edid_mode_add);

void tegra_edid_mode_rem(struct tegra_dc_edid *data, struct fb_videomode *mode) {
	unsigned char sti[2] = { 1 , 1 };
	unsigned char *sti_position = NULL;
	struct established_timing_bitmap* etb = tegra_edid_etb_find(mode);

	if (etb) {
		data->buf[etb->byte] &= ~(1 << etb->bit);
		return;
	}

	if (tegra_edid_mode_to_sti(mode,sti))
		return;

	sti_position = tegra_edid_sti_find(data,sti);

	if (sti_position) {
		sti_position[0] = 1;
		sti_position[1] = 1;
	}
}
EXPORT_SYMBOL(tegra_edid_mode_rem);

int tegra_edid_get_filter(struct tegra_edid *edid) {
	return edid->filter;
}
EXPORT_SYMBOL(tegra_edid_get_filter);

void tegra_edid_set_filter(struct tegra_edid *edid, int value) {
	edid->filter = value;
}
EXPORT_SYMBOL(tegra_edid_set_filter);

int tegra_edid_get_status(struct tegra_edid *edid) {
	return edid->status;
}
EXPORT_SYMBOL(tegra_edid_get_status);

void tegra_edid_set_status(struct tegra_edid *edid, int value) {
	edid->status = value;
}
EXPORT_SYMBOL(tegra_edid_set_status);

static const struct i2c_device_id tegra_edid_id[] = {
        { "tegra_edid", 0 },
        { }
};

MODULE_DEVICE_TABLE(i2c, tegra_edid_id);

static struct i2c_driver tegra_edid_driver = {
        .id_table = tegra_edid_id,
        .driver = {
                .name = "tegra_edid",
        },
};

static int __init tegra_edid_init(void)
{
        return i2c_add_driver(&tegra_edid_driver);
}

static void __exit tegra_edid_exit(void)
{
        i2c_del_driver(&tegra_edid_driver);
}

module_init(tegra_edid_init);
module_exit(tegra_edid_exit);

static struct established_timing_bitmap* tegra_edid_etb_find(struct fb_videomode *mode) {
	static struct established_timing_bitmap etb_array[] = {
		{ 720,400,70,35,7 },
		{ 720,400,88,35,6 },
		{ 640,480,60,35,5 },
		{ 640,480,67,35,4 },
		{ 640,480,72,35,3 },
		{ 640,480,75,35,2 },
		{ 800,600,56,35,1 },
		{ 800,600,60,35,0 },
		{ 800,600,72,36,7 },
		{ 800,600,75,36,6 },
		{ 832,624,75,36,5 },
		{ 1024,768,87,36,4 },
		{ 1024,768,60,36,3 },
		{ 1024,768,72,36,2 },
		{ 1024,768,75,36,1 },
		{ 1280,1024,75,36,0 },
	};
	int etb_len=sizeof(etb_array);
	int i=0;
	for (i = 0 ; i < etb_len ; i++ ) {
		struct established_timing_bitmap *etb = &etb_array[i];
		if ((mode->xres == etb->xres) && (mode->yres == etb->yres) && (mode->refresh == etb->refresh))
			return etb;
	}
	return NULL;
}
