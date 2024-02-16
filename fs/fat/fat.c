/*
 * Copyright (C) 2015 Stephen Warren <swarren at wwwdotorg.org>
 * (GPL-2.0+)
 *
 * Small portions taken from Barebox v2015.07.0
 * Copyright (c) 2007 Sascha Hauer <s.hauer at pengutronix.de>, Pengutronix
 * (GPL-2.0)
 *
 * Copyright (c) 2024 CTCaer
 *
 * SPDX-License-Identifier:     GPL-2.0+ GPL-2.0
 */

#include <common.h>
#include <fat.h>
#include <linux/ctype.h>
#include <memalign.h>
#include <part.h>
#include "diskio.h"
#include "ff.h"

struct blk_desc *ff_dev = NULL;
disk_partition_t *ff_part = NULL;
static FATFS fat_ff_fs;

/* Functions that call into ff.c */

int fat_set_blk_dev(struct blk_desc *dev, disk_partition_t *part)
{
	FRESULT res;

	debug("%s()\n", __func__);

	/* First close any currently found FAT filesystem */
	if (ff_dev)
		f_mount(NULL, "0:", 1);

	ff_dev = dev;
	ff_part = part;

	res = f_mount(&fat_ff_fs, "0:", 1);
	if (res != FR_OK) {
		debug("f_mount() failed: %d\n", res);
		ff_dev = NULL;
		ff_part = NULL;
		return -1;
	}

	debug("f_mount() succeeded\n");
	return 0;
}

int fat_register_device(struct blk_desc *dev_desc, int part_no)
{
	disk_partition_t info;

	/* First close any currently found FAT filesystem */
	if (ff_dev) {
		f_mount(NULL, "0:", 1);
		ff_dev = NULL;
	}

	/* Read the partition table, if present */
	if (part_get_info(dev_desc, part_no, &info)) {
		if (part_no != 0) {
			printf("** Partition %d not valid on device %d **\n",
					part_no, dev_desc->devnum);
			return -1;
		}

		info.start = 0;
		info.size = dev_desc->lba;
		info.blksz = dev_desc->blksz;
		info.name[0] = 0;
		info.type[0] = 0;
		info.bootable = 0;
#if CONFIG_IS_ENABLED(PARTITION_UUIDS)
		info.uuid[0] = 0;
#endif
	}

	return fat_set_blk_dev(dev_desc, &info);
}

int file_fat_detectfs(void)
{
	FRESULT res;
	TCHAR label[12];
	DWORD vsn;

	debug("%s()\n", __func__);

	if (!ff_dev) {
		printf("No current device\n");
		return 1;
	}

#if defined(CONFIG_IDE) || \
    defined(CONFIG_SATA) || \
    defined(CONFIG_SCSI) || \
    defined(CONFIG_CMD_USB) || \
    defined(CONFIG_MMC)
	printf("Interface:  ");
	switch (ff_dev->if_type) {
	case IF_TYPE_IDE:
		printf("IDE");
		break;
	case IF_TYPE_SATA:
		printf("SATA");
		break;
	case IF_TYPE_SCSI:
		printf("SCSI");
		break;
	case IF_TYPE_ATAPI:
		printf("ATAPI");
		break;
	case IF_TYPE_USB:
		printf("USB");
		break;
	case IF_TYPE_DOC:
		printf("DOC");
		break;
	case IF_TYPE_MMC:
		printf("MMC");
		break;
	default:
		printf("Unknown");
	}

	printf("\n  Device %d: ", ff_dev->devnum);
	dev_print(ff_dev);
#endif

	res = f_getlabel("0:", label, &vsn);
	if (res != FR_OK) {
		debug("f_getlabel() failed: %d\n", res);
		return -1;
	}

	printf("Filesystem:    ");
	switch (fat_ff_fs.fs_type) {
	case FS_FAT12:
		puts("FAT12\n");
		break;
	case FS_FAT16:
		puts("FAT16\n");
		break;
	case FS_FAT32:
		puts("FAT32\n");
		break;
	case FS_EXFAT:
		puts("EXFAT\n");
		break;
	default:
		puts("<<unknown>>\n");
		break;
	}

	printf("Volume label:  ");
	if (!label[0]) {
		puts("<<no label>>\n");
	} else {
		puts(label);
		puts("\n");
	}

	printf("Volume serial: %08x\n", vsn);

	return 0;
}

int fat_exists(const char *filename)
{
	loff_t size;
	int ret;

	debug("%s(filename=%s)\n", __func__, filename);

	ret = fat_size(filename, &size);
	if (ret)
		return 0;

	return 1;
}

int fat_size(const char *filename, loff_t *size)
{
	FRESULT res;
	FILINFO finfo;

	debug("%s(filename=%s)\n", __func__, filename);

	res = f_stat(filename, &finfo);
	if (res != FR_OK) {
		debug("f_stat() failed: %d\n", res);
		return -1;
	}

	*size = finfo.fsize;

	return 0;
}

int fat_read_file(const char *filename, void *buf, loff_t offset, loff_t len,
		  loff_t *actread)
{
	FRESULT res;
	FIL fp;

	debug("%s(filename=%s, offset=%d, len=%d)\n", __func__, filename,
	      (int)offset, (int)len);

	res = f_open(&fp, filename, FA_READ);
	if (res != FR_OK) {
		eprintf("Failed to open %s: %d\n", filename, res);
		return -1;
	}

	if (!len)
		len = f_size(&fp) - offset;

#ifdef CONFIG_FAT_FAST
	if (!f_expand_cltbl(&fp, 0x1000, 0)) {
		eprintf("Failed to expand cltbl\n");
		goto error;
	}
#endif

	res = f_lseek(&fp, offset);
	if (res != FR_OK) {
		eprintf("Failed to seek to %lld: %d\n", offset, res);
		goto error;
	}

#ifdef CONFIG_FAT_FAST
	res = f_read_fast(&fp, buf, len);
#else
	res = f_read(&fp, buf, len, NULL);
#endif
	if (res != FR_OK) {
		eprintf("Failed to read %lldB: %d\n", len, res);
		goto error;
	}


	debug("f_read() read %llu bytes\n", len);
	*actread = len;

#ifdef CONFIG_FAT_FAST
	f_expand_cltbl(&fp, 0, 0);
#endif

	res = f_close(&fp);
	if (res != FR_OK) {
		eprintf("Failed to close %s: %d\n", filename, res);
		return -1;
	}

	return 0;

error:
#ifdef CONFIG_FAT_FAST
	f_expand_cltbl(&fp, 0, 0);
#endif
	f_close(&fp);

	return -1;
}

#ifdef CONFIG_FAT_WRITE
int file_fat_write(const char *filename, void *buf, loff_t offset, loff_t len,
		   loff_t *actwrite)
{
	FRESULT res;
	FIL fp;
	UINT ff_actwrite;

	debug("%s(filename=%s, offset=%d, len=%d)\n", __func__, filename,
	      (int)offset, (int)len);

	res = f_open(&fp, filename, FA_WRITE | FA_OPEN_ALWAYS);
	if (res != FR_OK) {
		eprintf("f_open() failed: %d\n", res);
		goto err;
	}

	res = f_lseek(&fp, offset);
	if (res != FR_OK) {
		eprintf("f_lseek() failed: %d\n", res);
		goto err;
	}

	res = f_write(&fp, buf, len, &ff_actwrite);
	if (res != FR_OK) {
		eprintf("f_write() failed: %d\n", res);
		goto err;
	}
	debug("f_write() wrote %u bytes\n", ff_actwrite);
	*actwrite = ff_actwrite;

	res = f_close(&fp);
	if (res != FR_OK) {
		eprintf("f_close() failed: %d\n", res);
		goto err;
	}

	return 0;

err:
	printf("** Unable to write file %s **\n", filename);
	return -1;
}
#endif

typedef struct {
	struct fs_dir_stream parent;
	struct fs_dirent dirent;
	char fname[256];
	int iter;
} fat_dir;

int file_fat_read(const char *filename, void *buffer, int maxsize)
{
	loff_t actread;
	int ret;

	ret = fat_read_file(filename, buffer, 0, maxsize, &actread);
	if (ret)
		return ret;
	else
		return actread;
}

int fat_opendir(const char *filename, struct fs_dir_stream **dirsp)
{
	FRESULT res;
	DIR ff_dir;
	fat_dir *dir;
	int ret;

	debug("%s(filename=%s)\n", __func__, filename);

	dir = malloc_cache_aligned(sizeof(*dir));
	if (!dir) {
		ret = -ENOMEM;
		goto fail_free_dir;
	}
	memset(dir, 0, sizeof(*dir));

	res = f_opendir(&ff_dir, filename);
	if (res != FR_OK) {
		ret = -ENOENT;
		goto fail_free_dir;
	}
	f_closedir(&ff_dir);

	strncpy(dir->fname, filename, 256);

	*dirsp = (struct fs_dir_stream *)dir;
	return 0;

fail_free_dir:
	printf("** Unable to open directory %s **\n", filename);

	free(dir);
	return ret;
}

int fat_readdir(struct fs_dir_stream *dirs, struct fs_dirent **dentp)
{
	FILINFO fno;
	DIR ff_dir;
	FRESULT res;
	fat_dir *dir = (fat_dir *)dirs;
	struct fs_dirent *dent = &dir->dirent;

	debug("%s()\n", __func__);

	res = f_opendir(&ff_dir, dir->fname);
	if (res != FR_OK) {
		return -ENOENT;
	}

	dir->iter++;
	for (int i = 0; i < dir->iter; i++) {
		res = f_readdir(&ff_dir, &fno);
		if (res != FR_OK || fno.fname[0] == 0) {
			return -ENOENT;
		}
	}

	f_closedir(&ff_dir);

	memset(dent, 0, sizeof(*dent));
	strcpy(dent->name, fno.fname);

	if (fno.fattrib & AM_DIR) {
		dent->type = FS_DT_DIR;
	} else {
		dent->type = FS_DT_REG;
		dent->size = fno.fsize;
	}

	*dentp = dent;

	return 0;
}

void fat_closedir(struct fs_dir_stream *dirs)
{
	free(dirs);
}

void fat_close(void)
{
	debug("%s()\n", __func__);

	if (ff_dev && ff_part)
		f_mount(NULL, "0:", 1);

	ff_dev = NULL;
	ff_part = NULL;
}
