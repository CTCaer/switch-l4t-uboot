// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2018 Linaro Ltd.
 * Sam Protsenko <semen.protsenko@linaro.org>
 * (C) Copyright 2022 CTCaer.
 */

#include <common.h>
#include <linux/libfdt.h>
#include <mapmem.h>
#include <linux/types.h>

#define DT_TABLE_MAGIC			0xd7b7ab1e
#define DT_TABLE_DEFAULT_PAGE_SIZE	2048
#define DT_TABLE_DEFAULT_VERSION	0

struct dt_table_header {
	u32 magic;		/* DT_TABLE_MAGIC */
	u32 total_size;		/* includes dt_table_header + all dt_table_entry
				 * and all dtb/dtbo
				 */
	u32 header_size;	/* sizeof(dt_table_header) */

	u32 dt_entry_size;	/* sizeof(dt_table_entry) */
	u32 dt_entry_count;	/* number of dt_table_entry */
	u32 dt_entries_offset;	/* offset to the first dt_table_entry
				 * from head of dt_table_header.
				 * The value will be equal to header_size if
				 * no padding is appended
				 */
	u32 page_size;		/* flash page size we assume */
	u32 version;            /* DTBO image version, the current version is 0.
				 * The version will be incremented when the
				 * dt_table_header struct is updated.
				 */
};

struct dt_table_entry {
	u32 dt_size;
	u32 dt_offset;		/* offset from head of dt_table_header */

	u32 id;			/* optional, must be zero if unused */
	u32 rev;		/* optional, must be zero if unused */
	u32 custom[4];		/* optional, must be zero if unused */
};

/**
 * Check if image header is correct.
 *
 * @param hdr_addr Start address of DT image
 * @return true if header is correct or false if header is incorrect
 */
static bool dt_check_header(ulong hdr_addr)
{
	const struct dt_table_header *hdr;
	u32 magic;

	hdr = map_sysmem(hdr_addr, sizeof(*hdr));
	magic = fdt32_to_cpu(hdr->magic);
	unmap_sysmem(hdr);

	return magic == DT_TABLE_MAGIC;
}

/**
 * Get the address of FDT (dtb or dtbo) in memory by its id.
 *
 * @param hdr_addr Start address of DT image
 * @param entries_offset The entries offset as reported by header
 * @param entry_size The size of an entry as reported by header
 * @param entry_count The number of entries as reported by header
 * @param id ID/sku of desired FDT in image
 * @param[out] dt_offset Contains offset of target dt if found
 * @param[out] dt_size Contains size of target dt if found
 * @param[out] e Contains target dt if found
 *
 * @return true on success or false on error
 */
static bool dt_get_fdt_by_id(ulong hdr_addr, u32 entries_offset, u32 entry_size, u32 entry_count,
				u32 id, u32 *dt_offset, u32 *dt_size, struct dt_table_entry **e)
{
	ulong e_addr;
	u8 i;

	for (i = 0; i < entry_count; i++) {
		e_addr = hdr_addr + entries_offset + i * entry_size;
		(*e) = map_sysmem(e_addr, sizeof(**e));
		*dt_offset = fdt32_to_cpu((*e)->dt_offset);
		*dt_size = fdt32_to_cpu((*e)->dt_size);

		if (fdt32_to_cpu((*e)->id) == id) {
			break;
		}
		
		if (i == (entry_count - 1)) {
			eprintf("Error: id %u not found\n", id);
			unmap_sysmem(*e);
			return false;
		}
	}

	return true;
}

/**
 * Get the address of FDT (dtb or dtbo) in memory by its index in image.
 *
 * @param hdr_addr Start address of DT image
 * @param index Index of desired FDT in image (starting from 0)
 * @param id_text Index of desired FDT in image (starting from 0)
 * @param[out] addr If not NULL, will contain address to specified FDT
 * @param[out] size If not NULL, will contain size of specified FDT
 *
 * @return true on success or false on error
 */
static bool dt_get_fdt_blob(ulong hdr_addr, u32 index, char *id_text, ulong *addr,
				 u32 *size)
{
	const struct dt_table_header *hdr;
	struct dt_table_entry *e = NULL;
	u32 entry_count, entries_offset, entry_size;
	ulong e_addr;
	u32 dt_offset = 0;
	u32 dt_size = 0;
	char env_key[16];
	u8 i;

	hdr = map_sysmem(hdr_addr, sizeof(*hdr));
	entry_count = fdt32_to_cpu(hdr->dt_entry_count);
	entries_offset = fdt32_to_cpu(hdr->dt_entries_offset);
	entry_size = fdt32_to_cpu(hdr->dt_entry_size);
	unmap_sysmem(hdr);

	if (id_text == NULL) {
		if (index > entry_count) {
			eprintf("Error: index > dt_entry_count (%u > %u)\n", index,
				entry_count);
			return false;
		}

		e_addr = hdr_addr + entries_offset + index * entry_size;
		e = map_sysmem(e_addr, sizeof(*e));
		dt_offset = fdt32_to_cpu(e->dt_offset);
		dt_size = fdt32_to_cpu(e->dt_size);

	} else {
		if (entry_count <= 0) {
			eprintf("Error: no entries found--invalid header?\n");
			return false;
		}

		if (!dt_get_fdt_by_id(hdr_addr, entries_offset, entry_size, entry_count, *id_text, 
				&dt_offset, &dt_size, &e)) {
			eprintf("Error: Failed to get dt by id\n");
			return false;
		}
	}

	/* Set dt info to env */
	env_set_hex("fdt_id", fdt32_to_cpu(e->id));
	env_set_hex("fdt_rev", fdt32_to_cpu(e->rev));

	for (i = 0; i < 4; i++) {
		sprintf(env_key, "fdt_custom%d", i);
		env_set_hex(env_key, fdt32_to_cpu(e->custom[i]));
	}

	/* Set readable id to env in case it's ASCII */
	memcpy(env_key, &e->id, sizeof(e->id));
	env_key[sizeof(e->id)] = '\0';
	env_set("fdt_id_text", env_key);

	unmap_sysmem(e);

	if (addr)
		*addr = hdr_addr + dt_offset;
	if (size)
		*size = dt_size;

	return true;
}

#if !defined(CONFIG_SPL_BUILD)
static void dt_print_fdt_info(const struct fdt_header *fdt)
{
	u32 fdt_size;
	int root_node_off;
	const char *compatible = NULL;

	fdt_size = fdt_totalsize(fdt);
	root_node_off = fdt_path_offset(fdt, "/");
	if (root_node_off < 0) {
		eprintf("Error: Root node not found\n");
	} else {
		compatible = fdt_getprop(fdt, root_node_off, "compatible",
					 NULL);
	}

	printf("          (FDT)size = %d\n", fdt_size);
	printf("    (FDT)compatible = %s\n",
	       compatible ? compatible : "(unknown)");
}

/**
 * Print information about DT image structure.
 *
 * @param hdr_addr Start address of DT image
 */
static void dt_print_contents(ulong hdr_addr)
{
	const struct dt_table_header *hdr;
	u32 entry_count, entries_offset, entry_size;
	u32 i;

	hdr = map_sysmem(hdr_addr, sizeof(*hdr));
	entry_count = fdt32_to_cpu(hdr->dt_entry_count);
	entries_offset = fdt32_to_cpu(hdr->dt_entries_offset);
	entry_size = fdt32_to_cpu(hdr->dt_entry_size);

	/* Print image header info */
	printf("dt_table_header:\n");
	printf("              magic = %08x\n", fdt32_to_cpu(hdr->magic));
	printf("         total_size = %d\n", fdt32_to_cpu(hdr->total_size));
	printf("        header_size = %d\n", fdt32_to_cpu(hdr->header_size));
	printf("      dt_entry_size = %d\n", entry_size);
	printf("     dt_entry_count = %d\n", entry_count);
	printf("  dt_entries_offset = %d\n", entries_offset);
	printf("          page_size = %d\n", fdt32_to_cpu(hdr->page_size));
	printf("            version = %d\n", fdt32_to_cpu(hdr->version));

	unmap_sysmem(hdr);

	/* Print image entries info */
	for (i = 0; i < entry_count; ++i) {
		const ulong e_addr = hdr_addr + entries_offset + i * entry_size;
		const struct dt_table_entry *e;
		const struct fdt_header *fdt;
		u32 dt_offset, dt_size;
		u32 j;

		e = map_sysmem(e_addr, sizeof(*e));
		dt_offset = fdt32_to_cpu(e->dt_offset);
		dt_size = fdt32_to_cpu(e->dt_size);

		printf("  dt_table_entry[%d]:\n", i);
		printf("       dt_size = %d\n", dt_size);
		printf("     dt_offset = %d\n", dt_offset);
		printf("            id = %08x\n", fdt32_to_cpu(e->id));
		printf("           rev = %08x\n", fdt32_to_cpu(e->rev));
		for (j = 0; j < 4; ++j) {
			printf("    custom[%d] = %08x\n", j,
			       fdt32_to_cpu(e->custom[j]));
		}

		unmap_sysmem(e);

		/* Print FDT info for this entry */
		fdt = map_sysmem(hdr_addr + dt_offset, sizeof(*fdt));
		dt_print_fdt_info(fdt);
		unmap_sysmem(fdt);
	}
}
#endif

enum cmd_dtimg_info {
	CMD_DTIMG_START = 0,
	CMD_DTIMG_SIZE,
	CMD_DTIMG_LOAD,
};

static int do_dtimg_dump(cmd_tbl_t *cmdtp, int flag, int argc,
			 char * const argv[])
{
	char *endp;
	ulong hdr_addr;

	if (argc != 2)
		return CMD_RET_USAGE;

	hdr_addr = simple_strtoul(argv[1], &endp, 16);
	if (*endp != '\0') {
		eprintf("Error: Wrong image address\n");
		return CMD_RET_FAILURE;
	}

	if (!dt_check_header(hdr_addr)) {
		eprintf("Error: DT image header is incorrect\n");
		return CMD_RET_FAILURE;
	}

	dt_print_contents(hdr_addr);

	return CMD_RET_SUCCESS;
}

static int dtimg_get_fdt(int argc, char * const argv[], enum cmd_dtimg_info cmd)
{
	ulong hdr_addr;
	u32 index;
	char *endp;
	ulong fdt_addr, addr;
	u32 fdt_size;
	void *wbuf;
	char *id = NULL;

	switch (cmd) {
	case CMD_DTIMG_START:
	case CMD_DTIMG_SIZE:
		if (argc != 4 && argc != 5)
			return CMD_RET_USAGE;
		break;
	case CMD_DTIMG_LOAD:
		if (argc != 5 && argc != 6)
			return CMD_RET_USAGE;
		break;
	default:
		return CMD_RET_USAGE;
	}

	hdr_addr = simple_strtoul(argv[1], &endp, 16);
	if (*endp != '\0') {
		eprintf("Error: Wrong image address\n");
		return CMD_RET_FAILURE;
	}

	if (!dt_check_header(hdr_addr)) {
		eprintf("Error: DT image header is incorrect\n");
		return CMD_RET_FAILURE;
	}

	index = simple_strtoul(argv[2], &endp, 0);
	if (*endp != '\0') {
		eprintf("Error: Wrong index\n");
		return CMD_RET_FAILURE;
	}

	if (cmd <= CMD_DTIMG_SIZE && argc == 5) {
		id = argv[4];
	} else if (cmd == CMD_DTIMG_LOAD && argc == 6) {
		id = argv[5];
	}

	if (!dt_get_fdt_blob(hdr_addr, index, id, &fdt_addr, &fdt_size))
		return CMD_RET_FAILURE;

	switch (cmd) {
	case CMD_DTIMG_START:
		env_set_hex(argv[3], fdt_addr);
		break;
	case CMD_DTIMG_SIZE:
		env_set_hex(argv[3], fdt_size);
		break;
	case CMD_DTIMG_LOAD:
		addr = simple_strtoul(argv[3], &endp, 16);
		if (*endp != '\0') {
			eprintf("Error: Wrong destination address\n");
			return CMD_RET_FAILURE;
		}

		wbuf = map_sysmem(addr, fdt_size);
		memcpy(wbuf, (void *)fdt_addr, fdt_size);
		unmap_sysmem(wbuf);

		env_set_hex(argv[4], fdt_size);
		break;
	default:
		printf("Error: Unknown cmd_dtimg_info value: %d\n", cmd);
		return CMD_RET_FAILURE;
	}

	return CMD_RET_SUCCESS;
}

static int do_dtimg_start(cmd_tbl_t *cmdtp, int flag, int argc,
			  char * const argv[])
{
	return dtimg_get_fdt(argc, argv, CMD_DTIMG_START);
}

static int do_dtimg_size(cmd_tbl_t *cmdtp, int flag, int argc,
			 char * const argv[])
{
	return dtimg_get_fdt(argc, argv, CMD_DTIMG_SIZE);
}

static int do_dtimg_load(cmd_tbl_t *cmdtp, int flag, int argc,
			  char * const argv[])
{
	return dtimg_get_fdt(argc, argv, CMD_DTIMG_LOAD);
}


static cmd_tbl_t cmd_dtimg_sub[] = {
	U_BOOT_CMD_MKENT(dump, 2, 0, do_dtimg_dump, "", ""),
	U_BOOT_CMD_MKENT(start, 5, 0, do_dtimg_start, "", ""),
	U_BOOT_CMD_MKENT(size, 5, 0, do_dtimg_size, "", ""),
	U_BOOT_CMD_MKENT(load, 6, 0, do_dtimg_load, "", ""),
};

static int do_dtimg(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	cmd_tbl_t *cp;

	cp = find_cmd_tbl(argv[1], cmd_dtimg_sub, ARRAY_SIZE(cmd_dtimg_sub));

	/* Strip off leading 'dtimg' command argument */
	argc--;
	argv++;

	if (!cp || argc > cp->maxargs)
		return CMD_RET_USAGE;
	if (flag == CMD_FLAG_REPEAT && !cp->repeatable)
		return CMD_RET_SUCCESS;

	return cp->cmd(cmdtp, flag, argc, argv);
}

U_BOOT_CMD(
	dtimg, CONFIG_SYS_MAXARGS, 0, do_dtimg,
	"manipulate dtb/dtbo Android image",
	"dump <addr>\n"
	"    - parse specified image and print its structure info\n"
	"      <addr>: image address in RAM, in hex\n"
	"dtimg start <addr> <index> <varname> [<id>]\n"
	"    - get address (hex) of FDT in the image, by index\n"
	"      <addr>: image address in RAM, in hex\n"
	"      <index>: index of desired FDT in the image\n"
	"      <varname>: name of variable where to store address of FDT\n"
	"      [<id>]: id of desired FDT (optional)"
	"dtimg size <addr> <index> <varname> [<id>]\n"
	"    - get size (hex, bytes) of FDT in the image, by index\n"
	"      <addr>: image address in RAM, in hex\n"
	"      <index>: index of desired FDT in the image\n"
	"      <varname>: name of variable where to store size of FDT\n"
	"      [<id>]: id of desired FDT (optional)"
	"dtimg load <addr> <index> <addr_new> <varname> [<id>]\n"
	"    - get size (hex, bytes) of FDT in the image, by index\n"
	"      <addr>: image address in RAM, in hex\n"
	"      <index>: index of desired FDT in the image\n"
	"      <addr_new>: address to load FDT\n"
	"      <varname>: name of variable where to store size of FDT\n"
	"      [<id>]: id of desired FDT (optional)"
);
