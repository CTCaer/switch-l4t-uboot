/*
 * (C) Copyright 2022, CTCaer.
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <i2c.h>
#include <asm/io.h>
#include <asm/arch/gpio.h>
#include <asm/arch/pinmux.h>
#include <asm/arch/tegra.h>
#include <asm/arch-tegra/pmc.h>
#include <asm/arch-tegra/apb_misc.h>
#include <asm/arch-tegra210/gp_padctrl.h>
#include "../../nvidia/p2571/max77620_init.h"
#include "pinmux-config-nintendo-switch.h"

static bool get_soc_t210b01(void)
{
	struct apb_misc_gp_ctlr *gp =
			      (struct apb_misc_gp_ctlr *)NV_PA_APB_MISC_GP_BASE;
	u32 major_id = (readl(&gp->hidrev) & HIDREV_MAJORPREV_MASK) >>
		       HIDREV_MAJORPREV_SHIFT;

	/* Return if T210B01 */
	return (major_id == 2);
}

static void pmic_power_off_reset(void)
{
	struct udevice *dev;
	uchar val;
	int ret;

	ret = i2c_get_chip_for_busnum(5, MAX77620_I2C_ADDR_7BIT, 1, &dev);
	if (ret) {
		debug("%s: Cannot find MAX77620 I2C chip\n", __func__);
		return;
	}

	/* Set soft reset wake up reason */
	ret = dm_i2c_read(dev, MAX77620_REG_ONOFF_CFG2, &val, 1);
	if (ret)
		debug("Failed to read ONOFF_CNFG2 register: %d\n", ret);

	val |= BIT(7); /* SFT_RST_WK */
	ret = dm_i2c_write(dev, MAX77620_REG_ONOFF_CFG2, &val, 1);
	if (ret)
		debug("Failed to write ONOFF_CNFG2: %d\n", ret);

	/* Initiate power down sequence and generate a reset */
	val = BIT(7);
	ret = dm_i2c_write(dev, MAX77620_REG_ONOFF_CFG1, &val, 1);
	if (ret)
		debug("Failed to write ONOFF_CNFG1: %d\n", ret);
}

void reset_misc(void)
{
	/* r2p is not possible on T210B01, so do a full power off reboot */
	if (get_soc_t210b01()) {
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
	bool t210b01 = get_soc_t210b01();

	if (t210b01 && secure_scratch112 != SECURE_SCRATCH112_SETUP_DONE) {
		fprintf(stderr,
			"Board was not initialized properly! Hang prevented.\n"
			"Board will reboot in 10s..\n");
		mdelay(10000);

		/* r2p is not possible so do a full power off reboot */
		pmic_power_off_reset();

		mdelay(100);

		fprintf(stderr,"Failed to reboot board!\n");
		return -EDEADLOCK;
	}

	return 0;
}

void board_env_setup(void)
{
	struct pmc_ctlr *const pmc = (struct pmc_ctlr *)NV_PA_PMC_BASE;
	u32 scratch0 = readl(&pmc->pmc_scratch0);

	/* Set SoC type */
	if (get_soc_t210b01()) {
		env_set("t210b01", "1");
	} else {
		env_set("t210b01", "0");
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
	debug("%s: Set LDO2 for VDDIO_SDMMC_AP power to 3.3V\n", __func__);
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

	/* Disable LDO4 discharge for RTC power */
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
	pinmux_clear_tristate_input_clamping();

	gpio_config_table(nintendo_switch_gpio_inits,
			  ARRAY_SIZE(nintendo_switch_gpio_inits));

	pinmux_config_pingrp_table(nintendo_switch_pingrps,
				   ARRAY_SIZE(nintendo_switch_pingrps));

	if (!get_soc_t210b01()) {
		pinmux_config_pingrp_table(nintendo_switch_sd_t210_pingrps,
				ARRAY_SIZE(nintendo_switch_sd_t210_pingrps));
	} else {
		pinmux_config_pingrp_table(nintendo_switch_sd_t210b01_pingrps,
				ARRAY_SIZE(nintendo_switch_sd_t210b01_pingrps));
	}
}
