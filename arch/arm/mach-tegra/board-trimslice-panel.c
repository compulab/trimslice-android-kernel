/*
 * arch/arm/mach-tegra/board-trimslice-panel.c
 *
 * Copyright (c) 2010, NVIDIA Corporation.
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
#include <mach/nvhost.h>
#include <mach/nvmap.h>
#include <mach/irqs.h>
#include <mach/iomap.h>
#include <mach/dc.h>
#include <mach/fb.h>

#include "devices.h"
#include "gpio-names.h"

#define trimslice_lvds_shutdown	TEGRA_GPIO_PP5
#define trimslice_hdmi_hpd	TEGRA_GPIO_PN7

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
		.start	= 0x18012000,
		.end	= 0x18414000 - 1, /* enough for 1080P 16bpp */
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
		.start	= 0x18414000,
		.end	= 0x18BFD000 - 1,
	},
	{
		.name	= "hdmi_regs",
		.start	= TEGRA_HDMI_BASE,
		.end	= TEGRA_HDMI_BASE + TEGRA_HDMI_SIZE - 1,
		.flags	= IORESOURCE_MEM,
	},
};

static struct tegra_dc_mode trimslice_panel_modes[] = {
	{
		.pclk = 59400000,
		.h_ref_to_sync = 11,
		.v_ref_to_sync = 1,
		.h_sync_width = 58,
		.v_sync_width = 4,
		.h_back_porch = 58,
		.v_back_porch = 4,
		.h_active = 1024,
		.v_active = 768,
		.h_front_porch = 58,
		.v_front_porch = 4,
	},
};

static struct tegra_fb_data trimslice_fb_data = {
	.win		= 0,
	.xres		= 1024,
	.yres		= 768,
	.bits_per_pixel	= 16,
};

static struct tegra_fb_data trimslice_hdmi_fb_data = {
	.win		= 0,
	.xres		= 1280,
	.yres		= 720,
	.bits_per_pixel	= 16,
};

static struct tegra_dc_out trimslice_disp1_out = {
	.type		= TEGRA_DC_OUT_RGB,

	.align		= TEGRA_DC_ALIGN_LSB,
	.order		= TEGRA_DC_ORDER_RED_BLUE,

	.modes	 	= trimslice_panel_modes,
	.n_modes 	= ARRAY_SIZE(trimslice_panel_modes),

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
	.flags		= TEGRA_DC_FLAG_ENABLED,
	.default_out	= &trimslice_disp1_out,
	.fb		= &trimslice_fb_data,
};

static struct tegra_dc_platform_data trimslice_disp2_pdata = {
	.flags		= TEGRA_DC_FLAG_ENABLED,
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
	[0] = {
		.name		= "iram",
		.usage_mask	= NVMAP_HEAP_CARVEOUT_IRAM,
		.base		= TEGRA_IRAM_BASE,
		.size		= TEGRA_IRAM_SIZE,
		.buddy_size	= 0, /* no buddy allocation for IRAM */
	},
	[1] = {
		.name		= "generic-0",
		.usage_mask	= NVMAP_HEAP_CARVEOUT_GENERIC,
		.base		= 0x18C00000,
		.size		= SZ_128M - 0xC00000,
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
	&tegra_grhost_device,
	&tegra_pwfm2_device,
};

int __init trimslice_panel_init(void)
{
	int err;

	gpio_request(trimslice_hdmi_hpd, "hdmi_hpd");
	gpio_direction_input(trimslice_hdmi_hpd);
	tegra_gpio_enable(trimslice_hdmi_hpd);

	err = platform_add_devices(trimslice_gfx_devices,
				   ARRAY_SIZE(trimslice_gfx_devices));

	if (!err)
		err = nvhost_device_register(&trimslice_disp1_device);

	if (!err)
		err = nvhost_device_register(&trimslice_disp2_device);

	return err;
}

