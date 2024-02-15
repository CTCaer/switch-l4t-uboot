/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2016        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include <common.h>
#include <fat.h>
#include <linux/ctype.h>
#include <memalign.h>
#include <part.h>
#include "diskio.h"
#include "ff.h"

extern struct blk_desc *ff_dev;
extern disk_partition_t *ff_part;

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/
DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
	return 0;
}

/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/
DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
	return 0;
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/
DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
	int ret;
	void *buf;

	if (!ff_dev) return -1;
	debug("%s(sector=%d, count=%u)\n", __func__, sector, count);

	if (sector % 8)
		buf = malloc_cache_aligned(count * ff_dev->blksz);
	else
		buf = (void *)buff;

	ret = blk_dread(ff_dev, ff_part->start + sector, count, buf);
	if (ret != count) {
		if (sector % 8)
			free(buf);
		return RES_ERROR;
	}
	if (sector % 8) {
		memcpy(buff, buf, count * ff_dev->blksz);
		free(buf);
	}

	return RES_OK;
}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/
DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
	int ret;
	void *buf;

	if (!ff_dev) return -1;
	debug("%s(sector=%d, count=%u)\n", __func__, sector, count);

	if (sector % 8) {
		buf = malloc_cache_aligned(count * ff_dev->blksz);
		memcpy(buf, buff, count * ff_dev->blksz);
	} else
		buf = (void *)buff;

	ret = blk_dwrite(ff_dev, ff_part->start + sector, count, buf);
	if (sector % 8)
		free(buf);

	if (ret != count)
		return RES_ERROR;

	return RES_OK;
}

/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/
DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	return RES_OK;
}
