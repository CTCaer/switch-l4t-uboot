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
	"bootdir=/switchroot/android\0" \
	"bootenv=uEnv.txt\0" \
	"default_target=" \
		"setenv variant icosa; " \
		"setenv mmcdev 1; " \
		"setenv bootpart 0; " \
		"mmc dev ${mmcdev};\0" \
	"emmc_target=" \
		"setenv variant icosa_emmc; " \
		"setenv mmcdev 0; " \
		"mmc dev ${mmcdev};\0" \
	"emmc_overlay=" \
		"fdt set / model icosa_emmc; " \
		"fdt set /firmware/android hardware icosa_emmc; " \
		"fdt set /firmware/android/fstab/vendor dev /dev/block/platform/sdhci-tegra.3/by-name/vendor; " \
		"fdt set /sdhci@700b0600 status okay;\0" \
	"uart_overlay=" \
		"fdt set /serial@70006040 compatible nvidia,tegra20-uart; " \
		"fdt set /serial@70006040/joyconr status disabled; " \
		"fdt set /joyconr_charger status disabled; " \
		"fdt set /pinmux@700008d4/common/pk3 nvidia,open-drain <0x1>; " \
		"fdt set /pinmux@700008d4/common/ph6 nvidia,open-drain <0x1>;\0" \
	"loadbootenv=" \
		"fatload mmc ${mmcdev} ${scriptaddr} ${bootdir}/${bootenv};\0" \
	"importbootenv=" \
		"echo Importing environment from mmc${mmcdev} ...; " \
		"env import -t ${scriptaddr} ${filesize}\0" \
	"get_fdt=" \
		"part start mmc $mmcdev DTB dtb_start; " \
		"part size mmc $mmcdev DTB dtb_size; " \
		"mmc read $fdt_addr_r $dtb_start $dtb_size; " \
		"fdt addr $fdt_addr_r;\0" \
	"bootcmd_common=" \
		"setenv bootargs \"init=/init nvdec_enabled=0 pcie_aspm=off vpr_resize tegra_fbmem=0x800000@0xf5a00000\"; " \
		"setenv bootargs \"${bootargs} firmware_class.path=/vendor/firmware pmc_reboot2payload.default_payload=reboot_payload.bin pmc_reboot2payload.reboot_action=via-payload pmc_reboot2payload.hekate_config_id=SWANDR pmc_reboot2payload.enabled=1\"; " \
		"if test -n $useemmc; then " \
			"run emmc_target; " \
		"fi; " \
		"run get_fdt; " \
		"if test -n $useemmc; then " \
			"run emmc_overlay; " \
		"fi; " \
		"if test -n $enableuart; then " \
			"run uart_overlay; " \
			"setenv bootargs \"${bootargs} no_console_suspend=1 androidboot.console=ttyS1 console=ttyS1,115200,8n1\"; " \
		"else " \
			"setenv bootargs \"${bootargs} console=null\"; " \
		"fi; " \
		"mmc info serial#; " \
		"setenv bootargs \"${bootargs} androidboot.bootloader=${blver} androidboot.hardware=${variant} androidboot.serialno=${serial#} androidboot.modem=none\";\0" \
	"bootcmd_android=" \
		"setenv boot_staging 0x98000000; " \
		"part number mmc ${mmcdev} APP app_part_num; " \
		"fsuuid mmc ${mmcdev}:${app_part_num} app_part_uuid; " \
		"part start mmc ${mmcdev} LNX lnx_start; " \
		"part size mmc ${mmcdev} LNX lnx_size; " \
		"mmc read $boot_staging $lnx_start $lnx_size; " \
		"setenv bootargs \"skip_initramfs ro rootwait root=PARTUUID=${app_part_uuid} ${bootargs} bluetooth.disable_ertm=1\"; " \
		"bootm $boot_staging $boot_staging $fdt_addr_r;\0" \
	"bootcmd_recovery=" \
		"setenv recovery_staging 0x98000000; " \
		"part start mmc ${mmcdev} SOS recovery_start; " \
		"part size mmc ${mmcdev} SOS recovery_size; " \
		"mmc read $recovery_staging $recovery_start $recovery_size; " \
		"bootm $recovery_staging $recovery_staging $fdt_addr_r;\0" \
	"bootcmd=" \
		"run default_target; " \
		"run loadbootenv && run importbootenv; " \
		"if test -n $uenvcmd; then " \
			"echo Running uenvcmd ...; " \
			"run uenvcmd; " \
		"else " \
			"run bootcmd_common; " \
			"if gpio input 190 || test ${recovery} = \"1\"; then " \
				"run bootcmd_recovery; " \
			"else " \
				"run bootcmd_android; " \
			"fi; " \
		"fi;\0"

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
