/*
 * (C) Copyright 2013-2015
 * NVIDIA Corporation <www.nvidia.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef _ICOSA_H
#define _ICOSA_H

#include <linux/sizes.h>

#define BOARD_EXTRA_ENV_SETTINGS \
	"boot_prefixes=/ /switchroot/android/\0"

#include "tegra210-common.h"

/* High-level configuration options */
#define CONFIG_TEGRA_BOARD_STRING	"Nintendo Switch"

/* Board-specific serial config */
#define CONFIG_TEGRA_ENABLE_UARTB

#include "tegra-common-usb-gadget.h"
#include "tegra-common-post.h"

/* Crystal is 38.4MHz. clk_m runs at half that rate */
#define COUNTER_FREQUENCY	19200000

#define CONFIG_SYS_BOOTM_LEN   SZ_64M

#endif /* _ICOSA_H */
