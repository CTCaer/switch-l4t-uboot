/*
 * Copyright (c) 2022, CTCaer.
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

/*
 * THIS FILE IS AUTO-GENERATED - DO NOT EDIT!
 *
 * To generate this file, use the tegra-pinmux-scripts tool available from
 * https://github.com/NVIDIA/tegra-pinmux-scripts
 * Run "board-to-uboot.py p2371-2180".
 */

#ifndef _PINMUX_CONFIG_SWITCH_H_
#define _PINMUX_CONFIG_SWITCH_H_

#define GPIO_INIT(_port, _gpio, _init)			\
	{						\
		.gpio	= TEGRA_GPIO(_port, _gpio),	\
		.init	= TEGRA_GPIO_INIT_##_init,	\
	}

static const struct tegra_gpio_config nintendo_switch_gpio_inits[] = {
	/*        port, pin, init_val */

	// UARTB TX
	GPIO_INIT(G, 0, SFIO),

	// UARTC TX
	GPIO_INIT(D, 1, SFIO),

	// SD card power
	GPIO_INIT(E, 4, OUT0),
};

#define PINCFG(_pingrp, _mux, _pull, _tri, _io, _od, _e_io_hv, _schmt)	\
	{								\
		.pingrp		= PMUX_PINGRP_##_pingrp,		\
		.func		= PMUX_FUNC_##_mux,			\
		.pull		= PMUX_PULL_##_pull,			\
		.tristate	= PMUX_TRI_##_tri,			\
		.io		= PMUX_PIN_##_io,			\
		.od		= PMUX_PIN_OD_##_od,			\
		.e_io_hv	= PMUX_PIN_E_IO_HV_##_e_io_hv,		\
		.schmt		= PMUX_SCHMT_##_schmt,			\
		.lock		= PMUX_PIN_LOCK_DEFAULT,		\
	}

static const struct pmux_pingrp_config nintendo_switch_pingrps[] = {
	/*     pingrp,               mux,        pull,   tri,      e_input, od,      e_io_hv  schmt */

	// UARTA
	PINCFG(UART1_TX_PU0,         UARTA,      NORMAL, NORMAL,   OUTPUT,  DISABLE, DEFAULT, NONE),
	PINCFG(UART1_RX_PU1,         UARTA,      NORMAL, TRISTATE, INPUT,   DISABLE, DEFAULT, NONE),
	PINCFG(UART1_RTS_PU2,        UARTA,      NORMAL, NORMAL,   OUTPUT,  DISABLE, DEFAULT, NONE),
	PINCFG(UART1_CTS_PU3,        UARTA,      NORMAL, TRISTATE, INPUT,   DISABLE, DEFAULT, NONE),

	// UARTB
	PINCFG(UART2_TX_PG0,         UARTB,      NORMAL, NORMAL,   OUTPUT,  DISABLE, DEFAULT, NONE),
	PINCFG(UART2_RX_PG1,         UARTB,      NORMAL, TRISTATE, INPUT,   DISABLE, DEFAULT, NONE),
	PINCFG(UART2_RTS_PG2,        UARTB,      NORMAL, NORMAL,   OUTPUT,  DISABLE, DEFAULT, NONE),
	PINCFG(UART2_CTS_PG3,        UARTB,      NORMAL, TRISTATE, INPUT,   DISABLE, DEFAULT, NONE),

	// UARTC
	PINCFG(UART3_TX_PD1,         UARTC,      NORMAL, NORMAL,   OUTPUT,  DISABLE, DEFAULT, NONE),
	PINCFG(UART3_RX_PD2,         UARTC,      NORMAL, TRISTATE, INPUT,   DISABLE, DEFAULT, NONE),
	PINCFG(UART3_RTS_PD3,        UARTC,      NORMAL, NORMAL,   OUTPUT,  DISABLE, DEFAULT, NONE),
	PINCFG(UART3_CTS_PD4,        UARTC,      NORMAL, TRISTATE, INPUT,   DISABLE, DEFAULT, NONE),

	// I2C
	PINCFG(GEN1_I2C_SDA_PJ0,     I2C1,       NORMAL, NORMAL,   INPUT,   DISABLE, NORMAL,  NONE),
	PINCFG(GEN1_I2C_SCL_PJ1,     I2C1,       NORMAL, NORMAL,   INPUT,   DISABLE, NORMAL,  NONE),
	PINCFG(PWR_I2C_SCL_PY3,      I2CPMU,     NORMAL, NORMAL,   INPUT,   DISABLE, NORMAL,  NONE),
	PINCFG(PWR_I2C_SDA_PY4,      I2CPMU,     NORMAL, NORMAL,   INPUT,   DISABLE, NORMAL,  NONE),
};

static const struct pmux_pingrp_config nintendo_switch_sd_t210_pingrps[] = {
	/*     pingrp,               mux,        pull,   tri,      e_input, od,      e_io_hv  schmt */

	// SDMMC1
	PINCFG(SDMMC1_CLK_PM0,       SDMMC1,     NORMAL, NORMAL,   INPUT,   DISABLE, DEFAULT, DISABLE),
	PINCFG(SDMMC1_CMD_PM1,       SDMMC1,     UP,     NORMAL,   INPUT,   DISABLE, DEFAULT, DISABLE),
	PINCFG(SDMMC1_DAT3_PM2,      SDMMC1,     UP,     NORMAL,   INPUT,   DISABLE, DEFAULT, DISABLE),
	PINCFG(SDMMC1_DAT2_PM3,      SDMMC1,     UP,     NORMAL,   INPUT,   DISABLE, DEFAULT, DISABLE),
	PINCFG(SDMMC1_DAT1_PM4,      SDMMC1,     UP,     NORMAL,   INPUT,   DISABLE, DEFAULT, DISABLE),
	PINCFG(SDMMC1_DAT0_PM5,      SDMMC1,     UP,     NORMAL,   INPUT,   DISABLE, DEFAULT, DISABLE),
	// Card detect
	PINCFG(PZ1,                  RSVD2,      UP,     NORMAL,   INPUT,   DISABLE, DEFAULT, NONE),
	// Card power
	PINCFG(DMIC3_CLK_PE4,        RSVD2,      DOWN,   NORMAL,   OUTPUT,  DISABLE, DEFAULT, NONE),
};

static const struct pmux_pingrp_config nintendo_switch_sd_t210b01_pingrps[] = {
	/*     pingrp,               mux,        pull,   tri,      e_input, od,      e_io_hv  schmt */

	// SDMMC1
	PINCFG(SDMMC1_CLK_PM0,       SDMMC1,     NORMAL, NORMAL,   INPUT,   DISABLE, DEFAULT, ENABLE),
	PINCFG(SDMMC1_CMD_PM1,       SDMMC1,     UP,     NORMAL,   INPUT,   DISABLE, DEFAULT, ENABLE),
	PINCFG(SDMMC1_DAT3_PM2,      SDMMC1,     UP,     NORMAL,   INPUT,   DISABLE, DEFAULT, ENABLE),
	PINCFG(SDMMC1_DAT2_PM3,      SDMMC1,     UP,     NORMAL,   INPUT,   DISABLE, DEFAULT, ENABLE),
	PINCFG(SDMMC1_DAT1_PM4,      SDMMC1,     UP,     NORMAL,   INPUT,   DISABLE, DEFAULT, ENABLE),
	PINCFG(SDMMC1_DAT0_PM5,      SDMMC1,     UP,     NORMAL,   INPUT,   DISABLE, DEFAULT, ENABLE),
	// Card detect
	PINCFG(PZ1,                  RSVD2,      UP,     NORMAL,   INPUT,   DISABLE, DEFAULT, NONE),
	// Card power
	PINCFG(DMIC3_CLK_PE4,        RSVD2,      DOWN,   NORMAL,   OUTPUT,  DISABLE, DEFAULT, NONE),
};

#endif /* PINMUX_CONFIG_SWITCH_H */
