/*
 * (C) Copyright 2000-2009
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <bootm.h>
#include <command.h>
#include <image.h>
#include <lmb.h>
#include <mapmem.h>
#include <stdlib.h>
#include <libfdt.h>
#include <linux/kernel.h>
#include <linux/sizes.h>

DECLARE_GLOBAL_DATA_PTR;

/*
 * Image booting support
 */
static int booti_start(cmd_tbl_t *cmdtp, int flag, int argc,
			char * const argv[], bootm_headers_t *images)
{
	int ret;
	ulong ld;
	ulong relocated_addr;
	ulong image_size;

	ret = do_bootm_states(cmdtp, flag, argc, argv, BOOTM_STATE_START,
			      images, 1);

	/* Setup Linux kernel Image entry point */
	if (!argc) {
		ld = load_addr;
		debug("*  kernel: default image load address = 0x%08lx\n",
				load_addr);
	} else {
		ld = simple_strtoul(argv[0], NULL, 16);
		debug("*  kernel: cmdline image address = 0x%08lx\n",
			images->ep);
	}

	ret = booti_setup(ld, &relocated_addr, &image_size);
	if (ret != 0)
		return 1;

	/* Handle BOOTM_STATE_LOADOS */
	if (relocated_addr != ld) {
		debug("Moving Image from 0x%lx to 0x%lx\n", ld, relocated_addr);
		memmove((void *)relocated_addr, (void *)ld, image_size);
	}

	images->ep = relocated_addr;
	lmb_reserve(&images->lmb, images->ep, le32_to_cpu(image_size));

	/*
	 * Handle the BOOTM_STATE_FINDOTHER state ourselves as we do not
	 * have a header that provide this informaiton.
	 */
	if (bootm_find_images(flag, argc, argv))
		return 1;

	return 0;
}

int do_booti(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int ret;

	/* Consume 'booti' */
	argc--; argv++;

	if (booti_start(cmdtp, flag, argc, argv, &images))
		return 1;

	/*
	 * We are doing the BOOTM_STATE_LOADOS state ourselves, so must
	 * disable interrupts ourselves
	 */
	bootm_disable_interrupts();

	images.os.os = IH_OS_LINUX;
	images.os.arch = IH_ARCH_ARM64;
	ret = do_bootm_states(cmdtp, flag, argc, argv,
#ifdef CONFIG_SYS_BOOT_RAMDISK_HIGH
			      BOOTM_STATE_RAMDISK |
#endif
			      BOOTM_STATE_OS_PREP | BOOTM_STATE_OS_FAKE_GO |
			      BOOTM_STATE_OS_GO,
			      &images, 1);

	return ret;
}

int do_boota(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    struct andr_img_hdr *hdr;
    ulong kernel_addr = 0;
    ulong kernel_len = 0;
    ulong ramdisk_addr = 0;
    ulong ramdisk_len = 0;
    ulong fdt_addr = 0;
    ulong fdt_len = 0;
    ulong ramdisk_addr_env = 0;
    ulong fdt_addr_env = 0;

    if (argc == 4) {
        debug("bin normal %s %s %s %s\n", argv[0], argv[1], argv[2], argv[3]);
        return do_booti(cmdtp, flag, argc, argv);
    }

    debug("boot android arm64 bootimage\n");
    hdr = (struct andr_img_hdr *)simple_strtoul(argv[1], NULL, 16);
    if (android_image_check_header(hdr)) {
        printf("invalid android image\n");
        return -1;
    }

    android_image_get_kernel(hdr, false, &kernel_addr, &kernel_len);
    android_image_get_ramdisk(hdr, &ramdisk_addr, &ramdisk_len);
    android_image_get_second(hdr, &fdt_addr, &fdt_len);

    if (fdt_check_header((void*)fdt_addr)) {
        printf(" error: invalid fdt\n");
        return -1;
    }

    /* relocate ramdisk and fdt to the address defined by the environment variable.
     * that means we'll ignore the load address of ramdisk and dtb defined in the
     * abootimg, since it make more sense letting u-boot handling where to put what.
     * kernel relocation will be handled in booti_setup
     */
    ramdisk_addr_env = env_get_ulong("ramdisk_addr_r", 16, 0);;
    fdt_addr_env = env_get_ulong("fdt_addr_r", 16, 0);

    if (!ramdisk_addr_env) {
        printf(" error: didn't define ramdisk_addr_r\n");
        return -1;
    }
    memmove((void *)ramdisk_addr_env, (void *)ramdisk_addr, ramdisk_len);

    if (!fdt_addr_env) {
        printf(" error: didn't define fdt_addr_r\n");
        return -1;
    }
    memmove((void *)fdt_addr_env, (void *)fdt_addr, fdt_len);

    const int max_length = 40;
    const int new_argc = 4;
    char *new_argv[new_argc];

    for (int i = 0; i < new_argc; i++) {
        new_argv[i] = (char*) malloc(max_length);
    }

    strcpy(new_argv[0], "booti");
    snprintf(new_argv[1], max_length, "0x%lx", kernel_addr);
    snprintf(new_argv[2], max_length, "0x%lx:%lx", ramdisk_addr_env,ramdisk_len);
    snprintf(new_argv[3], max_length, "0x%lx", fdt_addr_env);

    debug("android: %s %s %s %s\n", new_argv[0], new_argv[1], new_argv[2], new_argv[3]);

    int ret = do_booti(cmdtp, flag, new_argc, new_argv);

    for (int i = 0; i < new_argc; i++) {
        free(new_argv[i]);
    }

    return ret;
}

#ifdef CONFIG_SYS_LONGHELP
static char booti_help_text[] =
	"[addr [initrd[:size]] [fdt]]\n"
	"    - boot arm64 Linux Image stored in memory\n"
	"\tThe argument 'initrd' is optional and specifies the address\n"
	"\tof an initrd in memory. The optional parameter ':size' allows\n"
	"\tspecifying the size of a RAW initrd.\n"
#if defined(CONFIG_OF_LIBFDT)
	"\tSince booting a Linux kernel requires a flat device-tree, a\n"
	"\tthird argument providing the address of the device-tree blob\n"
	"\tis required. To boot a kernel with a device-tree blob but\n"
	"\twithout an initrd image, use a '-' for the initrd argument.\n"
#endif
	"";
#endif

U_BOOT_CMD(
	booti,	CONFIG_SYS_MAXARGS,	1,	do_booti,
	"boot arm64 Linux Image image from memory", booti_help_text);
U_BOOT_CMD(
	boota,	CONFIG_SYS_MAXARGS,	1,	do_boota,
	"boot arm64 boot image from memory", booti_help_text);
