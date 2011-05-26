/*
 * arch/arm/mach-tegra/board-trimslice.c
 *
 * Copyright (C) 2011 CompuLab, Ltd.
 * Author: Mike Rapoport <mike@compulab.co.il>
 *
 * Based on board-harmony.c
 * Copyright (C) 2010 Google, Inc.
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
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/serial_8250.h>
#include <linux/io.h>
#include <linux/platform_data/tegra_usb.h>
#include <linux/i2c.h>
#include <linux/i2c-tegra.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/setup.h>

#include <mach/iomap.h>
#include <mach/sdhci.h>
#include <mach/usb_phy.h>
#include <mach/gpio.h>
#include <mach/suspend.h>

#include <mach/clk.h>
#include <mach/powergate.h>


#include "board.h"
#include "clock.h"
#include "devices.h"
#include "gpio-names.h"
#include "power.h"
#include "wakeups-t2.h"


#include "board-trimslice.h"

#define PMC_CTRL               0x0
#define PMC_CTRL_INTR_LOW      (1 << 17)


static struct plat_serial8250_port debug_uart_platform_data[] = {
	{
		.membase	= IO_ADDRESS(TEGRA_UARTA_BASE),
		.mapbase	= TEGRA_UARTA_BASE,
		.irq		= INT_UARTA,
		.flags		= UPF_BOOT_AUTOCONF,
		.iotype		= UPIO_MEM,
		.regshift	= 2,
		.uartclk	= 216000000,
	}, {
		.flags		= 0
	}
};

static struct platform_device debug_uart = {
	.name	= "serial8250",
	.id	= PLAT8250_DEV_PLATFORM,
	.dev	= {
		.platform_data	= debug_uart_platform_data,
	},
};
static struct tegra_sdhci_platform_data sdhci_pdata1 = {
	.cd_gpio	= -1,
	.wp_gpio	= -1,
	.power_gpio	= -1,
};

static struct tegra_sdhci_platform_data sdhci_pdata4 = {
	.cd_gpio	= TRIMSLICE_GPIO_SD4_CD,
	.wp_gpio	= TRIMSLICE_GPIO_SD4_WP,
	.power_gpio	= -1,
};

static struct platform_device trimslice_audio_device = {
	.name	= "tegra-snd-trimslice",
	.id	= 0,
};

static struct platform_device spdif_dit_device = {
	.name   = "spdif-dit",
	.id     = -1,
};

static struct platform_device *trimslice_devices[] __initdata = {
	&debug_uart,
	&tegra_sdhci_device1,
	&tegra_sdhci_device4,
	&tegra_pmu_device,
	/* &tegra_rtc_device, */
	&tegra_gart_device,
	/* &audio_device, */
	&tegra_avp_device,
	&tegra_i2s_device1,
	&tegra_das_device,
	&tegra_pcm_device,
	&tegra_spdif_device,
	&spdif_dit_device,
	&trimslice_audio_device,
};

struct tegra_ulpi_config ehci2_phy_config = {
	.reset_gpio = TEGRA_GPIO_PV0,
	.clk = "cdev2",
};

static struct tegra_ehci_platform_data ehci_ulpi_data = {
	.operating_mode = TEGRA_USB_HOST,
	.phy_config = &ehci2_phy_config,
};

static struct tegra_ehci_platform_data ehci_utmi_data = {
	.operating_mode = TEGRA_USB_HOST,
};

static void trimslice_usb_init(void)
{
	tegra_ehci3_device.dev.platform_data = &ehci_utmi_data;
	platform_device_register(&tegra_ehci3_device);

	tegra_ehci2_device.dev.platform_data = &ehci_ulpi_data;
	platform_device_register(&tegra_ehci2_device);

	tegra_gpio_enable(TEGRA_GPIO_PV2);
	gpio_request(TEGRA_GPIO_PV2, "usb1 mode");
	gpio_direction_output(TEGRA_GPIO_PV2, 1);

	tegra_ehci1_device.dev.platform_data = &ehci_utmi_data;
	platform_device_register(&tegra_ehci1_device);
}

static const struct tegra_pingroup_config i2c1_ddc = {
	.pingroup	= TEGRA_PINGROUP_RM,
	.func		= TEGRA_MUX_I2C,
};

static struct tegra_i2c_platform_data trimslice_i2c1_platform_data = {
        .adapter_nr     = 0,
        .bus_count      = 1,
	.bus_clk_rate   = { 400000, 0 },
	.bus_mux	= { &i2c1_ddc, 0 },
        .bus_mux_len    = { 1, 1 },
};

static const struct tegra_pingroup_config i2c2_ddc = {
	.pingroup	= TEGRA_PINGROUP_DDC,
	.func		= TEGRA_MUX_I2C2,
};

static struct tegra_i2c_platform_data trimslice_i2c2_platform_data = {
        .adapter_nr     = 1,
        .bus_count      = 1,
	.bus_clk_rate   = { 400000, 0 },
	.bus_mux	= { &i2c2_ddc, 0 },
	.bus_mux_len	= { 1, 0 },
};

static const struct tegra_pingroup_config i2c3_gen_i2c = {
	.pingroup	= TEGRA_PINGROUP_DTF,
	.func		= TEGRA_MUX_I2C3,
};

static struct tegra_i2c_platform_data trimslice_i2c3_platform_data = {
        .adapter_nr     = 2,
        .bus_count      = 1,
	.bus_clk_rate   = { 400000, 0 },
	.bus_mux	= { &i2c3_gen_i2c, 0 },
	.bus_mux_len	= { 1, 0 },
};

static struct i2c_board_info trimslice_i2c3_board_info[] = {
	{
		I2C_BOARD_INFO("tlv320aic23", 0x1a),
	},
	{
		I2C_BOARD_INFO("em3027", 0x56),
	},
};

static void trimslice_i2c_init(void)
{
	tegra_i2c_device1.dev.platform_data = &trimslice_i2c1_platform_data;
	tegra_i2c_device2.dev.platform_data = &trimslice_i2c2_platform_data;
	tegra_i2c_device3.dev.platform_data = &trimslice_i2c3_platform_data;

	platform_device_register(&tegra_i2c_device1);
	platform_device_register(&tegra_i2c_device2);
	platform_device_register(&tegra_i2c_device3);

	i2c_register_board_info(2, trimslice_i2c3_board_info,
				ARRAY_SIZE(trimslice_i2c3_board_info));
}

static void __init tegra_trimslice_fixup(struct machine_desc *desc,
	struct tag *tags, char **cmdline, struct meminfo *mi)
{
	mi->nr_banks = 2;
	mi->bank[0].start = PHYS_OFFSET;
	mi->bank[0].size = 448 * SZ_1M;
	mi->bank[1].start = SZ_512M;
	mi->bank[1].size = SZ_512M;
}

static __initdata struct tegra_clk_init_table trimslice_clk_init_table[] = {
	/* name		parent		rate		enabled */
	{ "uarta",	"pll_p",	216000000,	true },
	{ NULL,		NULL,		0,		0},
};

static int __init tegra_trimslice_pci_init(void)
{
	if (!machine_is_trimslice())
		return 0;

	return tegra_pcie_init(true, false);
}
subsys_initcall(tegra_trimslice_pci_init);


static struct tegra_suspend_platform_data trimslice_suspend_data = {
	.cpu_timer = 5000,
	.cpu_off_timer = 5000,
	.core_timer = 0x7e7e,
	.core_off_timer = 0x7f,
	.separate_req = true,
	.corereq_high = false,
	.sysclkreq_high = true,
	.suspend_mode = TEGRA_SUSPEND_LP1,
};

/*
static struct tegra_suspend_platform_data trimslice_suspend_data = {
       .cpu_timer      = 2000,
       .cpu_off_timer  = 0,
       .suspend_mode   = TEGRA_SUSPEND_LP2,
       .core_timer     = 0x7e7e,
       .core_off_timer = 0,
       .separate_req   = true,
       .corereq_high   = false,
       .sysclkreq_high = true,
       .wake_enb       = TEGRA_WAKE_GPIO_PV2,
       .wake_high      = 0,
       .wake_low       = TEGRA_WAKE_GPIO_PV2,
       .wake_any       = 0,
};
*/


int __init trimslice_pm_init(void)
{
       void __iomem *pmc = IO_ADDRESS(TEGRA_PMC_BASE);
       u32 pmc_ctrl;

       /* configure the power management controller to trigger PMU
        * interrupts when low */
       pmc_ctrl = readl(pmc + PMC_CTRL);
       writel(pmc_ctrl | PMC_CTRL_INTR_LOW, pmc + PMC_CTRL);

       tegra_init_suspend(&trimslice_suspend_data);
       return 0;
}


static void __init tegra_trimslice_init(void)
{
	trimslice_pm_init();

	tegra_clk_init_from_table(trimslice_clk_init_table);


	trimslice_pinmux_init();

	/* Disable video input */
	tegra_gpio_enable(TEGRA_GPIO_PP6);
	gpio_request(TEGRA_GPIO_PP6, "video_en");
	gpio_direction_output(TEGRA_GPIO_PP6, 0);

	tegra_sdhci_device1.dev.platform_data = &sdhci_pdata1;
	tegra_sdhci_device4.dev.platform_data = &sdhci_pdata4;

	platform_add_devices(trimslice_devices, ARRAY_SIZE(trimslice_devices));

	trimslice_usb_init();
	trimslice_i2c_init();
	trimslice_panel_init();
}

MACHINE_START(TRIMSLICE, "trimslice")
	.boot_params	= 0x00000100,
	.fixup		= tegra_trimslice_fixup,
	.map_io         = tegra_map_common_io,
	.init_early	= tegra_init_early,
	.init_irq       = tegra_init_irq,
	.timer          = &tegra_timer,
	.init_machine   = tegra_trimslice_init,
MACHINE_END
