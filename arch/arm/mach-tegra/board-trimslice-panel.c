/*
 * arch/arm/mach-tegra/board-trimslice-panel.c
 *
 * Copyright (c) 2010, 2011, NVIDIA Corporation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/resource.h>
#include <asm/mach-types.h>
#include <linux/platform_device.h>
#include <linux/pwm_backlight.h>
#include <linux/nvhost.h>
#include <mach/nvmap.h>
#include <mach/irqs.h>
#include <mach/iomap.h>
#include <mach/dc.h>
#include <mach/fb.h>

#include "devices.h"
#include "gpio-names.h"
#include "board.h"

#define trimslice_lvds_shutdown	TEGRA_GPIO_PP5
#define trimslice_hdmi_hpd	TEGRA_GPIO_PN7
#define RGB_XRES		1280	//720	//1366
#define RGB_YRES		720	//480	//768
#define RGB_COLOR_DEPTH		32
#define HDMI_XRES		1280
#define HDMI_YRES		720
#define HDMI_COLOR_DEPTH	32


static int trimslice_panel_enable(void)
{
	gpio_set_value(trimslice_lvds_shutdown, 1);
	return 0;
}

static int trimslice_panel_disable(void)
{
	gpio_set_value(trimslice_lvds_shutdown, 0);
	return 0;
}

static int trimslice_hdmi_enable(void)
{
	return 0;
}

static int trimslice_hdmi_disable(void)
{
	return 0;
}

static struct resource trimslice_disp1_resources[] = {
	{
		.name	= "irq",
		.start	= INT_DISPLAY_GENERAL,
		.end	= INT_DISPLAY_GENERAL,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.name	= "regs",
		.start	= TEGRA_DISPLAY_BASE,
		.end	= TEGRA_DISPLAY_BASE + TEGRA_DISPLAY_SIZE-1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.name	= "fbmem",
		.flags	= IORESOURCE_MEM,
	},
};

static struct resource trimslice_disp2_resources[] = {
	{
		.name	= "irq",
		.start	= INT_DISPLAY_B_GENERAL,
		.end	= INT_DISPLAY_B_GENERAL,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.name	= "regs",
		.start	= TEGRA_DISPLAY2_BASE,
		.end	= TEGRA_DISPLAY2_BASE + TEGRA_DISPLAY2_SIZE - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.name	= "fbmem",
		.flags	= IORESOURCE_MEM,
	},
	{
		.name	= "hdmi_regs",
		.start	= TEGRA_HDMI_BASE,
		.end	= TEGRA_HDMI_BASE + TEGRA_HDMI_SIZE - 1,
		.flags	= IORESOURCE_MEM,
	},
};


static struct tegra_fb_data trimslice_fb_data = {
	.win		= 0,
	.xres		= RGB_XRES,
	.yres		= RGB_YRES,
	.bits_per_pixel	= RGB_COLOR_DEPTH,
	.flags		= TEGRA_FB_FLIP_ON_PROBE,
};

static struct tegra_fb_data trimslice_hdmi_fb_data = {
	.xres		= HDMI_XRES,
	.yres		= HDMI_YRES,
	.bits_per_pixel	= HDMI_COLOR_DEPTH,
	.flags		= TEGRA_FB_FLIP_ON_PROBE,
};

static struct tegra_dc_out trimslice_disp1_out = {
	.type		= TEGRA_DC_OUT_RGB,

	.align		= TEGRA_DC_ALIGN_LSB,
	.order		= TEGRA_DC_ORDER_RED_BLUE,

	.enable		= trimslice_panel_enable,
	.disable	= trimslice_panel_disable,
};

static struct tegra_dc_out trimslice_disp2_out = {
	.type		= TEGRA_DC_OUT_HDMI,
	.flags		= TEGRA_DC_OUT_HOTPLUG_HIGH,

	.dcc_bus	= 1,
	.hotplug_gpio	= trimslice_hdmi_hpd,

	.align		= TEGRA_DC_ALIGN_MSB,
	.order		= TEGRA_DC_ORDER_RED_BLUE,

	.enable		= trimslice_hdmi_enable,
	.disable	= trimslice_hdmi_disable,
};

static struct tegra_dc_platform_data trimslice_disp1_pdata = {
	.flags		= TEGRA_DC_FLAG_USE_EDID,
	.default_out	= &trimslice_disp1_out,
	.fb		= &trimslice_fb_data,
};

static struct tegra_dc_platform_data trimslice_disp2_pdata = {
	.flags		= TEGRA_DC_FLAG_USE_EDID,
	.default_out	= &trimslice_disp2_out,
	.fb		= &trimslice_hdmi_fb_data,
};

static struct nvhost_device trimslice_disp1_device = {
	.name		= "tegradc",
	.id		= 0,
	.resource	= trimslice_disp1_resources,
	.num_resources	= ARRAY_SIZE(trimslice_disp1_resources),
	.dev = {
		.platform_data = &trimslice_disp1_pdata,
	},
};

static struct nvhost_device trimslice_disp2_device = {
	.name		= "tegradc",
	.id		= 1,
	.resource	= trimslice_disp2_resources,
	.num_resources	= ARRAY_SIZE(trimslice_disp2_resources),
	.dev = {
		.platform_data = &trimslice_disp2_pdata,
	},
};

static struct nvmap_platform_carveout trimslice_carveouts[] = {
	[0] = NVMAP_HEAP_CARVEOUT_IRAM_INIT,
	[1] = {
		.name		= "generic-0",
		.usage_mask	= NVMAP_HEAP_CARVEOUT_GENERIC,
		.buddy_size	= SZ_32K,
	},
};

static struct nvmap_platform_data trimslice_nvmap_data = {
	.carveouts	= trimslice_carveouts,
	.nr_carveouts	= ARRAY_SIZE(trimslice_carveouts),
};

static struct platform_device trimslice_nvmap_device = {
	.name	= "tegra-nvmap",
	.id	= -1,
	.dev	= {
		.platform_data = &trimslice_nvmap_data,
	},
};

static struct platform_device *trimslice_gfx_devices[] __initdata = {
	&trimslice_nvmap_device,
#ifdef CONFIG_TEGRA_GRHOST
	&tegra_grhost_device,
#endif

	// &tegra_pwfm2_device, 
};

static int  tegra_default_dvi_mode = 0;
static int  tegra_default_hdmi_mode = 0;

static int __init tegra_default_dvi_mode_setup(char *options)
{
	if (!strcmp(options, "test"))
		tegra_default_dvi_mode = 1;
	else if (!strcmp(options, "720p"))
		tegra_default_dvi_mode = 2;
	else
		tegra_default_dvi_mode = 0;
	return 0;
}
__setup("dvi=", tegra_default_dvi_mode_setup);

static int __init tegra_default_hdmi_mode_setup(char *options)
{
	if (!strcmp(options, "test"))
		tegra_default_hdmi_mode = 1;
	else if (!strcmp(options, "720p"))
		tegra_default_hdmi_mode = 2;
	else if (!strcmp(options, "1080p"))
		tegra_default_hdmi_mode = 3;
	else if (!strcmp(options, "hdready"))
		tegra_default_hdmi_mode = 4;
	else
		tegra_default_hdmi_mode = 0;
	return 0;
}
__setup("hdmi=", tegra_default_hdmi_mode_setup);



int __init trimslice_panel_init(void)
{
	int err = 0;
	struct resource __maybe_unused *res;

	/* Configure HDMI hotplug  as input */
	gpio_request(trimslice_hdmi_hpd, "hdmi_hpd");
	gpio_direction_input(trimslice_hdmi_hpd);
	tegra_gpio_enable(trimslice_hdmi_hpd);

	/* Disable DVI trasceiver by default */
	gpio_request(trimslice_lvds_shutdown, "dvi shutdown");
	gpio_direction_output(trimslice_lvds_shutdown, 0);
	tegra_gpio_enable(trimslice_lvds_shutdown);

	trimslice_carveouts[1].base = tegra_carveout_start;
	trimslice_carveouts[1].size = tegra_carveout_size;

	err = platform_add_devices(trimslice_gfx_devices,
				   ARRAY_SIZE(trimslice_gfx_devices));

#if defined(CONFIG_TEGRA_GRHOST) && defined(CONFIG_TEGRA_DC)
	res = nvhost_get_resource_byname(&trimslice_disp1_device,
		IORESOURCE_MEM, "fbmem");
	res->start = tegra_fb_start;
	res->end = tegra_fb_start + tegra_fb_size - 1;

	res = nvhost_get_resource_byname(&trimslice_disp2_device,
		IORESOURCE_MEM, "fbmem");
	res->start = tegra_fb2_start;
	res->end = tegra_fb2_start + tegra_fb2_size - 1;
	 
	/* Make nvhost devices aware one of the another.
	   This is required as the devices use mutual resource (PLL_D)
	   and should be aware of the neighbour requirement.
	 */
	trimslice_disp1_device.dev_neighbour =
		(struct device *) &trimslice_disp2_device;
	trimslice_disp2_device.dev_neighbour =
		(struct device *) &trimslice_disp1_device;

	((struct tegra_dc_platform_data*)
		(trimslice_disp1_device.dev.platform_data))->default_mode =
		tegra_default_dvi_mode;
	((struct tegra_dc_platform_data*)
		(trimslice_disp2_device.dev.platform_data))->default_mode =
		tegra_default_hdmi_mode;

	if (!err)
		err = nvhost_device_register(&trimslice_disp1_device);
	if (!err)
		err = nvhost_device_register(&trimslice_disp2_device);
#endif
	return err;
}

