/*
 * drivers/video/tegra/dc/hdmi.c
 *
 * Copyright (C) 2011 Compulab, Inc.
 * Author: Konstantin Sinyuk <kostyas@compulab.co.il>
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

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/fb.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>

#include <mach/clk.h>
#include <mach/dc.h>
#include <mach/fb.h>

#include <video/tegrafb.h>

#include "dc_reg.h"
#include "dc_priv.h"
#include "hdmi_reg.h"
#include "hdmi.h"
#include "edid.h"
#include "nvhdcp.h"

#define DEFAULT_MODE_DISABLED   0
#define DEFAULT_MODE_TEST       1
#define DEFAULT_MODE_720P       2
#define DEFAULT_MODE_1080P      3

const struct fb_videomode tegra_dc_atp_supported_modes[] = {
	/* 800x600p 72hz: Syntetic mode used for ATP DVI testing */
	{
		.xres =          800,
		.yres =          600,
		.pixclock =      20000,
		.hsync_len =     120,
		.vsync_len =     6,
		.left_margin  =  64,
		.upper_margin =  23,
		.right_margin =  56,
		.lower_margin =  37,
		.vmode =         FB_VMODE_NONINTERLACED,
		.sync = FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,
        },

};

const struct fb_videomode tegra_dc_720p_supported_modes[] = {

	/* 1280x720p 60hz: EIA/CEA-861-B Format 4 */
	{
		.xres =		1280,
		.yres =		720,
		.pixclock =	KHZ2PICOS(74250),
		.hsync_len =	40,	/* h_sync_width */
		.vsync_len =	5,	/* v_sync_width */
		.left_margin =	220,	/* h_back_porch */
		.upper_margin =	20,	/* v_back_porch */
		.right_margin =	110,	/* h_front_porch */
		.lower_margin =	5,	/* v_front_porch */
		.vmode =	FB_VMODE_NONINTERLACED,
		.sync = FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,
	},
	/* 720x480p 59.94hz: EIA/CEA-861-B Formats 2 & 3 */
	{
		.xres =		720,
		.yres =		480,
		.pixclock =	KHZ2PICOS(27000),
		.hsync_len =	62,	/* h_sync_width */
		.vsync_len =	6,	/* v_sync_width */
		.left_margin =	60,	/* h_back_porch */
		.upper_margin =	30,	/* v_back_porch */
		.right_margin =	16,	/* h_front_porch */
		.lower_margin =	9,	/* v_front_porch */
		.vmode =	FB_VMODE_NONINTERLACED,
		.sync = 0,
	},

	/* 640x480p 60hz: EIA/CEA-861-B Format 1 */
	{
		.xres =		640,
		.yres =		480,
		.pixclock =	KHZ2PICOS(25200),
		.hsync_len =	96,	/* h_sync_width */
		.vsync_len =	2,	/* v_sync_width */
		.left_margin =	48,	/* h_back_porch */
		.upper_margin =	33,	/* v_back_porch */
		.right_margin =	16,	/* h_front_porch */
		.lower_margin =	10,	/* v_front_porch */
		.vmode =	FB_VMODE_NONINTERLACED,
		.sync = 0,
	},

	/* 720x576p 50hz EIA/CEA-861-B Formats 17 & 18 */
	{
		.xres =		720,
		.yres =		576,
		.pixclock =	KHZ2PICOS(27000),
		.hsync_len =	64,	/* h_sync_width */
		.vsync_len =	5,	/* v_sync_width */
		.left_margin =	68,	/* h_back_porch */
		.upper_margin =	39,	/* v_back_porch */
		.right_margin =	12,	/* h_front_porch */
		.lower_margin =	5,	/* v_front_porch */
		.vmode =	FB_VMODE_NONINTERLACED,
		.sync = 0,
	},
};

const struct fb_videomode tegra_dc_1080p_supported_modes[] = {

	/* 1920x1080p 59.94/60hz EIA/CEA-861-B Format 16 */
	{
		.xres =		1920,
		.yres =		1080,
		.pixclock =	KHZ2PICOS(148500),
		.hsync_len =	44,	/* h_sync_width */
		.vsync_len =	5,	/* v_sync_width */
		.left_margin =	148,	/* h_back_porch */
		.upper_margin =	36,	/* v_back_porch */
		.right_margin =	88,	/* h_front_porch */
		.lower_margin =	4,	/* v_front_porch */
		.vmode =	FB_VMODE_NONINTERLACED,
		.sync = FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,
	},

	/* 1280x720p 60hz: EIA/CEA-861-B Format 4 */
	{
		.xres =		1280,
		.yres =		720,
		.pixclock =	KHZ2PICOS(74250),
		.hsync_len =	40,	/* h_sync_width */
		.vsync_len =	5,	/* v_sync_width */
		.left_margin =	220,	/* h_back_porch */
		.upper_margin =	20,	/* v_back_porch */
		.right_margin =	110,	/* h_front_porch */
		.lower_margin =	5,	/* v_front_porch */
		.vmode =	FB_VMODE_NONINTERLACED,
		.sync = FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,
	},

	/* 720x480p 59.94hz: EIA/CEA-861-B Formats 2 & 3 */
	{
		.xres =		720,
		.yres =		480,
		.pixclock =	KHZ2PICOS(27000),
		.hsync_len =	62,	/* h_sync_width */
		.vsync_len =	6,	/* v_sync_width */
		.left_margin =	60,	/* h_back_porch */
		.upper_margin =	30,	/* v_back_porch */
		.right_margin =	16,	/* h_front_porch */
		.lower_margin =	9,	/* v_front_porch */
		.vmode =	FB_VMODE_NONINTERLACED,
		.sync = 0,
	},

	/* 640x480p 60hz: EIA/CEA-861-B Format 1 */
	{
		.xres =		640,
		.yres =		480,
		.pixclock =	KHZ2PICOS(25200),
		.hsync_len =	96,	/* h_sync_width */
		.vsync_len =	2,	/* v_sync_width */
		.left_margin =	48,	/* h_back_porch */
		.upper_margin =	33,	/* v_back_porch */
		.right_margin =	16,	/* h_front_porch */
		.lower_margin =	10,	/* v_front_porch */
		.vmode =	FB_VMODE_NONINTERLACED,
		.sync = 0,
	},

	/* 720x576p 50hz EIA/CEA-861-B Formats 17 & 18 */
	{
		.xres =		720,
		.yres =		576,
		.pixclock =	KHZ2PICOS(27000),
		.hsync_len =	64,	/* h_sync_width */
		.vsync_len =	5,	/* v_sync_width */
		.left_margin =	68,	/* h_back_porch */
		.upper_margin =	39,	/* v_back_porch */
		.right_margin =	12,	/* h_front_porch */
		.lower_margin =	5,	/* v_front_porch */
		.vmode =	FB_VMODE_NONINTERLACED,
		.sync = 0,
	},
};

static struct fb_monspecs __initdata default_monspecs = {
	   .manufacturer           = "CL",
           .monitor                = "DEFAULT",
	   .max_x                  =  48,
	   .max_y                  =  27,
	   .misc                   =  2,
	   .hfmin                  =  30000,
	   .hfmax                  =  83000,
	   .vfmin                  =  56,
	   .vfmax                  =  75,
	   .dclkmax                =  150000000
};


static void dump_monitor_info(struct fb_monspecs *specs)
{
	int i;
	pr_err("specs:\n" \
		"manufacturer=%s\n" \
		"monitor=%s\n" \
		"max_x=%d\n" \
		"max_y=%d\n" \
		"misc=%d\n" \
		"hfmin=%d\n" \
		"hfmax=%d\n" \
		"vfmin=%d\n" \
		"vfmax=%d\n" \
		"dclkmax=%d\n",
		specs->manufacturer,
		specs->monitor,
		specs->max_x,
		specs->max_y,
		specs->misc,
		specs->hfmin,
		specs->hfmax,
		specs->vfmin,
		specs->vfmax,
		specs->dclkmax
	);

	for (i=0; i < specs->modedb_len; i++)
	{
		struct fb_videomode *mode;
		mode = &specs->modedb[i];

		pr_err("mode:\n" \
                        ".xres = %d,\n" \
		        ".yres = %d,\n" \
			".pixclock = %d,\n" \
			".hsync_len = %d,\n" \
			".vsync_len = %d,\n" \
			".left_margin  = %d,\n" \
		        ".upper_margin = %d,\n" \
		        ".right_margin = %d,\n" \
		        ".lower_margin = %d,\n" \
			".vmode = %d,\n" \
			".sync = %d,\n",
			mode->xres,
			mode->yres,
			mode->pixclock,
			mode->hsync_len,
			mode->vsync_len,
			mode->left_margin,
			mode->upper_margin,
			mode->right_margin,
			mode->lower_margin,
			mode->vmode,
			mode->sync
	        );
	}
}

void tegra_dc_create_default_monspecs(int default_mode,
				struct fb_monspecs *specs)
{
	if (default_mode == DEFAULT_MODE_DISABLED)
		return;
	memcpy(specs, &default_monspecs, sizeof (struct fb_monspecs));
	specs->modedb = kzalloc(50 * sizeof(struct fb_videomode), GFP_KERNEL);
	switch (default_mode) {
	  case DEFAULT_MODE_TEST:
		specs->modedb_len = ARRAY_SIZE(tegra_dc_atp_supported_modes);
		memcpy(specs->modedb, tegra_dc_atp_supported_modes,
			specs->modedb_len * sizeof(struct fb_videomode));
		break;
	  case DEFAULT_MODE_720P:
		specs->modedb_len = ARRAY_SIZE(tegra_dc_720p_supported_modes);
		memcpy(specs->modedb, tegra_dc_720p_supported_modes,
			specs->modedb_len * sizeof(struct fb_videomode));
		break;
	  case DEFAULT_MODE_1080P:
	        specs->modedb_len = ARRAY_SIZE(tegra_dc_1080p_supported_modes);
		memcpy(specs->modedb, tegra_dc_1080p_supported_modes,
		       specs->modedb_len * sizeof(struct fb_videomode));
		break;
	}
	dump_monitor_info(specs);
}
