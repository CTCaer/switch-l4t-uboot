/*
 * (C) Copyright 2022-2023, CTCaer.
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <i2c.h>
#include <asm/io.h>
#include <asm/arch/gpio.h>
#include <asm/arch/pinmux.h>
#include <asm/arch/tegra.h>
#include <asm/arch-tegra/ap.h>
#include <asm/arch-tegra/pmc.h>
#include <asm/arch-tegra/gp_padctrl.h>
#include "../../nvidia/p2571/max77620_init.h"
#include "pinmux-config-nintendo-switch.h"

#define FUSE_BASE                   0x7000F800
#define FUSE_RESERVED_ODMX(x)       (0x1C8 + 4 * (x))
#define FUSE_OPT_LOT_CODE_0         0x208
#define FUSE_OPT_WAFER_ID           0x210
#define FUSE_OPT_X_COORDINATE       0x214
#define FUSE_OPT_Y_COORDINATE       0x218
#define FUSE_RESERVED_ODM28_T210B01 0x240

enum {
	NX_HW_TYPE_ODIN,
	NX_HW_TYPE_MODIN,
	NX_HW_TYPE_VALI,
	NX_HW_TYPE_FRIG
};

static int get_sku(void)
{
	const volatile void __iomem *odm4 = (void *)(FUSE_BASE + FUSE_RESERVED_ODMX(4));

	if (tegra_get_chip_rev() == MAJORPREV_TEGRA210B01)
	{
		switch ((readl(odm4) & 0xF0000) >> 16)
		{
		case 2:
			return NX_HW_TYPE_VALI;

		case 4:
			return NX_HW_TYPE_FRIG;

		case 1:
		default:
			return NX_HW_TYPE_MODIN;
		}
	}

	return NX_HW_TYPE_ODIN;
}

static void set_pmic_type(void)
{
	const volatile void __iomem *rsvd_odm28 =
				    (void *)(FUSE_BASE + FUSE_RESERVED_ODM28_T210B01);

	u32 odm28 = readl(rsvd_odm28);
	if (odm28 & 1)
		env_set("pmic_type", "0"); /* 0x33 211 phase config (retail) */
	else
		env_set("pmic_type", "1"); /* 0x31 31  phase config (devboard) */
}

static void generate_and_set_serial(void)
{
	const volatile void __iomem *opt_lot0 =
				    (void *)(FUSE_BASE + FUSE_OPT_LOT_CODE_0);
	const volatile void __iomem *opt_wafer =
				    (void *)(FUSE_BASE + FUSE_OPT_WAFER_ID);
	const volatile void __iomem *opt_x =
				    (void *)(FUSE_BASE + FUSE_OPT_X_COORDINATE);
	const volatile void __iomem *opt_y =
				    (void *)(FUSE_BASE + FUSE_OPT_Y_COORDINATE);
	u32 lot0 = readl(opt_lot0);
	u32 wfxy = (readl(opt_wafer) << 18) | (readl(opt_x) << 9) | readl(opt_y);
	char buf[32];

	/* Generate serial number */
	switch (get_sku()) {
	case NX_HW_TYPE_MODIN:
		sprintf(buf, "NXM-%08X-%06X", (~lot0) & 0x3FFFFFFF, wfxy);
		break;

	case NX_HW_TYPE_VALI:
		sprintf(buf, "NXV-%08X-%06X", (~lot0) & 0x3FFFFFFF, wfxy);
		break;

	case NX_HW_TYPE_FRIG:
		sprintf(buf, "NXF-%08X-%06X", (~lot0) & 0x3FFFFFFF, wfxy);
		break;

	case NX_HW_TYPE_ODIN:
	default:
		sprintf(buf, "NXO-%08X-%06X", (~lot0) & 0x3FFFFFFF, wfxy);
		break;
	}

	/* Set serial number to env */
	env_set("device_serial", buf);

	/* Generate default bluetooth mac address and set it to env */
	sprintf(buf, "98:B6:E9:%02X:%02X:%02X",
		(lot0 >> 16) & 0xFF, (lot0 >> 8) & 0xFF, (lot0 + 1) & 0xFF);
	env_set("device_bt_mac", buf);

	/* Generate default wifi mac address and set it to env */
	sprintf(buf, "98:B6:E9:%02X:%02X:%02X",
		(lot0 >> 16) & 0xFF, (lot0 >> 8) & 0xFF, (lot0 + 2) & 0xFF);
	env_set("device_wifi_mac", buf);
}

static void pmic_power_off_reset(void)
{
	struct udevice *dev;
	uchar val;
	int ret;

	ret = i2c_get_chip_for_busnum(5, MAX77620_I2C_ADDR_7BIT, 1, &dev);
	if (ret) {
		printf("%s: Cannot find MAX77620 I2C chip\n", __func__);
		return;
	}

	/* Set soft reset wake up reason */
	ret = dm_i2c_read(dev, MAX77620_REG_ONOFF_CFG2, &val, 1);
	if (ret)
		printf("Failed to read ONOFF_CNFG2 register: %d\n", ret);

	val |= BIT(7); /* SFT_RST_WK */
	ret = dm_i2c_write(dev, MAX77620_REG_ONOFF_CFG2, &val, 1);
	if (ret)
		printf("Failed to write ONOFF_CNFG2: %d\n", ret);

	/* Initiate power down sequence and generate a reset */
	val = BIT(7);
	ret = dm_i2c_write(dev, MAX77620_REG_ONOFF_CFG1, &val, 1);
	if (ret)
		printf("Failed to write ONOFF_CNFG1: %d\n", ret);
}

void reset_misc(void)
{
	/* r2p is not possible on T210B01, so do a full power off reboot */
	if (tegra_get_chip_rev() == MAJORPREV_TEGRA210B01) {
		pmic_power_off_reset();

		mdelay(100);

		/* If failed try a complete power off */
		psci_system_off();
	}

	/* r2p reboot */
	psci_system_reset();
}

int board_env_check(void)
{
	struct pmc_ctlr *const pmc = (struct pmc_ctlr *)NV_PA_PMC_BASE;
	u32 secure_scratch112 = readl(&pmc->pmc_secure_scratch112);
	bool t210b01 = tegra_get_chip_rev() == MAJORPREV_TEGRA210B01;

	if (t210b01 && secure_scratch112 != SECURE_SCRATCH112_SETUP_DONE) {
		eputs("Board was not initialized properly! Hang prevented.\n"
			"Board will reboot in 10s..\n");
		mdelay(10000);

		/* r2p is not possible so do a full power off reboot */
		pmic_power_off_reset();

		mdelay(100);

		eputs("Failed to reboot board!\n");
		return -EDEADLOCK;
	}

	return 0;
}

void board_env_setup(void)
{
	struct pmc_ctlr *const pmc = (struct pmc_ctlr *)NV_PA_PMC_BASE;
	u32 secure_scratch113 = readl(&pmc->pmc_secure_scratch113);
	u32 scratch0 = readl(&pmc->pmc_scratch0);
	u32 display_id = secure_scratch113 & 0xFFFF;
	u32 in_volt_lim = secure_scratch113 >> 16;
	bool t210b01 = tegra_get_chip_rev() == MAJORPREV_TEGRA210B01;

	/* Set SoC type */
	if (t210b01) {
		env_set("t210b01", "1");
	} else {
		env_set("t210b01", "0");
	}

	/* Set SKU type */
	switch (get_sku()) {
	case NX_HW_TYPE_MODIN:
		env_set("sku", "1");
		break;

	case NX_HW_TYPE_VALI:
		env_set("sku", "2");
		break;

	case NX_HW_TYPE_FRIG:
		env_set("sku", "3");
		break;

	case NX_HW_TYPE_ODIN:
	default:
		env_set("sku", "0");
		break;
	}

	/* Handle recovery mode in-place */
	if (scratch0 & SCRATCH0_FASTBOOT_MODE) {
		/* Shouldn't be possible but disable recovery anyway */
		env_set("recovery", "0");
	} else if(scratch0 & SCRATCH0_RECOVERY_MODE) {
		env_set("recovery", "1");
	} else {
		env_set("recovery", "0");
	}

	/* Clear out scratch0 mode select bits */
	writel(scratch0 & (~SCRATCH0_BOOT_MODE_MASK), &pmc->pmc_scratch0);

	/* Set Display ID */
	env_set_hex("display_id", display_id);

	/* Set pmic type for T210b01 */
	if (t210b01)
		set_pmic_type();

	/* Generate device serial and set it to env */
	generate_and_set_serial();

	/* Get charging parameters if Vali */
	if (get_sku() == NX_HW_TYPE_VALI) {
		/* Set voltage limit */
		env_set_hex("VLIM", in_volt_lim);

		/* Set translated SOC */
		switch (in_volt_lim) {
		case 4192:
			env_set_hex("SOCLIM", 90);
			break;
		case 4224:
			env_set_hex("SOCLIM", 93);
			break;
		case 4240:
			env_set_hex("SOCLIM", 94);
			break;
		case 4256:
			env_set_hex("SOCLIM", 95);
			break;
		case 4272:
			env_set_hex("SOCLIM", 97);
			break;
		case 4288:
			env_set_hex("SOCLIM", 98);
			break;
		case 4304:
			env_set_hex("SOCLIM", 99);
			break;
		default:
			env_set_hex("SOCLIM", 100);
			break;
		}
	}
}

void pin_mux_mmc(void)
{
	struct pmc_ctlr *const pmc = (struct pmc_ctlr *)NV_PA_PMC_BASE;
	struct udevice *dev;
	u32 reg_val;
	uchar val;
	int ret;

	/* Make sure the SDMMC1 controller is powered */
	reg_val = readl(&pmc->pmc_no_iopower);
	reg_val |= BIT(12);
	writel(reg_val, &pmc->pmc_no_iopower);
	(void)readl(&pmc->pmc_no_iopower);
	udelay(1000);
	reg_val &= ~(BIT(12));
	writel(reg_val, &pmc->pmc_no_iopower);
	(void)readl(&pmc->pmc_no_iopower);

	/* Inform IO pads that voltage is gonna be 3.3V */
	reg_val = readl(&pmc->pmc_pwr_det_val);
	reg_val |= BIT(12);
	writel(reg_val, &pmc->pmc_pwr_det_val);
	(void)readl(&pmc->pmc_pwr_det_val);

	/* Turn on MAX77620 LDO2 to 3.3V for SD card power */
	ret = i2c_get_chip_for_busnum(5, MAX77620_I2C_ADDR_7BIT, 1, &dev);
	if (ret) {
		printf("%s: Cannot find MAX77620 I2C chip\n", __func__);
		return;
	}
	/* 0xF2 for 3.3v, enabled: bit7:6 = 11 = enable, bit5:0 = voltage */
	val = 0xF2;
	ret = dm_i2c_write(dev, MAX77620_CNFG1_L2_REG, &val, 1);
	if (ret)
		printf("Failed to enable 3.3V LDO for SD Card IO: %d\n", ret);

	/* Disable LDO4 discharge for RTC power. Already disabled on T210B01. */
	ret = dm_i2c_read(dev, MAX77620_CNFG2_L4_REG, &val, 1);
	if (ret) {
		printf("Failed to read LDO4 register: %d\n", ret);
	} else {
		val &= ~BIT(1); /* ADE */
		ret = dm_i2c_write(dev, MAX77620_CNFG2_L4_REG, &val, 1);
		if (ret)
			printf("Failed to disable ADE in LDO4: %d\n", ret);
	}
}

/*
 * Routine: pinmux_init
 * Description: Do individual peripheral pinmux configs
 */
void pinmux_init(void)
{
	gpio_config_table(nintendo_switch_gpio_inits,
			  ARRAY_SIZE(nintendo_switch_gpio_inits));

	pinmux_config_pingrp_table(nintendo_switch_pingrps,
				   ARRAY_SIZE(nintendo_switch_pingrps));

	if (tegra_get_chip_rev() == MAJORPREV_TEGRA210) {
		pinmux_config_pingrp_table(nintendo_switch_sd_t210_pingrps,
				ARRAY_SIZE(nintendo_switch_sd_t210_pingrps));
	} else {
		pinmux_config_pingrp_table(nintendo_switch_sd_t210b01_pingrps,
				ARRAY_SIZE(nintendo_switch_sd_t210b01_pingrps));
	}
}
