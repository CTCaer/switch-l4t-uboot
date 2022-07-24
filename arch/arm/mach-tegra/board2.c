/*
 *  (C) Copyright 2010,2011,2017
 *  NVIDIA Corporation <www.nvidia.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <dm.h>
#include <errno.h>
#include <ns16550.h>
#include <usb.h>
#include <asm/io.h>
#include <asm/arch-tegra/ap.h>
#include <asm/arch-tegra/board.h>
#include <asm/arch-tegra/clk_rst.h>
#include <asm/arch-tegra/pmc.h>
#include <asm/arch-tegra/sys_proto.h>
#include <asm/arch-tegra/uart.h>
#include <asm/arch-tegra/warmboot.h>
#include <asm/arch-tegra/gpu.h>
#include <asm/arch-tegra/usb.h>
#include <asm/arch-tegra/xusb-padctl.h>
#include <asm/arch-tegra210/mc.h>
#include <asm/arch/clock.h>
#include <asm/arch/funcmux.h>
#include <asm/arch/pinmux.h>
#include <asm/arch/pmu.h>
#include <asm/arch/tegra.h>
#ifdef CONFIG_TEGRA_CLOCK_SCALING
#include <asm/arch/emc.h>
#endif
#include "emc.h"

DECLARE_GLOBAL_DATA_PTR;

#ifdef CONFIG_SPL_BUILD
/* TODO(sjg@chromium.org): Remove once SPL supports device tree */
U_BOOT_DEVICE(tegra_gpios) = {
	"gpio_tegra"
};
#endif

__weak void pinmux_init(void) {}
__weak void pin_mux_usb(void) {}
__weak void pin_mux_spi(void) {}
__weak void pin_mux_mmc(void) {}
__weak void gpio_early_init_uart(void) {}
__weak void pin_mux_display(void) {}
__weak void start_cpu_fan(void) {}
__weak void board_env_setup(void) {}

#if defined(CONFIG_TEGRA_NAND)
__weak void pin_mux_nand(void)
{
	funcmux_select(PERIPH_ID_NDFLASH, FUNCMUX_DEFAULT);
}
#endif

__weak int board_env_check(void)
{
	return 0;
}

/*
 * Routine: power_det_init
 * Description: turn off power detects
 */
static void power_det_init(void)
{
#if defined(CONFIG_TEGRA20)
	struct pmc_ctlr *const pmc = (struct pmc_ctlr *)NV_PA_PMC_BASE;

	/* turn off power detects */
	writel(0, &pmc->pmc_pwr_det_latch);
	writel(0, &pmc->pmc_pwr_det);
#endif
}

__weak int tegra_board_id(void)
{
	return -1;
}

#ifdef CONFIG_DISPLAY_BOARDINFO
int checkboard(void)
{
	int board_id = tegra_board_id();

	printf("Board: %s", CONFIG_TEGRA_BOARD_STRING);
	if (board_id != -1)
		printf(", ID: %d\n", board_id);
	printf("\n");

	return 0;
}
#endif	/* CONFIG_DISPLAY_BOARDINFO */

__weak int tegra_lcd_pmic_init(int board_it)
{
	return 0;
}

__weak int nvidia_board_init(void)
{
	return 0;
}

/*
 * Routine: board_init
 * Description: Early hardware init.
 */
int board_init(void)
{
	__maybe_unused int err;
	__maybe_unused int board_id;

	/* Do clocks and UART first so that printf() works */
	clock_init();
	clock_verify();

	tegra_gpu_config();

	/* Check that board early init was setup properly */
	err = board_env_check();
	if (err) {
		debug("Board was not initialized properly!\n");
		return err;
	}

#ifdef CONFIG_TEGRA_SPI
	pin_mux_spi();
#endif

#ifdef CONFIG_MMC_SDHCI_TEGRA
	pin_mux_mmc();
#endif

	/* Init is handled automatically in the driver-model case */
#if defined(CONFIG_DM_VIDEO)
	pin_mux_display();
#endif
	/* boot param addr */
	gd->bd->bi_boot_params = (NV_PA_SDRAM_BASE + 0x100);

	power_det_init();

#ifdef CONFIG_SYS_I2C_TEGRA
# ifdef CONFIG_TEGRA_PMU
	if (pmu_set_nominal())
		debug("Failed to select nominal voltages\n");
#  ifdef CONFIG_TEGRA_CLOCK_SCALING
	err = board_emc_init();
	if (err)
		debug("Memory controller init failed: %d\n", err);
#  endif
# endif /* CONFIG_TEGRA_PMU */
#endif /* CONFIG_SYS_I2C_TEGRA */

#ifdef CONFIG_USB_EHCI_TEGRA
	pin_mux_usb();
#endif

#if defined(CONFIG_DM_VIDEO)
	board_id = tegra_board_id();
	err = tegra_lcd_pmic_init(board_id);
	if (err) {
		debug("Failed to set up LCD PMIC\n");
		return err;
	}
#endif

#ifdef CONFIG_TEGRA_NAND
	pin_mux_nand();
#endif

	tegra_xusb_padctl_init();

#ifdef CONFIG_TEGRA_LP0
	/* save Sdram params to PMC 2, 4, and 24 for WB0 */
	warmboot_save_sdram_params();

	/* prepare the WB code to LP0 location */
	warmboot_prepare_code(TEGRA_LP0_ADDR, TEGRA_LP0_SIZE);
#endif

	return nvidia_board_init();
}

void board_cleanup_before_linux(void)
{
	/* power down UPHY PLL */
	tegra_xusb_padctl_exit();
}

#ifdef CONFIG_BOARD_EARLY_INIT_F
static void __gpio_early_init(void)
{
}

void gpio_early_init(void) __attribute__((weak, alias("__gpio_early_init")));

int board_early_init_f(void)
{
	if (!clock_early_init_done())
		clock_early_init();

#if defined(CONFIG_TEGRA_DISCONNECT_UDC_ON_BOOT)
#define USBCMD_FS2 (1 << 15)
	{
		struct usb_ctlr *usbctlr = (struct usb_ctlr *)0x7d000000;
		writel(USBCMD_FS2, &usbctlr->usb_cmd);
	}
#endif

	/* Do any special system timer/TSC setup */
#if defined(CONFIG_TEGRA_SUPPORT_NON_SECURE)
	if (!tegra_cpu_is_non_secure())
#endif
		arch_timer_init();

	pinmux_init();
	board_init_uart_f();

	/* Initialize periph GPIOs */
	gpio_early_init();
	gpio_early_init_uart();

	return 0;
}
#endif	/* EARLY_INIT */

int board_late_init(void)
{
#if defined(CONFIG_TEGRA_SUPPORT_NON_SECURE)
	if (tegra_cpu_is_non_secure()) {
		printf("CPU is in NS mode\n");
		env_set("cpu_ns_mode", "1");
	} else {
		env_set("cpu_ns_mode", "");
	}
#endif
	start_cpu_fan();

	board_env_setup();

	return 0;
}

phys_size_t carveout_t210_size(bool below_4g)
{
	struct mc_ctlr *mc = (struct mc_ctlr *)NV_PA_MC_BASE;
	u32 *gsc_bom = (u32 *)&mc->mc_security_carveout1_bom;
	u32 *gsc_bom_hi = (u32 *)&mc->mc_security_carveout1_bom_hi;
	u32 *gsc_size_128kb = (u32 *)&mc->mc_security_carveout1_size_128kb;
	const phys_addr_t dram_base = CONFIG_SYS_SDRAM_BASE;
	const phys_addr_t bank_start = below_4g ?
					(dram_base) :
					(dram_base + SZ_2G);
	const phys_addr_t bank_end = below_4g ?
					(dram_base + SZ_2G) :
					(dram_base + gd->ram_size);
	phys_addr_t carveout_start = bank_end;
	phys_addr_t addr;
	phys_size_t size;
	int i;

	/* Parse Secure Carveout */
	addr = readl(&mc->mc_sec_carveout_bom);
	addr |= ((phys_addr_t)readl(&mc->mc_sec_carveout_adr_hi) << 32);
	size = readl(&mc->mc_sec_carveout_size_mb);
	if (size && addr >= bank_start && addr <= bank_end &&
	    addr < carveout_start) {
		carveout_start = addr;
	}

	/* Parse CPU Microcode Carveout */
	addr = readl(&mc->mc_mts_carveout_bom);
	addr |= ((phys_addr_t)readl(&mc->mc_mts_carveout_adr_hi) << 32);
	size = readl(&mc->mc_mts_carveout_size_mb);
	if (size && addr >= bank_start && addr <= bank_end &&
	    addr < carveout_start) {
		carveout_start = addr;
	}

	/* Parse VPR Carveout */
	addr = readl(&mc->mc_video_protect_bom);
	addr |= ((phys_addr_t)readl(&mc->mc_video_protect_bom_adr_hi) << 32);
	size = readl(&mc->mc_video_protect_size_mb);
	if (size && addr >= bank_start && addr <= bank_end &&
	    addr < carveout_start) {
		carveout_start = addr;
	}

	/* Parse GSC Carveouts */
	for (i = 0; i < 5; i++) {
		/* GSC1 - NVDEC Carveout */
		addr = readl(gsc_bom + 0x14 * i);
		addr |= ((phys_addr_t)readl(gsc_bom_hi + 0x14 * i) << 32);
		size = readl(gsc_size_128kb + 0x14 * i);
		if (size && addr >= bank_start && addr <= bank_end &&
		    addr < carveout_start) {
			carveout_start = addr;
		}
	}

	return (bank_end - carveout_start);
}

/*
 * In some SW environments, a memory carve-out exists to house a secure
 * monitor, a trusted OS, and/or various statically allocated media buffers.
 *
 * This carveout exists at the highest possible address that is within a
 * 32-bit physical address space.
 *
 * For now, we support two configurations in U-Boot:
 * - 32-bit ports without any form of carve-out.
 * - 64 bit ports are assumed to use a carve-out below TZDRAM.
 */
static ulong carveout_size(bool below_4g)
{
#ifdef CONFIG_ARM64

#ifndef CONFIG_TEGRA210

	return (below_4g ? SZ_512M : 0);

#elif defined(CONFIG_TEGRA210_CARVEOUT_EXACT_SIZE)

	return carveout_t210_size(below_4g);

#else /* Jetson/Shield T210B01 boards non-cboot */

	return (below_4g ? SZ_16M : (SZ_8M + SZ_4M + SZ_1M));

#endif

#else /* NOT CONFIG_ARM64 */

	return 0;
#endif
}

/*
 * Determine the amount of usable RAM below 4GiB, taking into account any
 * carve-out that may be assigned.
 */
static ulong usable_ram_size_below_4g(void)
{
	ulong total_size_below_4g;
	ulong usable_size_below_4g;

	/*
	 * The total size of RAM below 4GiB is the lesser address of:
	 * (a) 2GiB itself (RAM starts at 2GiB, and 4GiB - 2GiB == 2GiB).
	 * (b) The size RAM physically present in the system.
	 */
	if (gd->ram_size < SZ_2G)
		total_size_below_4g = gd->ram_size;
	else
		total_size_below_4g = SZ_2G;

	/* Calculate usable RAM by subtracting out any carve-out size */
	usable_size_below_4g = total_size_below_4g - carveout_size(true);

	return usable_size_below_4g;
}

/*
 * Determine the amount of usable RAM above 4GiB, taking into account any
 * carve-out that may be assigned.
 */
static phys_size_t usable_ram_size_above_4g(void)
{
	phys_size_t total_size_above_4g;
	phys_size_t usable_size_above_4g;

	total_size_above_4g = gd->ram_size - SZ_2G;

	/* Calculate usable RAM by subtracting out any carve-out size */
	usable_size_above_4g = total_size_above_4g - carveout_size(false);

	return usable_size_above_4g;
}

/*
 * Represent all available RAM in either one or two banks.
 *
 * The first bank describes any usable RAM below 4GiB.
 * The second bank describes any RAM above 4GiB.
 *
 * This split is driven by the following requirements:
 * - The NVIDIA L4T kernel requires separate entries in the DT /memory/reg
 *   property for memory below and above the 4GiB boundary. The layout of that
 *   DT property is directly driven by the entries in the U-Boot bank array.
 * - The potential existence of a carve-out at the end of RAM below 4GiB can
 *   only be represented using multiple banks.
 *
 * Explicitly removing the carve-out RAM from the bank entries makes the RAM
 * layout a bit more obvious, e.g. when running "bdinfo" at the U-Boot
 * command-line.
 *
 * This does mean that the DT U-Boot passes to the Linux kernel will not
 * include this RAM in /memory/reg at all. An alternative would be to include
 * all RAM in the U-Boot banks (and hence DT), and add a /memreserve/ node
 * into DT to stop the kernel from using the RAM. IIUC, I don't /think/ the
 * Linux kernel will ever need to access any RAM in* the carve-out via a CPU
 * mapping, so either way is acceptable.
 *
 * On 32-bit systems, we never define a bank for RAM above 4GiB, since the
 * start address of that bank cannot be represented in the 32-bit .size
 * field.
 */
int dram_init_banksize(void)
{
	gd->bd->bi_dram[0].start = CONFIG_SYS_SDRAM_BASE;
	gd->bd->bi_dram[0].size = usable_ram_size_below_4g();

#ifdef CONFIG_PCI
	gd->pci_ram_top = gd->bd->bi_dram[0].start + gd->bd->bi_dram[0].size;
#endif

#ifdef CONFIG_PHYS_64BIT
	if (gd->ram_size > SZ_2G) {
		gd->bd->bi_dram[1].start = 0x100000000;
		gd->bd->bi_dram[1].size = usable_ram_size_above_4g();
	} else
#endif
	{
		gd->bd->bi_dram[1].start = 0;
		gd->bd->bi_dram[1].size = 0;
	}

	return 0;
}

/*
 * Most hardware on 64-bit Tegra is still restricted to DMA to the lower
 * 32-bits of the physical address space. Cap the maximum usable RAM area
 * at 4 GiB to avoid DMA buffers from being allocated beyond the 32-bit
 * boundary that most devices can address. Also, don't let U-Boot use any
 * carve-out, as mentioned above.
 *
 * This function is called before dram_init_banksize(), so we can't simply
 * return gd->bd->bi_dram[1].start + gd->bd->bi_dram[1].size.
 */
ulong board_get_usable_ram_top(ulong total_size)
{
	return CONFIG_SYS_SDRAM_BASE + usable_ram_size_below_4g();
}
