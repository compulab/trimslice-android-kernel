/*
 * linux/arch/arm/mach-tegra/include/mach/pinmux.h
 *
 * Copyright (C) 2010 Google, Inc.
 * Copyright (C) 2011 NVIDIA Corporation.
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

#ifndef __MACH_TEGRA_PINMUX_H
#define __MACH_TEGRA_PINMUX_H

#if defined(CONFIG_ARCH_TEGRA_2x_SOC)
#include "pinmux-t2.h"
#elif defined(CONFIG_ARCH_TEGRA_3x_SOC)
#include "pinmux-t3.h"
#else
#error "Undefined Tegra architecture"
#endif

#define TEGRA_MUX_LIST \
	TEGRA_MUX(NONE) \
	TEGRA_MUX(AHB_CLK) \
	TEGRA_MUX(APB_CLK) \
	TEGRA_MUX(AUDIO_SYNC) \
	TEGRA_MUX(CRT) \
	TEGRA_MUX(DAP1) \
	TEGRA_MUX(DAP2) \
	TEGRA_MUX(DAP3) \
	TEGRA_MUX(DAP4) \
	TEGRA_MUX(DAP5) \
	TEGRA_MUX(DISPLAYA) \
	TEGRA_MUX(DISPLAYB) \
	TEGRA_MUX(EMC_TEST0_DLL) \
	TEGRA_MUX(EMC_TEST1_DLL) \
	TEGRA_MUX(GMI) \
	TEGRA_MUX(GMI_INT) \
	TEGRA_MUX(HDMI) \
	TEGRA_MUX(I2C1) \
	TEGRA_MUX(I2C2) \
	TEGRA_MUX(I2C3) \
	TEGRA_MUX(IDE) \
	TEGRA_MUX(IRDA) \
	TEGRA_MUX(KBC) \
	TEGRA_MUX(MIO) \
	TEGRA_MUX(MIPI_HS) \
	TEGRA_MUX(NAND) \
	TEGRA_MUX(OSC) \
	TEGRA_MUX(OWR) \
	TEGRA_MUX(PCIE) \
	TEGRA_MUX(PLLA_OUT) \
	TEGRA_MUX(PLLC_OUT1) \
	TEGRA_MUX(PLLM_OUT1) \
	TEGRA_MUX(PLLP_OUT2) \
	TEGRA_MUX(PLLP_OUT3) \
	TEGRA_MUX(PLLP_OUT4) \
	TEGRA_MUX(PWM) \
	TEGRA_MUX(PWR_INTR) \
	TEGRA_MUX(PWR_ON) \
	TEGRA_MUX(RTCK) \
	TEGRA_MUX(SDIO1) \
	TEGRA_MUX(SDIO2) \
	TEGRA_MUX(SDIO3) \
	TEGRA_MUX(SDIO4) \
	TEGRA_MUX(SFLASH) \
	TEGRA_MUX(SPDIF) \
	TEGRA_MUX(SPI1) \
	TEGRA_MUX(SPI2) \
	TEGRA_MUX(SPI2_ALT) \
	TEGRA_MUX(SPI3) \
	TEGRA_MUX(SPI4) \
	TEGRA_MUX(TRACE) \
	TEGRA_MUX(TWC) \
	TEGRA_MUX(UARTA) \
	TEGRA_MUX(UARTB) \
	TEGRA_MUX(UARTC) \
	TEGRA_MUX(UARTD) \
	TEGRA_MUX(UARTE) \
	TEGRA_MUX(ULPI) \
	TEGRA_MUX(VI) \
	TEGRA_MUX(VI_SENSOR_CLK) \
	TEGRA_MUX(XIO) \
	/* End of Tegra2 MUX selectors */ \
	TEGRA_MUX(BLINK) \
	TEGRA_MUX(CEC) \
	TEGRA_MUX(CLK12) \
	TEGRA_MUX(DAP) \
	TEGRA_MUX(DAPSDMMC2) \
	TEGRA_MUX(DDR) \
	TEGRA_MUX(DEV3) \
	TEGRA_MUX(DTV) \
	TEGRA_MUX(VI_ALT1) \
	TEGRA_MUX(VI_ALT2) \
	TEGRA_MUX(VI_ALT3) \
	TEGRA_MUX(EMC_DLL) \
	TEGRA_MUX(EXTPERIPH1) \
	TEGRA_MUX(EXTPERIPH2) \
	TEGRA_MUX(EXTPERIPH3) \
	TEGRA_MUX(GMI_ALT) \
	TEGRA_MUX(HDA) \
	TEGRA_MUX(HSI) \
	TEGRA_MUX(I2C4) \
	TEGRA_MUX(I2C5) \
	TEGRA_MUX(I2CPWR) \
	TEGRA_MUX(I2S0) \
	TEGRA_MUX(I2S1) \
	TEGRA_MUX(I2S2) \
	TEGRA_MUX(I2S3) \
	TEGRA_MUX(I2S4) \
	TEGRA_MUX(NAND_ALT) \
	TEGRA_MUX(POPSDIO4) \
	TEGRA_MUX(POPSDMMC4) \
	TEGRA_MUX(PWM0) \
	TEGRA_MUX(PWM1) \
	TEGRA_MUX(PWM2) \
	TEGRA_MUX(PWM3) \
	TEGRA_MUX(SATA) \
	TEGRA_MUX(SPI5) \
	TEGRA_MUX(SPI6) \
	TEGRA_MUX(SYSCLK) \
	TEGRA_MUX(VGP1) \
	TEGRA_MUX(VGP2) \
	TEGRA_MUX(VGP3) \
	TEGRA_MUX(VGP4) \
	TEGRA_MUX(VGP5) \
	TEGRA_MUX(VGP6) \
	/* End of Tegra3 MUX selectors */

enum tegra_mux_func {
	TEGRA_MUX_RSVD = 0x8000,
	TEGRA_MUX_RSVD0 = TEGRA_MUX_RSVD,
	TEGRA_MUX_RSVD1 = 0x8000,
	TEGRA_MUX_RSVD2 = 0x8001,
	TEGRA_MUX_RSVD3 = 0x8002,
	TEGRA_MUX_RSVD4 = 0x8003,
	TEGRA_MUX_INVALID = 0x4000,
	TEGRA_MUX_NONE = -1/*0*/,
	TEGRA_MUX_AHB_CLK,
	TEGRA_MUX_APB_CLK,
	TEGRA_MUX_AUDIO_SYNC,
	TEGRA_MUX_CRT,
	TEGRA_MUX_DAP1,
	TEGRA_MUX_DAP2,
	TEGRA_MUX_DAP3,
	TEGRA_MUX_DAP4,
	TEGRA_MUX_DAP5,
	TEGRA_MUX_DISPLAYA,
	TEGRA_MUX_DISPLAYB,
	TEGRA_MUX_EMC_TEST0_DLL,
	TEGRA_MUX_EMC_TEST1_DLL,
	TEGRA_MUX_GMI,
	TEGRA_MUX_GMI_INT,
	TEGRA_MUX_HDMI,
	TEGRA_MUX_I2C,
	TEGRA_MUX_I2C1 = TEGRA_MUX_I2C,
	TEGRA_MUX_I2C2,
	TEGRA_MUX_I2C3,
	TEGRA_MUX_IDE,
	TEGRA_MUX_IRDA,
	TEGRA_MUX_KBC,
	TEGRA_MUX_MIO,
	TEGRA_MUX_MIPI_HS,
	TEGRA_MUX_NAND,
	TEGRA_MUX_OSC,
	TEGRA_MUX_OWR,
	TEGRA_MUX_PCIE,
	TEGRA_MUX_PLLA_OUT,
	TEGRA_MUX_PLLC_OUT1,
	TEGRA_MUX_PLLM_OUT1,
	TEGRA_MUX_PLLP_OUT2,
	TEGRA_MUX_PLLP_OUT3,
	TEGRA_MUX_PLLP_OUT4,
	TEGRA_MUX_PWM,
	TEGRA_MUX_PWR_INTR,
	TEGRA_MUX_PWR_ON,
	TEGRA_MUX_RTCK,
	TEGRA_MUX_SDIO1,
	TEGRA_MUX_SDMMC1 = TEGRA_MUX_SDIO1,
	TEGRA_MUX_SDIO2,
	TEGRA_MUX_SDMMC2 = TEGRA_MUX_SDIO2,
	TEGRA_MUX_SDIO3,
	TEGRA_MUX_SDMMC3 = TEGRA_MUX_SDIO3,
	TEGRA_MUX_SDIO4,
	TEGRA_MUX_SDMMC4 = TEGRA_MUX_SDIO4,
	TEGRA_MUX_SFLASH,
	TEGRA_MUX_SPDIF,
	TEGRA_MUX_SPI1,
	TEGRA_MUX_SPI2,
	TEGRA_MUX_SPI2_ALT,
	TEGRA_MUX_SPI3,
	TEGRA_MUX_SPI4,
	TEGRA_MUX_TRACE,
	TEGRA_MUX_TWC,
	TEGRA_MUX_UARTA,
	TEGRA_MUX_UARTB,
	TEGRA_MUX_UARTC,
	TEGRA_MUX_UARTD,
	TEGRA_MUX_UARTE,
	TEGRA_MUX_ULPI,
	TEGRA_MUX_VI,
	TEGRA_MUX_VI_SENSOR_CLK,
	TEGRA_MUX_XIO,
#ifndef CONFIG_ARCH_TEGRA_2x_SOC
	TEGRA_MUX_BLINK,
	TEGRA_MUX_CEC,
	TEGRA_MUX_CLK12,
	TEGRA_MUX_DAP,
	TEGRA_MUX_DAPSDMMC2,
	TEGRA_MUX_DDR,
	TEGRA_MUX_DEV3,
	TEGRA_MUX_DTV,
	TEGRA_MUX_VI_ALT1,
	TEGRA_MUX_VI_ALT2,
	TEGRA_MUX_VI_ALT3,
	TEGRA_MUX_EMC_DLL,
	TEGRA_MUX_EXTPERIPH1,
	TEGRA_MUX_EXTPERIPH2,
	TEGRA_MUX_EXTPERIPH3,
	TEGRA_MUX_GMI_ALT,
	TEGRA_MUX_HDA,
	TEGRA_MUX_HSI,
	TEGRA_MUX_I2C4,
	TEGRA_MUX_I2C5,
	TEGRA_MUX_I2CPWR,
	TEGRA_MUX_I2S0,
	TEGRA_MUX_I2S1,
	TEGRA_MUX_I2S2,
	TEGRA_MUX_I2S3,
	TEGRA_MUX_I2S4,
	TEGRA_MUX_NAND_ALT,
	TEGRA_MUX_POPSDIO4,
	TEGRA_MUX_POPSDMMC4,
	TEGRA_MUX_PWM0,
	TEGRA_MUX_PWM1,
	TEGRA_MUX_PWM2,
	TEGRA_MUX_PWM3,
	TEGRA_MUX_SATA,
	TEGRA_MUX_SPI5,
	TEGRA_MUX_SPI6,
	TEGRA_MUX_SYSCLK,
	TEGRA_MUX_VGP1,
	TEGRA_MUX_VGP2,
	TEGRA_MUX_VGP3,
	TEGRA_MUX_VGP4,
	TEGRA_MUX_VGP5,
	TEGRA_MUX_VGP6,
#endif
	TEGRA_MUX_SAFE,
	TEGRA_MAX_MUX,
};

enum tegra_pullupdown {
	TEGRA_PUPD_NORMAL = 0,
	TEGRA_PUPD_PULL_DOWN,
	TEGRA_PUPD_PULL_UP,
};

enum tegra_tristate {
	TEGRA_TRI_NORMAL = 0,
	TEGRA_TRI_TRISTATE = 1,
};

enum tegra_pin_io {
	TEGRA_PIN_OUTPUT = 0,
	TEGRA_PIN_INPUT = 1,
};

enum tegra_pin_lock {
	TEGRA_PIN_LOCK_DEFAULT = 0,
	TEGRA_PIN_LOCK_DISABLE,
	TEGRA_PIN_LOCK_ENABLE,
};

enum tegra_pin_od {
	TEGRA_PIN_OD_DEFAULT = 0,
	TEGRA_PIN_OD_DISABLE,
	TEGRA_PIN_OD_ENABLE,
};

enum tegra_pin_ioreset {
	TEGRA_PIN_IO_RESET_DEFAULT = 0,
	TEGRA_PIN_IO_RESET_DISABLE,
	TEGRA_PIN_IO_RESET_ENABLE,
};

enum tegra_vddio {
	TEGRA_VDDIO_BB = 0,
	TEGRA_VDDIO_LCD,
	TEGRA_VDDIO_VI,
	TEGRA_VDDIO_UART,
	TEGRA_VDDIO_DDR,
	TEGRA_VDDIO_NAND,
	TEGRA_VDDIO_SYS,
	TEGRA_VDDIO_AUDIO,
	TEGRA_VDDIO_SD,
#ifndef CONFIG_ARCH_TEGRA_2x_SOC
	TEGRA_VDDIO_CAM,
	TEGRA_VDDIO_GMI,
	TEGRA_VDDIO_PEXCTL,
	TEGRA_VDDIO_SDMMC1,
	TEGRA_VDDIO_SDMMC3,
	TEGRA_VDDIO_SDMMC4,
#endif
};

struct tegra_pingroup_config {
	enum tegra_pingroup	pingroup;
	enum tegra_mux_func	func;
	enum tegra_pullupdown	pupd;
	enum tegra_tristate	tristate;
	enum tegra_pin_io	io;
	enum tegra_pin_lock	lock;
	enum tegra_pin_od	od;
	enum tegra_pin_ioreset	ioreset;
};

enum tegra_slew {
	TEGRA_SLEW_FASTEST = 0,
	TEGRA_SLEW_FAST,
	TEGRA_SLEW_SLOW,
	TEGRA_SLEW_SLOWEST,
	TEGRA_MAX_SLEW,
};

enum tegra_pull_strength {
	TEGRA_PULL_0 = 0,
	TEGRA_PULL_1,
	TEGRA_PULL_2,
	TEGRA_PULL_3,
	TEGRA_PULL_4,
	TEGRA_PULL_5,
	TEGRA_PULL_6,
	TEGRA_PULL_7,
	TEGRA_PULL_8,
	TEGRA_PULL_9,
	TEGRA_PULL_10,
	TEGRA_PULL_11,
	TEGRA_PULL_12,
	TEGRA_PULL_13,
	TEGRA_PULL_14,
	TEGRA_PULL_15,
	TEGRA_PULL_16,
	TEGRA_PULL_17,
	TEGRA_PULL_18,
	TEGRA_PULL_19,
	TEGRA_PULL_20,
	TEGRA_PULL_21,
	TEGRA_PULL_22,
	TEGRA_PULL_23,
	TEGRA_PULL_24,
	TEGRA_PULL_25,
	TEGRA_PULL_26,
	TEGRA_PULL_27,
	TEGRA_PULL_28,
	TEGRA_PULL_29,
	TEGRA_PULL_30,
	TEGRA_PULL_31,
	TEGRA_PULL_32,
	TEGRA_PULL_33,
	TEGRA_PULL_34,
	TEGRA_PULL_35,
	TEGRA_PULL_36,
	TEGRA_PULL_37,
	TEGRA_PULL_38,
	TEGRA_PULL_39,
	TEGRA_PULL_40,
	TEGRA_PULL_41,
	TEGRA_PULL_42,
	TEGRA_PULL_43,
	TEGRA_PULL_44,
	TEGRA_PULL_45,
	TEGRA_PULL_46,
	TEGRA_MAX_PULL,
};

enum tegra_drive {
	TEGRA_DRIVE_DIV_8 = 0,
	TEGRA_DRIVE_DIV_4,
	TEGRA_DRIVE_DIV_2,
	TEGRA_DRIVE_DIV_1,
	TEGRA_MAX_DRIVE,
};

enum tegra_hsm {
	TEGRA_HSM_DISABLE = 0,
	TEGRA_HSM_ENABLE,
};

enum tegra_schmitt {
	TEGRA_SCHMITT_DISABLE = 0,
	TEGRA_SCHMITT_ENABLE,
};

struct tegra_drive_pingroup_config {
	enum tegra_drive_pingroup pingroup;
	enum tegra_hsm hsm;
	enum tegra_schmitt schmitt;
	enum tegra_drive drive;
	enum tegra_pull_strength pull_down;
	enum tegra_pull_strength pull_up;
	enum tegra_slew slew_rising;
	enum tegra_slew slew_falling;
};

struct tegra_drive_pingroup_desc {
	const char *name;
	s16 reg;
	u8 drvup_offset;
	u16 drvup_mask;
	u8 drvdown_offset;
	u16 drvdown_mask;
	u8 slewrise_offset;
	u16 slewrise_mask;
	u8 slewfall_offset;
	u16 slewfall_mask;
};

struct tegra_pingroup_desc {
	const char *name;
	int funcs[4];
	int func_safe;
	int vddio;
	s16 tri_reg;	/* offset into the TRISTATE_REG_* register bank */
	s16 mux_reg;	/* offset into the PIN_MUX_CTL_* register bank */
	s16 pupd_reg;	/* offset into the PULL_UPDOWN_REG_* register bank */
	s8 tri_bit;	/* offset into the TRISTATE_REG_* register bit */
	s8 mux_bit;	/* offset into the PIN_MUX_CTL_* register bit */
	s8 pupd_bit;	/* offset into the PULL_UPDOWN_REG_* register bit */
	s8 lock_bit;	/* offser of the LOCK bit into mux register bit */
	s8 od_bit;	/* offset of the OD bit into mux register bit */
	s8 ioreset_bit;	/* offset of the IO_RESET bit into mux register bit */
	s8 io_default;
	int gpionr;
};

extern const struct tegra_pingroup_desc tegra_soc_pingroups[];
extern const struct tegra_drive_pingroup_desc tegra_soc_drive_pingroups[];
extern const int gpio_to_pingroup[];

int tegra_pinmux_get_func(enum tegra_pingroup pg);
int tegra_pinmux_set_func(const struct tegra_pingroup_config *config);
int tegra_pinmux_set_tristate(enum tegra_pingroup pg,
	enum tegra_tristate tristate);
int tegra_pinmux_set_io(enum tegra_pingroup pg,
	enum tegra_pin_io input);
int tegra_pinmux_get_pingroup(int gpio_nr);
int tegra_pinmux_set_pullupdown(enum tegra_pingroup pg,
	enum tegra_pullupdown pupd);

void tegra_pinmux_config_table(const struct tegra_pingroup_config *config,
	int len);

void tegra_drive_pinmux_config_table(struct tegra_drive_pingroup_config *config,
	int len);
void tegra_pinmux_set_safe_pinmux_table(const struct tegra_pingroup_config *config,
	int len);
void tegra_pinmux_config_pinmux_table(const struct tegra_pingroup_config *config,
	int len);
void tegra_pinmux_config_tristate_table(const struct tegra_pingroup_config *config,
	int len, enum tegra_tristate tristate);
void tegra_pinmux_config_pullupdown_table(const struct tegra_pingroup_config *config,
	int len, enum tegra_pullupdown pupd);

void __init tegra_init_pinmux(void);
#endif
