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
#include <mach/dc.h>
 #include <mach/fb.h>

#include "dc_reg.h"
#include "dc_priv.h"
#include "edid.h"

static unsigned int LCD_MAX_HORIZONTAL_RESOLUTION=1680;
static unsigned int LCD_MAX_VERTICAL_RESOLUTION=1050;

#define LCD_MIN_REFRESH_RATE 50


static const u32 tegra_dc_rgb_enable_partial_pintable[] = {
	DC_COM_PIN_OUTPUT_ENABLE0,	0x00000000,
	DC_COM_PIN_OUTPUT_ENABLE1,	0x00000000,
	DC_COM_PIN_OUTPUT_ENABLE2,	0x00000000,
	DC_COM_PIN_OUTPUT_ENABLE3,	0x00000000,
	DC_COM_PIN_OUTPUT_POLARITY0,	0x00000000,
	DC_COM_PIN_OUTPUT_POLARITY2,	0x00000000,
	DC_COM_PIN_OUTPUT_DATA0,	0x00000000,
	DC_COM_PIN_OUTPUT_DATA1,	0x00000000,
	DC_COM_PIN_OUTPUT_DATA2,	0x00000000,
	DC_COM_PIN_OUTPUT_DATA3,	0x00000000,
};

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
};

static const u32 tegra_dc_rgb_enable_out_sel_pintable[] = {
	DC_COM_PIN_OUTPUT_SELECT0,	0x00000000,
	DC_COM_PIN_OUTPUT_SELECT1,	0x00000000,
	DC_COM_PIN_OUTPUT_SELECT2,	0x00000000,
#ifdef CONFIG_TEGRA_SILICON_PLATFORM
	DC_COM_PIN_OUTPUT_SELECT3,	0x00000000,
#else
	/* The display panel sub-board used on FPGA platforms (panel 86)
	   is non-standard. It expects the Data Enable signal on the WR
	   pin instead of the DE pin. */
	DC_COM_PIN_OUTPUT_SELECT3,	0x00200000,
#endif
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

struct tegra_dc_rgb_data {
	struct tegra_dc			*dc;
	struct tegra_edid		*edid;
	struct tegra_edid_hdmi_eld		eld;
	struct tegra_nvhdcp		*nvhdcp;
	spinlock_t			suspend_lock;
};

static int rgb_filter = 1;
static int rgb_edid   = 0;

void tegra_dc_rgb_enable(struct tegra_dc *dc)
{
	int i;
	u32 out_sel_pintable[ARRAY_SIZE(tegra_dc_rgb_enable_out_sel_pintable)];

	tegra_dc_writel(dc, PW0_ENABLE | PW1_ENABLE | PW2_ENABLE | PW3_ENABLE |
			PW4_ENABLE | PM0_ENABLE | PM1_ENABLE,
			DC_CMD_DISPLAY_POWER_CONTROL);

	tegra_dc_writel(dc, DISP_CTRL_MODE_C_DISPLAY, DC_CMD_DISPLAY_COMMAND);

	if (dc->out->out_pins) {
		tegra_dc_set_out_pin_polars(dc, dc->out->out_pins,
			dc->out->n_out_pins);
		tegra_dc_write_table(dc, tegra_dc_rgb_enable_partial_pintable);
	} else {
		tegra_dc_write_table(dc, tegra_dc_rgb_enable_pintable);
	}

	memcpy(out_sel_pintable, tegra_dc_rgb_enable_out_sel_pintable,
		sizeof(tegra_dc_rgb_enable_out_sel_pintable));

	if (dc->out && dc->out->out_sel_configs) {
		u8 *out_sels = dc->out->out_sel_configs;
		for (i = 0; i < dc->out->n_out_sel_configs; i++) {
			switch (out_sels[i]) {
			case TEGRA_PIN_OUT_CONFIG_SEL_LM1_M1:
				out_sel_pintable[5*2+1] =
					(out_sel_pintable[5*2+1] &
					~PIN5_LM1_LCD_M1_OUTPUT_MASK) |
					PIN5_LM1_LCD_M1_OUTPUT_M1;
				break;
			case TEGRA_PIN_OUT_CONFIG_SEL_LM1_LD21:
				out_sel_pintable[5*2+1] =
					(out_sel_pintable[5*2+1] &
					~PIN5_LM1_LCD_M1_OUTPUT_MASK) |
					PIN5_LM1_LCD_M1_OUTPUT_LD21;
				break;
			case TEGRA_PIN_OUT_CONFIG_SEL_LM1_PM1:
				out_sel_pintable[5*2+1] =
					(out_sel_pintable[5*2+1] &
					~PIN5_LM1_LCD_M1_OUTPUT_MASK) |
					PIN5_LM1_LCD_M1_OUTPUT_PM1;
				break;
			default:
				dev_err(&dc->ndev->dev,
					"Invalid pin config[%d]: %d\n",
					 i, out_sels[i]);
				break;
			}
		}
	}

	tegra_dc_write_table(dc, out_sel_pintable);
}

void tegra_dc_rgb_disable(struct tegra_dc *dc)
{
	tegra_dc_writel(dc, 0x00000000, DC_CMD_DISPLAY_POWER_CONTROL);

	tegra_dc_write_table(dc, tegra_dc_rgb_disable_pintable);
}

static bool tegra_dc_rgb_mode_filter_extra(const struct tegra_dc *dc, struct fb_videomode *mode)
{
	int clocks;
	struct tegra_dc_mode dc_mode;
	bool mode_supported = false;

	clocks = (mode->left_margin + mode->xres + mode->right_margin
		+ mode->hsync_len) *
		(mode->upper_margin + mode->yres + mode->lower_margin
		+ mode->vsync_len);
	if (clocks)
		mode->refresh = (PICOS2KHZ(mode->pixclock) * 1000) / clocks;

	if (mode->refresh < LCD_MIN_REFRESH_RATE)
		return false;

	dc_mode.pclk = PICOS2KHZ(mode->pixclock) * 1000;
	dc_mode.h_active = mode->xres;
	dc_mode.v_active = mode->yres;

	if (tegra_dc_check_pll_rate(dc, &dc_mode) > 0)
		mode_supported = true;

	if (mode->xres > LCD_MAX_HORIZONTAL_RESOLUTION ||
		mode->yres > LCD_MAX_VERTICAL_RESOLUTION)
		mode_supported = false;

	return mode_supported;
}

static bool tegra_dc_rgb_mode_filter(const struct tegra_dc *dc, struct fb_videomode *mode)
{
	bool mode_supported = false;
	struct tegra_dc_rgb_data *rgb = (struct tegra_dc_rgb_data *) tegra_dc_get_outdata((struct tegra_dc *) dc);

	mode_supported = tegra_dc_mode_filter(dc,mode);
	if (mode_supported && tegra_edid_get_filter(rgb->edid)) {
		/* Don't issue the function if filter is not set */
		mode_supported = tegra_dc_rgb_mode_filter_extra(dc,mode);
	}

	pr_info("dvi: \t%dx%d-%d (pclk=%ld) -> %s\n",
		mode->xres, mode->yres,
		mode->refresh, (PICOS2KHZ(mode->pixclock) * 1000),
		mode_supported ? "supported" : "rejected");

	if (mode_supported && tegra_edid_get_filter(rgb->edid)) {
		/* Edid fixer */
		struct tegra_dc_edid *tegra_dc_edid = tegra_edid_get_data(rgb->edid);
		tegra_edid_mode_add(tegra_dc_edid, mode);
		tegra_edid_put_data(tegra_dc_edid);
	}

	return mode_supported;
}

void tegra_dc_create_default_monspecs(int default_mode,
                                struct fb_monspecs *specs);

static bool tegra_dc_rgb_detect(struct tegra_dc *dc)
{
	struct tegra_dc_rgb_data *rgb = (struct tegra_dc_rgb_data *) tegra_dc_get_outdata(dc);
	struct fb_monspecs specs;
	int err;

	if (!rgb->edid)
		goto fail;

	err = tegra_edid_get_monspecs(rgb->edid, &specs);

	if (err < 0 && !dc->pdata->default_mode) {
		dev_err(&dc->ndev->dev, "error reading edid\n");
		goto fail;
	} else if (dc->pdata->default_mode) {

		dev_info(&dc->ndev->dev, "ignore EDID data, using the default " \
			"DVI resolutions");
		tegra_dc_create_default_monspecs(dc->pdata->default_mode,
						&specs);
	}

	/* monitors like to lie about these but they are still useful for
	 * detecting aspect ratios
	 */
	dc->out->h_size = specs.max_x * 1000;
	dc->out->v_size = specs.max_y * 1000;

	/* Don't touch the edid if filter is not set */
	if (tegra_edid_get_filter(rgb->edid)) {
		/* Edid fixer */
		struct tegra_dc_edid *tegra_dc_edid = tegra_edid_get_data(rgb->edid);
		tegra_edid_modes_init(tegra_dc_edid);
		tegra_edid_put_data(tegra_dc_edid);
	}

	tegra_fb_update_monspecs(dc->fb, &specs, dc->mode_filter);
	dev_info(&dc->ndev->dev, "display detected\n");

	dc->connected = true;
	tegra_dc_ext_process_hotplug(dc->ndev->id);
	return true;
fail:
	return false;
}

static struct tegra_dc_edid *tegra_dc_rgb_get_edid(struct tegra_dc *dc)
{
	struct tegra_dc_rgb_data *rgb = (struct tegra_dc_rgb_data *) tegra_dc_get_outdata((struct tegra_dc *) dc);
	if (tegra_edid_get_status(rgb->edid)) {
		struct tegra_dc_rgb_data *rgb;

		rgb = tegra_dc_get_outdata(dc);

		return tegra_edid_get_data(rgb->edid);
	} else {
		pr_warning("%s: no dvi edid mode\n",__FUNCTION__);
		return NULL;
	}
}

static void tegra_dc_rgb_put_edid(struct tegra_dc_edid *edid)
{
	tegra_edid_put_data(edid);
}

static int tegra_dc_rgb_init(struct tegra_dc *dc)
{
	struct tegra_dc_rgb_data *rgb = NULL;
	int err;

	rgb = kzalloc(sizeof(*rgb), GFP_KERNEL);
	if (!rgb)
		return -ENOMEM;

	rgb->edid = tegra_edid_create(dc->out->dcc_bus);
	if (IS_ERR_OR_NULL(rgb->edid)) {
		dev_err(&dc->ndev->dev, "rgb: can't create edid\n");
		err = PTR_ERR(rgb->edid);
		goto err_free_mem;
	}

	tegra_edid_set_filter(rgb->edid,rgb_filter);
	tegra_edid_set_status(rgb->edid,rgb_edid);

	tegra_dc_set_outdata(dc, rgb);
	rgb->dc = dc;

	spin_lock_init(&rgb->suspend_lock);

	dc->predefined_pll_rate = 0;
	dc->get_edid = tegra_dc_rgb_get_edid;
	dc->put_edid = tegra_dc_rgb_put_edid;
	dc->mode_filter = tegra_dc_rgb_mode_filter;

	return 0;
err_free_mem:
	if (rgb)
		kfree(rgb);
	return err;
}

static void tegra_dc_rgb_destroy(struct tegra_dc *dc)
{
	struct tegra_dc_rgb_data *rgb = tegra_dc_get_outdata(dc);
	tegra_edid_destroy(rgb->edid);
	kfree(rgb);
}

struct tegra_dc_out_ops tegra_dc_rgb_ops = {
	.init = tegra_dc_rgb_init,
	.destroy = tegra_dc_rgb_destroy,
	.enable = tegra_dc_rgb_enable,
	.disable = tegra_dc_rgb_disable,
	.detect = tegra_dc_rgb_detect,
};

static int __init tegra_dc_rgb_filter_setup(char *options)
{
	rgb_filter = (strcmp(options,"no") != 0);
	return 0;
}
__setup("rgb_filter=", tegra_dc_rgb_filter_setup);

static int __init tegra_dc_rgb_lcd_max_h_setup(char *options)
{
	char **last = NULL;
	unsigned int lcd_max_h = simple_strtoul(options, last, 0);
	if (lcd_max_h)
		LCD_MAX_HORIZONTAL_RESOLUTION = lcd_max_h;
	return 0;
}
__setup("lcd_max_h=", tegra_dc_rgb_lcd_max_h_setup);

static int __init tegra_dc_rgb_lcd_max_v_setup(char *options)
{
	char **last = NULL;
	unsigned int lcd_max_v = simple_strtoul(options, last, 0);
	if (lcd_max_v)
		LCD_MAX_VERTICAL_RESOLUTION = lcd_max_v;
	return 0;
}
__setup("lcd_max_v=", tegra_dc_rgb_lcd_max_v_setup);

static int __init tegra_dc_rgb_edid_setup(char *options)
{
	rgb_edid = (strcmp(options,"no") != 0);
	return 0;
}
__setup("rgb_edid=", tegra_dc_rgb_edid_setup);

