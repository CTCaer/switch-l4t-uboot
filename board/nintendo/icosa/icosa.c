/*
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <env.h>
#include <i2c.h>
#include <asm/arch/gpio.h>
#include <asm/arch/pinmux.h>
#include <asm/arch-tegra/pmc.h>
#include "../../nvidia/p2571/max77620_init.h"

void pin_mux_mmc(void)
{
	struct udevice *dev;
	uchar val;
	int ret;

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
		printf("i2c_write 0 0x3c 0x27 failed: %d\n", ret);

	/* Disable LDO4 discharge */
	ret = dm_i2c_read(dev, MAX77620_CNFG2_L4_REG, &val, 1);
	if (ret) {
		printf("i2c_read 0 0x3c 0x2c failed: %d\n", ret);
	} else {
		val &= ~BIT(1); /* ADE */
		ret = dm_i2c_write(dev, MAX77620_CNFG2_L4_REG, &val, 1);
		if (ret)
			printf("i2c_write 0 0x3c 0x2c failed: %d\n", ret);
	}
}

#define PMC_SCRATCH0_RECOVERY_MODE		(1 << 31)
#define PMC_SCRATCH0_FASTBOOT_MODE		(1 << 30)
#define PMC_SCRATCH0_PAYLOAD_MODE		(1 << 29)
#define PMC_SCRATCH0_MASK ((1 << 31) | (1 << 30) | (1<<29))
// This has nothing to do with a fan, but it gets called at the right time
void start_cpu_fan(void)
{
	// switch: check scratch
	u32 scratch0 = tegra_pmc_readl(offsetof(struct pmc_ctlr, pmc_scratch0));

	if(scratch0 & PMC_SCRATCH0_FASTBOOT_MODE)
	{
		// We shouldn't get here, but don't boot to recovery anyway.
		env_set("recovery", "0");
	}
	else if(scratch0 & PMC_SCRATCH0_RECOVERY_MODE)
	{
		env_set("recovery", "1");
	}
	else
	{
		env_set("recovery", "0");
	}

	// Clear out scratch0 mode select bits
	tegra_pmc_writel(scratch0 & ~PMC_SCRATCH0_MASK, offsetof(struct pmc_ctlr, pmc_scratch0));
}
