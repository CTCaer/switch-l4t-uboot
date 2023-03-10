/*
 * (C) Copyright 2013-2015
 * NVIDIA Corporation <www.nvidia.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef _TEGRA210_COMMON_H_
#define _TEGRA210_COMMON_H_

#include "tegra-common.h"

/*
 * NS16550 Configuration
 */
#define V_NS16550_CLK		408000000	/* 408MHz (pllp_out0) */

/*
 * Miscellaneous configurable options
 */
#define CONFIG_STACKBASE    0x8A800000  /* 168MB */

/*-----------------------------------------------------------------------
 * Physical Memory Map
 */
#define CONFIG_SYS_TEXT_BASE	0x88110000

/* Generic Interrupt Controller */
#define CONFIG_GICV2

/*
 * Memory layout for where various images get loaded by boot scripts:
 *
 * scriptaddr can be pretty much anywhere that doesn't conflict with something
 *   else. Put it above BOOTMAPSZ to eliminate conflicts.
 *
 * pxefile_addr_r can be pretty much anywhere that doesn't conflict with
 *   something else. Put it above BOOTMAPSZ to eliminate conflicts.
 *
 * kernel_addr_r must be within the first 128M of RAM in order for the
 *   kernel's CONFIG_AUTO_ZRELADDR option to work. Since the kernel will
 *   decompress itself to 0x8000 after the start of RAM, kernel_addr_r
 *   should not overlap that area, or the kernel will have to copy itself
 *   somewhere else before decompression. Similarly, the address of any other
 *   data passed to the kernel shouldn't overlap the start of RAM. Pushing
 *   this up to 64M allows for a sizable kernel to be decompressed below the
 *   compressed load address.
 *
 * fdt_addr_r simply shouldn't overlap anything else.
 *
 * ramdisk_addr_r simply shouldn't overlap anything else.
 */
#define CONFIG_LOADADDR 0x84000000
#define MEM_LAYOUT_ENV_SETTINGS \
	"kernel_addr_r=" __stringify(CONFIG_LOADADDR) "\0" \
	"fdt_addr_r=0x8d000000\0" \
	"scriptaddr=0x8fe00000\0" \
	"pxefile_addr_r=0x8ff00000\0" \
	"ramdisk_addr_r=0x92000000\0" \
    "stdin=serial\0" \
    "stdout=serial\0" \
    "stderr=serial,vidconsole\0"

/* Defines for SPL */
#define CONFIG_SPL_TEXT_BASE		0x88108000
#define CONFIG_SYS_SPL_MALLOC_START	0x88090000
#define CONFIG_SPL_STACK		0x880ffffc

/* For USB EHCI controller */
#define CONFIG_EHCI_IS_TDI
#define CONFIG_USB_EHCI_TXFIFO_THRESH	0x10

/* GPU needs setup */
#define CONFIG_TEGRA_GPU

#endif /* _TEGRA210_COMMON_H_ */
