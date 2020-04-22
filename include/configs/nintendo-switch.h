/*
 * (C) Copyright 2013-2015
 * NVIDIA Corporation <www.nvidia.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef _NINTENDO_SWITCH_H
#define _NINTENDO_SWITCH_H

#include <linux/sizes.h>

#define BOARD_EXTRA_ENV_SETTINGS \
	"boot_prefixes=/ /switchroot/android/\0"

#include "tegra210-common.h"

#define CONFIG_REMAKE_ELF

/* High-level configuration options */
#define CONFIG_TEGRA_BOARD_STRING	"Nintendo Switch"

/* Board-specific serial config */
#define CONFIG_TEGRA_ENABLE_UARTB

/* I2C */
#define CONFIG_SYS_I2C_TEGRA

/* USB2.0 Host support */
#define CONFIG_USB_EHCI_TEGRA

/* USB networking support */

/* PCI host support */

/* General networking support */

#include "tegra-common-usb-gadget.h"
#include "tegra-common-post.h"

/* Crystal is 38.4MHz. clk_m runs at half that rate */
#define COUNTER_FREQUENCY	19200000

#define CONFIG_SYS_MEM_RESERVE_SECURE (4 * SZ_1M)

#endif /* _NINTENDO_SWITCH_H */
