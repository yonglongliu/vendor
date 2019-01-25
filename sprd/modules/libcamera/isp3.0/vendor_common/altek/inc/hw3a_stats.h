/******************************************************************************
 * File name: hw3a_stats.h
 * Create Date:
 *
 * Comment:
 * Describe common difinition of HW3A
 *
 ******************************************************************************/
#ifndef _AL_COMMON_TYPE_H_
#define _AL_COMMON_TYPE_H_

#define _AL_HW3A_STATS_VER              (0.02)

/******************************************************************************
 * Include files
 ******************************************************************************/

#include "mtype.h"
#include "frmwk_hw3a_event_type.h"
#include <sys/time.h>   /* for timestamp calling */

/******************************************************************************
 *  Macro definitions
 ******************************************************************************/

#define AL_MAX_ROI_NUM                  (5)
#define AL_MAX_EXP_ENTRIES              (11)	/* max exposure entries */
#define AL_MAX_AWB_STATS_NUM            (3072)	/* 64 x 48 total blocks */
#define AL_MAX_AE_STATS_NUM             (256)	/* 16 x 16 total blocks */
#define AL_MAX_AF_STATS_NUM             (480)	/* 16 x 30 total blocks */
#define AL_MAX_HIST_NUM                 (256)	/* 256, 8 bits value array */

#define AL_AE_HW3A_BIT_DEPTH            (10)	/* bit depth for AE algo of HW3A stats, usually 10 */
#define AL_AWB_HW3A_BIT_DEPTH           (8)	/* bit depth for AWB algo of HW3A stats, usually 8 */

#define AL_AE_SENSORTEST_MAXNODE        (4)    /* max:4 sets of exposure param */
/* for wrapper */
#define HW3A_MAX_DLSQL_NUM              (20)	/* maximun download sequence list number */

#define HW3A_MAGIC_NUMBER_VERSION       (0x00110113)
#define HW3A_VERSION_NUMBER_TYPEID       (0x00000000)    /*  for main ISP stats  */
#define HW3A_VERSION_NUMBER_AF_TYPEID       (0x01000000)    /*  for AF stats only for AF line meet Interrupt */

#define HW3A_METADATA_SIZE              HW3A_MAX_TOTAL_STATS_BUFFER_SIZE

/* Define for suggested single stats buffer size, including stats info */
/* This including each A stats info after A tag, take AE for exsample,
 * including  udPixelsPerBlocks/udbanksize/ucvalidblocks
 */
/* ucvalidbanks/8-align Dummy */
#define HW3A_MAX_AE_STATS_BUFFER_SIZE           ( HW3A_AE_STATS_BUFFER_SIZE + 64 )
#define HW3A_MAX_AWB_STATS_BUFFER_SIZE          ( HW3A_AWB_STATS_BUFFER_SIZE + 64 )
#define HW3A_MAX_AF_STATS_BUFFER_SIZE           ( HW3A_AF_STATS_BUFFER_SIZE + 64 )
#define HW3A_MAX_ANTIF_STATS_BUFFER_SIZE        ( HW3A_ANTIF_STATS_BUFFER_SIZE + 64 )
#define HW3A_MAX_YHIST_BUFFER_SIZE              ( HW3A_YHIST_STATS_BUFFER_SIZE + 64 )
#define HW3A_MAX_SUBIMG_BUFFER_SIZE             ( HW3A_SUBIMG_STATS_BUFFER_SIZE + 64 )

#define HW3A_MAX_TOTAL_STATS_BUFFER_SIZE        ( 230*1024 )

/* Define for MAX buffer of single stats */
#define HW3A_AE_STATS_BUFFER_SIZE               ( 31*1024  + 128 )
#define HW3A_AWB_STATS_BUFFER_SIZE              ( (26 + 48)*1024 + 128 )
#define HW3A_AF_STATS_BUFFER_SIZE               ( 19*1024 + 128 )
#define HW3A_ANTIF_STATS_BUFFER_SIZE            ( 10*1024 + 128 )
#define HW3A_YHIST_STATS_BUFFER_SIZE            ( 1*1024 + 128 )
#define HW3A_SUBIMG_STATS_BUFFER_SIZE           ( 96000 + 128)

/* for wrapper */
#define HW3A_MAX_FRMWK_AE_BLOCKS                AL_MAX_AE_STATS_NUM
#define HW3A_MAX_FRMWK_AF_BLOCKS                (9)
//#define IIR_PARAM_ENABLE


/* Define AE IQ info reserved size*/
#define MAX_AE_IQ_INFO_RESERVED_SIZE            (16)
/******************************************************************************
 * Static declarations
 ******************************************************************************/


/******************************************************************************
 * Type declarations
 ******************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif

/*
 *@typedef ae_flash_st_t
 *@brief Flash (HW) status (off/on)
 */
enum raw_bayer_order {
	BAYER_ORDER_RG = 0,	/* RGGB */
	BAYER_ORDER_GR = 1,	/* GRBG */
	BAYER_ORDER_GB = 2,	/* GBRB */
	BAYER_ORDER_BG = 3,	/* BGGR */
} ;

enum al3a_hw3a_dev_id_t {
	AL3A_HW3A_DEV_ID_A_0 = 0,
	AL3A_HW3A_DEV_ID_B_0 = 1,
	AL3A_HW3A_DEV_ID_TOTAL
};

/*
 * @struct 3A Download Sequence
 * @brief 3A Download Sequence
 */
enum hw3actrl_b_dl_seq_t {
	HA3ACTRL_B_DL_TYPE_AF = 1,
	HA3ACTRL_B_DL_TYPE_AE = 2,
	HA3ACTRL_B_DL_TYPE_AWB = 3,
	HA3ACTRL_B_DL_TYPE_YHIS = 4,
	HA3ACTRL_B_DL_TYPE_AntiF = 1,
	HA3ACTRL_B_DL_TYPE_Sub = 2,
	HA3ACTRL_B_DL_TYPE_NONE = 10,
};

/*
 * @struct 3A Download Sequence
 * @brief 3A Download Sequence
 */
enum hw3actrl_a_dl_seq_t {
	HA3ACTRL_A_DL_TYPE_AntiF = 1,
	HA3ACTRL_A_DL_TYPE_Sub = 2,
	HA3ACTRL_A_DL_TYPE_W9_ALL = 3,
} ;

/*
 * @typedef MID_mode
 * @brief mid mode
 */
enum alhw3a_mid_mode {
	HW3A_MF_51_MODE,
	HW3A_MF_31_MODE,
	HW3A_MF_DISABLE,
};

enum alisp_opmode_idx_t {
	OPMODE_NORMALLV = 0,
	OPMODE_AF_FLASH_AF,
	OPMODE_FLASH_AE,
	OPMODE_MAX,
};


/******************************************************************************
 *  Framework related declaration
 ******************************************************************************/
#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct rect_roi_t {
	uint32 w;
	uint32 h;
	uint32 left;
	uint32 top;
};
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct rect_roi_wt_t {
	struct rect_roi_t roi;
	uint32 weight;		/* unit: 1000, set 1000 to be default */
};
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct rect_roi_config_t {
	struct rect_roi_wt_t roi[AL_MAX_ROI_NUM];
	uint16  roi_count;		/* total valid ROI region numbers */
	uint16  ref_frame_width;
	uint16  ref_frame_height;
} ;
#pragma pack(pop)  /* restore old alignment setting from stack */


#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct al_crop_info_t {
	uint32 output_w;		/* final image out width (scaled) */
	uint32 output_h;		/* final image out height (scaled) */

	uint32 input_width;		/* image width before ISP pipeline */
	uint32 input_height;		/* image height before ISP pipeline */
	uint32 input_xoffset;		/* image valid start x coordinate */
	uint32 input_yoffset;		/* image valid start y coordinate */

	uint32 relay_out_width;		/* relay image width after some ISP pipeline */
	uint32 relay_out_height;	/* relay image height after some ISP pipeline */
	uint32 relay_out_xoffset;	/* relay image start x coordinate */
	uint32 relay_out_yoffset;	/* relay image start y coordinate */

};
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct raw_info {
	uint16     sensor_raw_w;
	uint16     sensor_raw_h;
	struct  rect_roi_t           sensor_raw_valid_roi;
	enum    color_order_mode_t   raw_color_order;
	uint32     line_time;		/* unit: nano-second */
} ;
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct ae_hw_config_t {
	struct raw_info   sensor_raw_info;
	uint32  hw_h_div;
	uint32  hw_v_div;
	uint8  stripemode;		/* calculate half size of sensor info of w */
	uint8  interlaceMode;		/* calculate half size of sensor info of h */
};
#pragma pack(pop)  /* restore old alignment setting from stack */

/* Below for HW3A wrapper used */
#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct rgb_gain_t {
	float r;
	float g;
	float b;
};
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct calib_wb_gain_t {
	uint32 r;	/* base unit: 1000 --> 1.0x */
	uint32 g;	/* base unit: 1000 --> 1.0x */
	uint32 b;	/* base unit: 1000 --> 1.0x */
	uint32 minISO;    /* minimun ISO, from calibration */
};
#pragma pack(pop)  /* restore old alignment setting from stack */

/******************************************************************************
 *  HW3A engine related declaration
 ******************************************************************************/
#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct alhw3a_statisticsdldregion_t {
	uint16 uwborderratiox;
	uint16 uwborderratioy;
	uint16 uwblknumx;
	uint16 uwblknumy;
	uint16 uwoffsetratiox;
	uint16 uwoffsetratioy;

};
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct alhw3a_ae_cfginfo_t {
	uint16 tokenid;
	struct alhw3a_statisticsdldregion_t taeregion;
};
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct alhw3a_awb_histogram_t {
	uint8       benable;			/* Enable HW3A Histogram */
	sint8       ccrstart;			/* Histogram Cr Range start */
	sint8       ccrend;			/* Histogram Cr Range End */
	sint8       coffsetup;			/* Histogram Offset Range Up */
	sint8       coffsetdown;		/* Histogram Offset Range Down */
	sint8       ccrpurple;			/* Purple compensation */
	uint8       ucoffsetpurple;		/* Purple compensation */
	sint8       cgrassoffset;		/* Grass compensation--offset */
	sint8       cgrassstart;		/* Grass compensation--cr left */
	sint8       cgrassend;			/* Grass compensation--cr right */
	uint8       ucdampgrass;		/* Grass compensation */
	sint8       coffset_bbr_w_start;	/* Weighting around bbr */
	sint8       coffset_bbr_w_end;		/* Weighting around bbr */
	uint8       ucyfac_w;			/* Weighting around bbr */
	uint32      dhisinterp;

};
#pragma pack(pop)  /* restore old alignment setting from stack */

/*
 * @typedef alHW3a_AF_StatisticsDldRegion
 * @brief AF Statistics download region
 */
#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct alhw3a_af_statisticsdldregion_t {
	uint16 uwsizeratiox;			/* used to control ROI of width,
						 * if set 50 means use half image to get HW3A stats data */
	uint16 uwsizeratioy;			/* used to control ROI of height,
						 * if set 50 means use half image to get HW3A stats data */
	uint16 uwblknumx;			/* block number of horizontal direction of ROI */
	uint16 uwblknumy;			/* block number of vertical direction of ROI */
	uint16 uwoffsetratiox;			/* used to control ROI shift position ratio of width,
						 * if set 1 means offset 1% width from start horizontal position, suggest 0 */
	uint16 uwoffsetratioy;			/* used to control ROI shift position ratio of height,
						 * if set 1 means offset 1% width from start horizontal position, suggest 0 */

};
#pragma pack(pop)  /* restore old alignment setting from stack */

/*
 * @typedef alhw3a_af_lpf_t
 * @brief AF LPF config info
 */
#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct alhw3a_af_lpf_t {
	uint32 udb0;
	uint32 udb1;
	uint32 udb2;
	uint32 udb3;
	uint32 udb4;
	uint32 udb5;
	uint32 udb6;
	uint32 udshift;
};
#pragma pack(pop)  /* restore old alignment setting from stack */

/*
 * @typedef alhw3a_af_iir_t
 * @brief AF IIR config info
 */
#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct alhw3a_af_iir_t {
	uint8   binitbyuser;
	uint32 udp;
	uint32 udq;
	uint32 udr;
	uint32 uds;
	uint32 udt;
	uint32 udabsshift;
	uint32 udth;
	uint32 udinita;
	uint32 udinitb;
};
#pragma pack(pop)  /* restore old alignment setting from stack */

/*
 * @typedef alhw3a_af_pseudoy_t
 * @brief AF PseudoY config info
 */
#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct alhw3a_af_pseudoy_t {
	uint32 udwr;
	uint32 udwgr;
	uint32 udwgb;
	uint32 udwb;
	uint32 udshift;
	uint32 udoffset;
	uint32 udgain;
};
#pragma pack(pop)  /* restore old alignment setting from stack */

/*
 * @typedef alhw3a_af_cfginfo_t
 * @brief AF configuration information
 */
#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct alhw3a_af_cfginfo_t {
	uint16 tokenid;
	struct alhw3a_af_statisticsdldregion_t tafregion;
	uint8  benableaflut;
	uint16 auwlut[259];
	uint16 auwaflut[259];
	uint8 aucweight[6];
	uint16 uwsh;
	uint8 ucthmode;
	uint8 aucindex[82];
	uint16 auwth[4];
	uint16 pwtv[4];
	uint32 udafoffset;
	uint8 baf_py_enable;
	uint8 baf_lpf_enable;
	enum alhw3a_mid_mode nfiltermode;
	uint8 ucfilterid;
	uint16 uwlinecnt;
#ifdef IIR_PARAM_ENABLE
	struct alhw3a_af_lpf_t tlpf;
	struct alhw3a_af_iir_t atiir[2];
	struct alhw3a_af_pseudoy_t tpseudoy;
#endif
};
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct alhw3a_awb_cfginfo_t {
	uint8                           tokenid;
	struct alhw3a_statisticsdldregion_t    tawbregion;		/* AWB Stat ROI region */
	uint8                           ucyfactor[16];		/* ALTEK format for Y weighting */
	sint8                           bbrfactor[33];		/* ALTEK format for CC data */
	uint16                          uwrgain;		/* ALTEK format of calibration data */
	uint16                          uwggain;		/* ALTEK format of calibration data */
	uint16                          uwbgain;		/* ALTEK format of calibration data */
	uint8                           uccrshift;		/* Cr shift for stat data */
	uint8                           ucoffsetshift;		/* offset shift for stat data */
	uint8                           ucquantize;		/* Set ucQuantize = 0 (fixed)
								 * since crs = Cr- [(cbs+awb_quantize_damp)>>awb_damp] */
	uint8                           ucdamp;			/* Set ucDamp = 7 (fixed)
								 * since crs = Cr- [(cbs+awb_quantize_damp)>>awb_damp] */
	uint8                           ucsumshift;		/* Sum shift = 5 (fixed), based on ISP sampling points.
								 * [9:0]G' = (sum(G[i,j]/2) + 2^(awb_sum_shift-1)) >> awb_sum_shift */
	struct alhw3a_awb_histogram_t   tawbhis;
	uint16                          uwrlineargain;		/* calib_r_gain with foramt scale 128, normalized by g */
	uint16                          uwblineargain;		/* calib_b_gain with foramt scale 128, normalized by g */

};
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct alhw3a_flicker_cfginfo_t {
	uint16      tokenid;
	uint16      uwoffsetratiox;
	uint16      uwoffsetratioy;

};
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct alhw3a_yhist_cfginfo_t {
	uint16 tokenid;
	struct alhw3a_statisticsdldregion_t tyhistregion;
};
#pragma pack(pop)  /* restore old alignment setting from stack */

/** HW3A stats info of packed data
\Total size should refer to define of HW3A_MAX_TOTAL_STATS_BUFFER_SIZE
\Here only partial stats info, not including stats data
*/
#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct isp_drv_meta_t {
	/* Common info */
	uint32 umagicnum;		/* HW3A FW magic number */
	uint16 uhwengineid;		/* HW3A engine ID, used for 3A behavior */
	uint16 uframeidx;		/* HW3a_frame_idx */
	uint32 uchecksum;		/* check sum for overall stats data */

	/* AE */
	uint16 uaetag;
	uint16 uaetokenid;		/* valid setting number, which same when calling XXX_setAECfg,
					 * for 3A libs synchronization */
	uint32 uaestatssize;		/* AE stats size, including AE stat info */
	uint32 uaestatsaddr;		/* offset, start from pointer */

	/* AWB */
	uint16 uawbtag;
	uint16 uawbtokenid;
	uint32 uawbstatssize;		/* AWB stats size, including AWB stat info */
	uint32 uawbstatsaddr;		/* offset, start from pointer */

	/* AF */
	uint16 uaftag;
	uint16 uaftokenid;
	uint32 uafstatssize;		/* AF stats size, including AF stat info */
	uint32 uafstatsaddr;		/* offset, start from pointer */

	/* Y-hist */
	uint16 uyhisttag;
	uint16 uyhisttokenid;
	uint32 uyhiststatssize;		/* Yhist stats size, including Yhist stat info */
	uint32 uyhiststatsaddr;		/* offset, start from pointer */

	/* anti-flicker */
	uint16 uantiftag;
	uint16 uantiftokenid;
	uint32 uantifstatssize;		/* anti-flicker stats size, including anti-flicker stat info */
	uint32 uantifstatsaddr;		/* offset, start from pointer */

	/* sub-sample */
	uint16 usubsampletag;
	uint16 usubsampletokenid;
	uint32 usubsamplestatssize;	/* subsample size, including subsample image info */
	uint32 usubsamplestatsaddr;	/* offset, start from pointer */

};
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct isp_drv_meta_ae_stats_info_t {
	/* AE stats info */
	uint32 udpixelsperblocks;
	uint32 udbanksize;
	uint8  ucvalidblocks;
	uint8  ucvalidbanks;
};
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct isp_drv_meta_ae_t {
	/* Common info */
	uint32 umagicnum;
	uint16 uhwengineid;
	uint16 uframeidx;		/* HW3a_frame_idx */

	/* AE info */
	uint8  b_isstats_byaddr;   // 0: use pae_stats, 1: use puc_ae_stats
	uint8  pae_stats[HW3A_AE_STATS_BUFFER_SIZE];
	uint8  *puc_ae_stats;
	uint16 uaetokenid;
	uint32 uaestatssize;
	uint16 upseudoflag;		/* 0: normal stats, 1: PseudoFlag flag
					 * (for lib, smoothing/progressive run) */

	/* framework time/frame idx info */
	struct timeval systemtime;
	uint32 udsys_sof_idx;
	uint32 udisp_dgain;

	struct isp_drv_meta_ae_stats_info_t  ae_stats_info;

};
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct isp_drv_meta_awb_stats_info_t {
	/* AWB stats info */
	uint32 udpixelsperblocks;
	uint32 udbanksize;
	uint16 uwsub_x;
	uint16 uwsub_y;
	uint16 uwwin_x;
	uint16 uwwin_y;
	uint16 uwwin_w;
	uint16 uwwin_h;
	uint16 uwtotalcount;
	uint16 uwgrasscount;
	uint8  ucvalidblocks;
	uint8  ucvalidbanks;
	uint8  ucRGBFmt;
};
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct isp_drv_meta_awb_t {
	/* Common info */
	uint32 umagicnum;
	uint16 uhwengineid;
	uint16 uframeidx;		/* HW3a_frame_idx */

	/* AWB info */
	uint8  b_isstats_byaddr;   // 0: use pae_stats, 1: use puc_ae_stats
	uint8  pawb_stats[HW3A_AWB_STATS_BUFFER_SIZE];
	uint8  *puc_awb_stats;
	uint16 uawbtokenid;
	uint32 uawbstatssize;
	uint16 upseudoflag;		/* 0: normal stats, 1: PseudoFlag flag
					 * (for lib, smoothing/progressive run) */

	/* framework time/frame idx info */
	struct timeval systemtime;
	uint32 udsys_sof_idx;

	struct isp_drv_meta_awb_stats_info_t awb_stats_info;

};
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct isp_drv_meta_af_stats_info_t {
	/* AF stats info */
	uint32 udpixelsperblocks;
	uint32 udbanksize;
	uint8  ucvalidblocks;
	uint8  ucvalidbanks;
} ;
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct isp_drv_meta_af_t {
	/* Common info */
	uint32 umagicnum;
	uint16 uhwengineid;
	uint16 uframeidx;		/* HW3a_frame_idx */

	/* AF info */
	uint8  b_isstats_byaddr;   // 0: use pae_stats, 1: use puc_ae_stats
	uint8  paf_stats[HW3A_AF_STATS_BUFFER_SIZE];
	uint8  *puc_af_stats;
	uint16 uaftokenid;
	uint32 uafstatssize;
	uint16 upseudoflag;		/* 0: normal stats, 1: PseudoFlag flag */

	/* framework time/frame idx info */
	struct timeval systemtime;
	uint32 udsys_sof_idx;

	struct isp_drv_meta_af_stats_info_t af_stats_info;

};
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct isp_drv_meta_yhist_t {
	/* Common info */
	uint32 umagicnum;
	uint16 uhwengineid;
	uint16 uframeidx;		/* HW3a_frame_idx */

	/* YHist info */
	uint8  b_isstats_byaddr;   // 0: use pae_stats, 1: use puc_ae_stats
	uint8  pyhist_stats[HW3A_YHIST_STATS_BUFFER_SIZE];
	uint8  *puc_yhist_stats;
	uint16 uyhisttokenid;
	uint32 uyhiststatssize;

	/* framework time/frame idx info */
	struct timeval systemtime;
	uint32 udsys_sof_idx;

};
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct isp_drv_meta_antif_t {
	/* Common info */
	uint32 umagicnum;
	uint16 uhwengineid;
	uint16 uframeidx;               /* HW3a_frame_idx */

	/* anti-flicker info */
	uint8  b_isstats_byaddr;   // 0: use pae_stats, 1: use puc_ae_stats
	uint8  pantif_stats[HW3A_ANTIF_STATS_BUFFER_SIZE];
	uint8  *puc_antif_stats;
	uint16 uantiftokenid;
	uint32 uantifstatssize;

	/* framework time/frame idx info */
	struct timeval systemtime;
	uint32 udsys_sof_idx;

};
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct isp_drv_meta_subsample_stats_info_t {
	/* Subsample stats info */
	uint32  udbuffertotalsize;
	uint16  ucvalidw;
	uint16  ucvalidh;
};
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct isp_drv_meta_subsample_t {
	/* Common info */
	uint32 umagicnum;
	uint16 uhwengineid;
	uint16 uframeidx;		/* HW3a_frame_idx */

	/* Subsample info */
	uint8  b_isstats_byaddr;   // 0: use pae_stats, 1: use puc_ae_stats
	uint8  psubsample_stats[HW3A_SUBIMG_STATS_BUFFER_SIZE];
	uint8  *puc_subsample_stats;
	uint16 usubsampletokenid;
	uint32 usubsamplestatssize;

	/* framework time/frame idx info */
	struct timeval systemtime;
	uint32 udsys_sof_idx;

	struct isp_drv_meta_subsample_stats_info_t subsample_stats_info;

};
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct alisp_dldsequence_t {
	uint8 ucahbsensoreid;
	uint8 ucissingle3amode;
	uint8 ucpreview_baisc_dldseqlength;
	uint8 ucpreview_adv_dldseqlength;
	uint8 ucfastconverge_baisc_dldseqlength;
	uint8 aucpreview_baisc_dldseq[HW3A_MAX_DLSQL_NUM];
	uint8 aucpreview_adv_dldseq[HW3A_MAX_DLSQL_NUM];
	uint8 aucfastconverge_baisc_dldseq[HW3A_MAX_DLSQL_NUM];
};
#pragma pack(pop)  /* restore old alignment setting from stack */


/* For AE wrapper of HW3A stats */
#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct al3awrapper_ae_wb_gain_t {
	uint32 r;
	uint32 g;
	uint32 b;
};
#pragma pack(pop)  /* restore old alignment setting from stack */

/* For AE wrapper of HW3A stats */
#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct al3awrapper_stats_ae_t {
	uint32  ustructuresize;		/* here for confirmation */

	/* Input */
	/* WB info */
	struct rgb_gain_t stat_wb_2y;		/* record WB when translate Y from R/G/B */

	/* Output */
	/* Common info */
	uint32 umagicnum;
	uint16 uhwengineid;
	uint16 uframeidx;		/* HW3a_frame_idx */

	/* AE info */
	uint16 uaetokenid;
	uint32 uaestatssize;

	/* AE stats info */
	uint32 udpixelsperblocks;
	uint32 udbanksize;
	uint8  ucvalidblocks;
	uint8  ucvalidbanks;
	uint8  ucstatsdepth;		/* 8: 8 bits, 10: 10 bits */

	uint16 upseudoflag;		/* 0: normal stats, 1: PseudoFlag flag
					*(for lib, smoothing/progressive run) */

	/* framework time/frame idx info */
	struct timeval systemtime;
	uint32 udsys_sof_idx;
	uint32 udisp_dgain;

	/* AE stats */
	uint8  bisstatsbyaddr;      /* true: use addr to passing stats, flase: use array define */
	uint32 statsy[AL_MAX_AE_STATS_NUM];
	uint32 statsr[AL_MAX_AE_STATS_NUM];
	uint32 statsg[AL_MAX_AE_STATS_NUM];
	uint32 statsb[AL_MAX_AE_STATS_NUM];
	void* ptstatsy;			/* store stats Y, each element should be uint32, 256 elements */
	void* ptstatsr;			/* store stats R, each element should be uint32, 256 elements */
	void* ptstatsg;			/* store stats G, each element should be uint32, 256 elements */
	void* ptstatsb;			/* store stats B, each element should be uint32, 256 elements */

};
#pragma pack(pop)  /* restore old alignment setting from stack */

/* For Flicker wrapper of HW3A stats */
#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct al3awrapper_stats_flicker_t {
	uint32  ustructuresize;    /* here for confirmation */

	/* Output */
	/* Common info */
	uint32 umagicnum;
	uint16 uhwengineid;
	uint16 uframeidx;		/* HW3a_frame_idx */

	/* anti-flicker info */
	uint8  pantif_stats[HW3A_ANTIF_STATS_BUFFER_SIZE];
	uint16 uantiftokenid;
	uint32 uantifstatssize;

	/* framework time/frame idx info */
	struct timeval systemtime;
	uint32 udsys_sof_idx;

};
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct al3awrapper_stats_awb_t {
	/* Common info */
	uint32  umagicnum;
	uint16  uhwengineid;
	uint16  uframeidx;		/* HW3a_frame_idx */

	/* AWB info */
	uint8   *pawb_stats;
	uint16  uawbtokenid;
	uint32  uawbstatssize;
	uint16  upseudoflag;		/* 0: normal stats, 1: PseudoFlag flag
					* (for lib, smoothing/progressive run) */

	/* framework time/frame idx info */
	struct timeval systemtime;
	uint32 udsys_sof_idx;

	/* AWB stats info */
	uint32  udpixelsperblocks;
	uint32  udbanksize;
	uint8   ucvalidblocks;
	uint8   ucvalidbanks;
	uint8   ucstatsdepth;		/* 8: 8 bits, 10: 10 bits */

	uint8   ucstats_format;		/* 0: No RGB output, 1: 10-bit RGB */

	uint32 statsr[AL_MAX_AWB_STATS_NUM];
	uint32 statsg[AL_MAX_AWB_STATS_NUM];
	uint32 statsb[AL_MAX_AWB_STATS_NUM];
};
#pragma pack(pop)  /* restore old alignment setting from stack */

/* For YHist wrapper of HW3A stats */
#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct al3awrapper_stats_yhist_t {
	uint32  ustructuresize;		/* here for confirmation */
	/* Common info */
	uint32 umagicnum;
	uint16 uhwengineid;
	uint16 uframeidx;		/* HW3a_frame_idx */
	/* yhist info */
	uint16 u_yhist_tokenid;
	uint32 u_yhist_statssize;
	/* framework time/frame idx info */
	struct timeval systemtime;
	uint32 udsys_sof_idx;
	/* yhist stats */
	uint8  b_is_stats_byaddr;      /* true: use addr to passing stats, flase: use array define */
	uint32 hist_y[AL_MAX_HIST_NUM];  /* Y histogram accumulate pixel number */
	void* pt_hist_y;			/* store stats Y, each element should be uint32, 256 elements */
};
#pragma pack(pop)  /* restore old alignment setting from stack */

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif /* _AL_COMMON_TYPE_H_ */
