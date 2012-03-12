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
#include <linux/seq_file.h>
#include <linux/debugfs.h>

#include <mach/dc.h>
#include <mach/fb.h>

#include "dc_reg.h"
#include "dc_priv.h"

#include "edid.h"
#include "tegra_dc_res.h"

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

typedef struct {
	unsigned int c;
	struct {
		u32 refresh;
		u32 xres;
		u32 yres;
	} res[32];
} resolutions_t;

extern void tegra_dc_create_default_monspecs(int default_mode, struct fb_monspecs *specs);

void tegra_dc_rgb_enable(struct tegra_dc *dc)
{
	int i;
	u32 out_sel_pintable[ARRAY_SIZE(tegra_dc_rgb_enable_out_sel_pintable)];

	tegra_dc_writel(dc, PW0_ENABLE | PW1_ENABLE | PW2_ENABLE | PW3_ENABLE |
			PW4_ENABLE | PM0_ENABLE | PM1_ENABLE,
			DC_CMD_DISPLAY_POWER_CONTROL);

	tegra_dc_writel(dc, DISP_CTRL_MODE_C_DISPLAY, DC_CMD_DISPLAY_COMMAND);

	tegra_dc_write_table(dc, tegra_dc_rgb_enable_pintable);

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

extern int tegra_dc_check_best_rate(struct tegra_dc_mode *mode);

#define LCD_MAX_HORIZONTAL_RESOLUTION 1680
#define LCD_MAX_VERTICAL_RESOLUTION 1050
#define LCD_MIN_REFRESH_RATE 50

static bool tegra_dc_rgb_mode_filter(const struct tegra_dc *dc, struct fb_videomode *mode)
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

	pr_info("\t%dx%d-%d (pclk=%d) -> %s\n",
		dc_mode.h_active, dc_mode.v_active,
		mode->refresh, dc_mode.pclk,
		mode_supported ? "supported" : "rejected");

	return mode_supported;
}

static int rgb_diagnostics(struct seq_file *s, void *data)
{
	int i = 0;
	resolutions_t *resolutions = s->private;
	for (i = 0 ; i < resolutions->c ; i++) {
		seq_printf(s, "%d %d %d\n", resolutions->res[i].xres,
					resolutions->res[i].yres,
					resolutions->res[i].refresh);
	}
	return 0;
}

static int rgb_diagnostics_open(struct inode *inode, struct file *file)
{
	return single_open(file, rgb_diagnostics, inode->i_private);
}

static const struct file_operations rgb_debug_fops = {
	.open		= rgb_diagnostics_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int setup_read(struct seq_file *s, void *data)
{
	static char setup_buffer[] = {TEGRA_DC_RES};
	seq_printf(s, "%s", &setup_buffer[0]);
	return 0;
}

static int setup_open(struct inode *inode, struct file *file)
{
	return single_open(file, setup_read, inode->i_private);
}

static const struct file_operations setup_debug_fops = {
	.open		= setup_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

void tegra_rgb_update_debugfs(struct tegra_dc *dc, struct fb_monspecs *specs)
{
	int i;
	static resolutions_t resolutions = { .c = 0 };
	struct dentry *debug_dir = NULL;
	struct dentry *debug_file = NULL;

	debug_file = debugfs_create_file("tegra_dc_res", S_IRUSR|S_IRGRP|S_IXUSR|S_IXGRP,
				debug_dir, NULL, &setup_debug_fops);

	debug_file = debugfs_create_file("tegra_dc_rgb_res", S_IRUSR|S_IRGRP,
				debug_dir, &resolutions, &rgb_debug_fops);

	resolutions.c = 0;
	for (i = 0; i < specs->modedb_len; i++) {
		struct fb_videomode *mode = &specs->modedb[i];
		if (tegra_dc_rgb_mode_filter(dc, mode)) {
			if (resolutions.c < 32) {
				resolutions.res[resolutions.c].refresh = mode->refresh;
				resolutions.res[resolutions.c].xres = mode->xres;
				resolutions.res[resolutions.c].yres = mode->yres;
				resolutions.c++;
			}
		}
	}
}

static bool tegra_dc_rgb_detect(struct tegra_dc *dc)
{
	struct fb_monspecs specs;
	int err;

	if (!dc->edid)
		goto fail;

	err = tegra_edid_get_monspecs(dc->edid, &specs);

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

	tegra_fb_update_monspecs(dc->fb, &specs, tegra_dc_rgb_mode_filter);
	tegra_rgb_update_debugfs(dc, &specs);
	dev_info(&dc->ndev->dev, "display detected\n");

	dc->connected = true;
	tegra_dc_ext_process_hotplug(dc->ndev->id);

	return true;

fail:
	return false;
}

struct tegra_dc_out_ops tegra_dc_rgb_ops = {
	.enable = tegra_dc_rgb_enable,
	.disable = tegra_dc_rgb_disable,
	.detect = tegra_dc_rgb_detect,
};

