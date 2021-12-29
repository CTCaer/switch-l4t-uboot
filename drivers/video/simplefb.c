/*
 * (C) Copyright 2017 Rob Clark
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <dm.h>
#include <fdtdec.h>
#include <fdt_support.h>
#include <video.h>

static int simple_video_probe(struct udevice *dev)
{
	struct video_uc_platdata *plat = dev_get_uclass_platdata(dev);
	struct video_priv *uc_priv = dev_get_uclass_priv(dev);
	static int enabled_nodes[2] = { -1, -1 };
	const void *blob = gd->fdt_blob;
	const char *format;
	fdt_addr_t base;
	fdt_size_t size;
	int node = -1;
	int fb_no;

	if (enabled_nodes[0] == -1)
		fb_no = 0;
	else if (enabled_nodes[1] == -1)
		fb_no = 1;
	else
		return -EINVAL; /* Reached maximum framebuffers */

	/* Find simple-framebuffer nodes */
	while (1) {
		node = fdt_node_offset_by_compatible(blob, node, "simple-framebuffer");
		if (node < 0) {
			debug("%s: node not found: %s\n", __func__, "simple-framebuffer");
			return -EINVAL;
		}
		if (node != enabled_nodes[!fb_no])
			break;
	}

	base = fdtdec_get_uint64(blob, node, "address", 0);
	if (!base)
		base = fdtdec_get_uint(blob, node, "address", 0);
	size = fdtdec_get_uint(blob, node, "size", 0);
	if (!base || !size) {
		debug("%s: invalid fb base=%llx, size=%llx\n", __func__, base, size);
		return -EINVAL;
	}

	/*
	 * TODO is there some way to reserve the framebuffer
	 * region so it isn't clobbered?
	 */
	plat->base = base;
	plat->size = size;

	debug("%s: base=%llx, size=%llu\n", __func__, base, size);

	video_set_flush_dcache(dev, true);

	debug("%s: Query resolution...\n", __func__);

	uc_priv->xsize = fdtdec_get_uint(blob, node, "width", 0);
	uc_priv->ysize = fdtdec_get_uint(blob, node, "height", 0);
	uc_priv->rot = fdtdec_get_uint(blob, node, "rot", 0);

	format = fdt_getprop(blob, node, "format", NULL);
	debug("%s: %dx%d@%s\n", __func__, uc_priv->xsize, uc_priv->ysize, format);

	if (strcmp(format, "r5g6b5") == 0) {
		uc_priv->bpix = VIDEO_BPP16;
	} else if (strcmp(format, "a8b8g8r8") == 0) {
		uc_priv->bpix = VIDEO_BPP32;
	} else {
		printf("%s: invalid format: %s\n", __func__, format);
		return -EINVAL;
	}

	enabled_nodes[fb_no] = node;

	return 0;
}

static const struct udevice_id simple_video_ids[] = {
	{ .compatible = "simple-framebuffer" },
	{ }
};

U_BOOT_DRIVER(simple_video) = {
	.name	= "simple_video",
	.id	= UCLASS_VIDEO,
	.of_match = simple_video_ids,
	.probe	= simple_video_probe,
	.flags	= DM_FLAG_PRE_RELOC,
};
