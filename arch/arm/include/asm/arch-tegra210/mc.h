/*
 * Copyright (c) 2014-2015 NVIDIA CORPORATION. All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#ifndef _TEGRA210_MC_H_
#define _TEGRA210_MC_H_

/**
 * Defines the memory controller registers we need/care about
 */
struct mc_ctlr {
	u32 reserved0[4];			/* offset 0x00 - 0x0C */
	u32 mc_smmu_config;			/* offset 0x10 */
	u32 mc_smmu_tlb_config;			/* offset 0x14 */
	u32 mc_smmu_ptc_config;			/* offset 0x18 */
	u32 mc_smmu_ptb_asid;			/* offset 0x1C */
	u32 mc_smmu_ptb_data;			/* offset 0x20 */
	u32 reserved1[3];			/* offset 0x24 - 0x2C */
	u32 mc_smmu_tlb_flush;			/* offset 0x30 */
	u32 mc_smmu_ptc_flush;			/* offset 0x34 */
	u32 reserved2[6];			/* offset 0x38 - 0x4C */
	u32 mc_emem_cfg;			/* offset 0x50 */
	u32 mc_emem_adr_cfg;			/* offset 0x54 */
	u32 mc_emem_adr_cfg_dev0;		/* offset 0x58 */
	u32 mc_emem_adr_cfg_dev1;		/* offset 0x5C */
	u32 reserved3[4];			/* offset 0x60 - 0x6C */
	u32 mc_security_cfg0;			/* offset 0x70 */
	u32 mc_security_cfg1;			/* offset 0x74 */
	u32 reserved4[6];			/* offset 0x7C - 0x8C */
	u32 mc_emem_arb_reserved[28];		/* offset 0x90 - 0xFC */
	u32 reserved5[74];			/* offset 0x100 - 0x224 */
	u32 mc_smmu_translation_enable_0;	/* offset 0x228 */
	u32 mc_smmu_translation_enable_1;	/* offset 0x22C */
	u32 mc_smmu_translation_enable_2;	/* offset 0x230 */
	u32 mc_smmu_translation_enable_3;	/* offset 0x234 */
	u32 mc_smmu_afi_asid;			/* offset 0x238 */
	u32 mc_smmu_avpc_asid;			/* offset 0x23C */
	u32 mc_smmu_dc_asid;			/* offset 0x240 */
	u32 mc_smmu_dcb_asid;			/* offset 0x244 */
	u32 reserved6[2];                       /* offset 0x248 - 0x24C */
	u32 mc_smmu_hc_asid;			/* offset 0x250 */
	u32 mc_smmu_hda_asid;			/* offset 0x254 */
	u32 mc_smmu_isp2_asid;			/* offset 0x258 */
	u32 reserved7[2];                       /* offset 0x25C - 0x260 */
	u32 mc_smmu_msenc_asid;			/* offset 0x264 */
	u32 mc_smmu_nv_asid;			/* offset 0x268 */
	u32 mc_smmu_nv2_asid;			/* offset 0x26C */
	u32 mc_smmu_ppcs_asid;			/* offset 0x270 */
	u32 mc_smmu_sata_asid;			/* offset 0x274 */
	u32 reserved8[1];                       /* offset 0x278 */
	u32 mc_smmu_vde_asid;			/* offset 0x27C */
	u32 mc_smmu_vi_asid;			/* offset 0x280 */
	u32 mc_smmu_vic_asid;			/* offset 0x284 */
	u32 mc_smmu_xusb_host_asid;		/* offset 0x288 */
	u32 mc_smmu_xusb_dev_asid;		/* offset 0x28C */
	u32 reserved9[1];                       /* offset 0x290 */
	u32 mc_smmu_tsec_asid;			/* offset 0x294 */
	u32 mc_smmu_ppcs1_asid;			/* offset 0x298 */
	u32 reserved10[235];			/* offset 0x29C - 0x644 */
	u32 mc_video_protect_bom;		/* offset 0x648 */
	u32 mc_video_protect_size_mb;		/* offset 0x64c */
	u32 mc_video_protect_reg_ctrl;		/* offset 0x650 */
	u32 reserved11[7];			/* offset 0x654 - 0x66C */
	u32 mc_sec_carveout_bom;		/* offset 0x670 */
	u32 mc_sec_carveout_size_mb;		/* offset 0x674 */
	u32 reserved12[192];			/* offset 0x678 - 0x974 */
	u32 mc_video_protect_bom_adr_hi;	/* offset 0x978 */
	u32 reserved13[9];			/* offset 0x97C - 0x99C */
	u32 mc_mts_carveout_bom;		/* offset 0x9A0 */
	u32 mc_mts_carveout_size_mb;		/* offset 0x9A4 */
	u32 mc_mts_carveout_adr_hi;		/* offset 0x9A8 */
	u32 reserved14[10];			/* offset 0x9AC - 0x9D0 */
	u32 mc_sec_carveout_adr_hi;		/* offset 0x9D4 */
	u32 reserved15[141];			/* offset 0x9D8 - 0xC08 */
	u32 mc_security_carveout1_bom;		/* offset 0xC0C */
	u32 mc_security_carveout1_bom_hi;	/* offset 0xC10 */
	u32 mc_security_carveout1_size_128kb;	/* offset 0xC14 */
	u32 reserved16[17];			/* offset 0xC18 - 0xC58 */
	u32 mc_security_carveout2_bom;		/* offset 0xC5C */
	u32 mc_security_carveout2_bom_hi;	/* offset 0xC60 */
	u32 mc_security_carveout2_size_128kb;	/* offset 0xC64 */
	u32 reserved17[17];			/* offset 0xC68 - 0xCA8 */
	u32 mc_security_carveout3_bom;		/* offset 0xCAC */
	u32 mc_security_carveout3_bom_hi;	/* offset 0xCB0 */
	u32 mc_security_carveout3_size_128kb;	/* offset 0xCB4 */
	u32 reserved18[17];			/* offset 0xCB8 - 0xCF8 */
	u32 mc_security_carveout4_bom;		/* offset 0xCFC */
	u32 mc_security_carveout4_bom_hi;	/* offset 0xD00 */
	u32 mc_security_carveout4_size_128kb;	/* offset 0xD04 */
	u32 reserved19[17];			/* offset 0xD08 - 0xD48 */
	u32 mc_security_carveout5_bom;		/* offset 0xD4C */
	u32 mc_security_carveout5_bom_hi;	/* offset 0xD50 */
	u32 mc_security_carveout5_size_128kb;	/* offset 0xD54 */
};

#define TEGRA_MC_SMMU_CONFIG_ENABLE (1 << 0)

#define TEGRA_MC_VIDEO_PROTECT_REG_WRITE_ACCESS_ENABLED		(0 << 0)
#define TEGRA_MC_VIDEO_PROTECT_REG_WRITE_ACCESS_DISABLED	(1 << 0)

#endif	/* _TEGRA210_MC_H_ */
