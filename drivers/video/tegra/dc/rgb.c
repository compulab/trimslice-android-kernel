/*
 * drivers/video/tegra/dc/rgb.c
 *
 * Copyright (C) 2010 Google, Inc.
 * Author: Erik Gilling <konkers@android.com>
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

#include <linux/kernel.h>
#include <linux/fb.h>

#include <mach/dc.h>
#include <mach/fb.h>

#include "dc_reg.h"
#include "dc_priv.h"

#include "edid.h"

static const u32 tegra_dc_rgb_enable_pintable[] = {
	DC_COM_PIN_OUTPUT_ENABLE0,	0x00000000,
	DC_COM_PIN_OUTPUT_ENABLE1,	0x00000000,
	DC_COM_PIN_OUTPUT_ENABLE2,	0x00000000,
	DC_COM_PIN_OUTPUT_ENABLE3,	0x00000000,
	DC_COM_PIN_OUTPUT_POLARITY0,	0x00000000,
	DC_COM_PIN_OUTPUT_POLARITY1,	0x01000000,
	DC_COM_PIN_OUTPUT_POLARITY2,	0x00000000,
	DC_COM_PIN_OUTPUT_POLARITY3,	0x00000000,
	DC_COM_PIN_OUTPUT_DATA0,	0x00000000,
	DC_COM_PIN_OUTPUT_DATA1,	0x00000000,
	DC_COM_PIN_OUTPUT_DATA2,	0x00000000,
	DC_COM_PIN_OUTPUT_DATA3,	0x00000000,
	DC_COM_PIN_OUTPUT_SELECT0,	0x00000000,
	DC_COM_PIN_OUTPUT_SELECT1,	0x00000000,
	DC_COM_PIN_OUTPUT_SELECT2,	0x00000000,
	DC_COM_PIN_OUTPUT_SELECT3,	0x00000000,
	DC_COM_PIN_OUTPUT_SELECT4,	0x00210222,
	DC_COM_PIN_OUTPUT_SELECT5,	0x00002200,
	DC_COM_PIN_OUTPUT_SELECT6,	0x00020000,
};

static const u32 tegra_dc_rgb_disable_pintable[] = {
	DC_COM_PIN_OUTPUT_ENABLE0,	0x55555555,
	DC_COM_PIN_OUTPUT_ENABLE1,	0x55150005,
	DC_COM_PIN_OUTPUT_ENABLE2,	0x55555555,
	DC_COM_PIN_OUTPUT_ENABLE3,	0x55555555,
	DC_COM_PIN_OUTPUT_POLARITY0,	0x00000000,
	DC_COM_PIN_OUTPUT_POLARITY1,	0x00000000,
	DC_COM_PIN_OUTPUT_POLARITY2,	0x00000000,
	DC_COM_PIN_OUTPUT_POLARITY3,	0x00000000,
	DC_COM_PIN_OUTPUT_DATA0,	0xaaaaaaaa,
	DC_COM_PIN_OUTPUT_DATA1,	0xaaaaaaaa,
	DC_COM_PIN_OUTPUT_DATA2,	0xaaaaaaaa,
	DC_COM_PIN_OUTPUT_DATA3,	0xaaaaaaaa,
	DC_COM_PIN_OUTPUT_SELECT0,	0x00000000,
	DC_COM_PIN_OUTPUT_SELECT1,	0x00000000,
	DC_COM_PIN_OUTPUT_SELECT2,	0x00000000,
	DC_COM_PIN_OUTPUT_SELECT3,	0x00000000,
	DC_COM_PIN_OUTPUT_SELECT4,	0x00000000,
	DC_COM_PIN_OUTPUT_SELECT5,	0x00000000,
	DC_COM_PIN_OUTPUT_SELECT6,	0x00000000,
};

void tegra_dc_rgb_enable(struct tegra_dc *dc)
{
	tegra_dc_writel(dc, PW0_ENABLE | PW1_ENABLE | PW2_ENABLE | PW3_ENABLE |
			PW4_ENABLE | PM0_ENABLE | PM1_ENABLE,
			DC_CMD_DISPLAY_POWER_CONTROL);

	tegra_dc_writel(dc, DISP_CTRL_MODE_C_DISPLAY, DC_CMD_DISPLAY_COMMAND);

	tegra_dc_write_table(dc, tegra_dc_rgb_enable_pintable);
}

void tegra_dc_rgb_disable(struct tegra_dc *dc)
{
	tegra_dc_writel(dc, 0x00000000, DC_CMD_DISPLAY_POWER_CONTROL);

	tegra_dc_write_table(dc, tegra_dc_rgb_disable_pintable);
}

extern int tegra_dc_check_best_rate(struct tegra_dc_mode *mode);

#define LCD_MAX_HORIZONTAL_RESOLUTION 1680
#define LCD_MAX_VERTICAL_RESOLUTION 1050

static bool tegra_dc_rgb_mode_filter(struct tegra_dc *dc, struct fb_videomode *mode)
{
	int clocks;
	struct tegra_dc_mode dc_mode;
	bool mode_supported = false;

	/* sanity check for EDID modes */
	if (mode->pixclock == 0)
		return false;

	clocks = (mode->left_margin + mode->xres + mode->right_margin
		+ mode->hsync_len) *
		(mode->upper_margin + mode->yres + mode->lower_margin
		+ mode->vsync_len);
	if (clocks)
		mode->refresh = (PICOS2KHZ(mode->pixclock) * 1000) / clocks;

	dc_mode.pclk = PICOS2KHZ(mode->pixclock) * 1000;
	dc_mode.h_active = mode->xres;
	dc_mode.v_active = mode->yres;

	if (tegra_dc_check_pll_rate(dc, &dc_mode) >0 )
		mode_supported = true;

	if (mode->xres > LCD_MAX_HORIZONTAL_RESOLUTION ||
		mode->yres > LCD_MAX_VERTICAL_RESOLUTION )
		mode_supported = false;

	pr_info("\t%dx%d-%d (pclk=%d) -> %s\n",
		dc_mode.h_active, dc_mode.v_active,
		mode->refresh, dc_mode.pclk,
		mode_supported ? "supported" : "rejected");

	return mode_supported;
}

extern void tegra_dc_create_default_monspecs(int default_mode,
					struct fb_monspecs *specs);

static void tegra_dc_rgb_detect(struct tegra_dc *dc)
{
	struct fb_monspecs specs;
	int err;

	if (!dc->edid)
		goto fail;

	err = tegra_edid_get_monspecs(dc->edid, &specs);

	if (err < 0 && !dc->pdata->default_mode) {
		dev_err(&dc->ndev->dev, "error reading edid\n");
		goto fail;
	}else if (dc->pdata->default_mode) {

		dev_info(&dc->ndev->dev,"ignore EDID data, using the default " \
			"DVI resolutions");
		tegra_dc_create_default_monspecs(dc->pdata->default_mode,
						&specs);
	}

	/* monitors like to lie about these but they are still useful for
	 * detecting aspect ratios
	 */
	dc->out->h_size = specs.max_x * 1000;
	dc->out->v_size = specs.max_y * 1000;

	tegra_fb_update_monspecs(dc->fb, &specs, tegra_dc_rgb_mode_filter);
	dev_info(&dc->ndev->dev, "display detected\n");

	dc->connected = true;
	tegra_dc_ext_process_hotplug(dc->ndev->id);

fail:
	return;
}

struct tegra_dc_out_ops tegra_dc_rgb_ops = {
	.enable = tegra_dc_rgb_enable,
	.disable = tegra_dc_rgb_disable,
	.detect = tegra_dc_rgb_detect,
};

