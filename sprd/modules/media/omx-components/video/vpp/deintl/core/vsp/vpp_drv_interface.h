/******************************************************************************
 ** File Name:      vsp_drv_sc8830.h                                         *
 ** Author:         Xiaowei.Luo                                               *
 ** DATE:           02/26/2015                                                *
 ** Copyright:      2015 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    														  *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 02/26/2015    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
#ifndef _VSP_DRV_SC8830_H_
#define _VSP_DRV_SC8830_H_

/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "vsp_deint.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/ioctl.h>

/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C"
{
#endif

/*----------------------------------------------------------------------------*
**                            Macro Definitions                               *
**---------------------------------------------------------------------------*/
/*
	Bit define, for video
*/
#define V_BIT_0               0x00000001
#define V_BIT_1               0x00000002
#define V_BIT_2               0x00000004
#define V_BIT_3               0x00000008
#define V_BIT_4               0x00000010
#define V_BIT_5               0x00000020
#define V_BIT_6               0x00000040
#define V_BIT_7               0x00000080
#define V_BIT_8               0x00000100
#define V_BIT_9               0x00000200
#define V_BIT_10              0x00000400
#define V_BIT_11              0x00000800
#define V_BIT_12              0x00001000
#define V_BIT_13              0x00002000
#define V_BIT_14              0x00004000
#define V_BIT_15              0x00008000
#define V_BIT_16              0x00010000
#define V_BIT_17              0x00020000
#define V_BIT_18              0x00040000
#define V_BIT_19              0x00080000
#define V_BIT_20              0x00100000
#define V_BIT_21              0x00200000
#define V_BIT_22              0x00400000
#define V_BIT_23              0x00800000
#define V_BIT_24              0x01000000
#define V_BIT_25              0x02000000
#define V_BIT_26              0x04000000
#define V_BIT_27              0x08000000
#define V_BIT_28              0x10000000
#define V_BIT_29              0x20000000
#define V_BIT_30              0x40000000
#define V_BIT_31              0x80000000

/* ------------------------------------------------------------------------
** Constants
** ------------------------------------------------------------------------ */
#define SPRD_VSP_DRIVER "/dev/sprd_vsp"
#define SPRD_VSP_MAP_SIZE 0xA000            // 40kbyte

#define SPRD_MAX_VSP_FREQ_LEVEL 3
#define SPRD_VSP_FREQ_BASE_LEVEL 0

#define SPRD_VSP_IOCTL_MAGIC 	'm'
#define VSP_CONFIG_FREQ 		_IOW(SPRD_VSP_IOCTL_MAGIC, 1, unsigned int)
#define VSP_GET_FREQ    			_IOR(SPRD_VSP_IOCTL_MAGIC, 2, unsigned int)
#define VSP_ENABLE      			_IO(SPRD_VSP_IOCTL_MAGIC, 3)
#define VSP_DISABLE     			_IO(SPRD_VSP_IOCTL_MAGIC, 4)
#define VSP_ACQUAIRE    			_IO(SPRD_VSP_IOCTL_MAGIC, 5)
#define VSP_RELEASE     			_IO(SPRD_VSP_IOCTL_MAGIC, 6)
#define VSP_COMPLETE       			_IO(SPRD_VSP_IOCTL_MAGIC, 7)
#define VSP_RESET       			_IO(SPRD_VSP_IOCTL_MAGIC, 8)
#define VSP_HW_INFO                  _IO(SPRD_VSP_IOCTL_MAGIC, 9)
#define VSP_VERSION                  _IO(SPRD_VSP_IOCTL_MAGIC, 10)
#define VSP_GET_IOVA                 _IOWR(SPRD_VSP_IOCTL_MAGIC, 11, VSP_IOMMU_MAP_DATA_T)
#define VSP_FREE_IOVA                _IOW(SPRD_VSP_IOCTL_MAGIC, 12, VSP_IOMMU_MAP_DATA_T)
#define  VSP_GET_IOMMU_STATUS _IO(SPRD_VSP_IOCTL_MAGIC, 13)
#define VSP_SET_CODEC_ID    _IOW(SPRD_VSP_IOCTL_MAGIC, 14, unsigned int)
#define VSP_GET_CODEC_COUNTER    _IOWR(SPRD_VSP_IOCTL_MAGIC, 15, unsigned int)
#define VSP_SET_SCENE                _IO(SPRD_VSP_IOCTL_MAGIC, 16)
#define VSP_GET_SCENE                _IO(SPRD_VSP_IOCTL_MAGIC, 17)


#define TIME_OUT_CLK			0xffff
#define TIME_OUT_CLK_FRAME		0x7fffff

/* -----------------------------------------------------------------------
** VSP versions
** ----------------------------------------------------------------------- */
enum {
	VSP_H264_DEC = 0,
	VSP_H264_ENC,
	VSP_MPEG4_DEC,
	VSP_MPEG4_ENC,
	VSP_H265_DEC ,
	VSP_H265_ENC,
	VSP_VPX_DEC,
	VSP_ENC,
	VSP_CODEC_INSTANCE_COUNT_MAX
};

typedef enum
{
    SCENE_NORMAL     = 0,
    SCENE_VOLTE         = 1,
    SCENE_WFD            = 2,
    MAX_SCENE_MODE,
}
VSP_SCENE_MODE_E;

typedef enum
{
    SHARK = 0,
    DOLPHIN = 1,
    TSHARK = 2,
    SHARKL = 3,
    PIKE = 4,
    PIKEL = 5,
    SHARKL64 = 6,
    SHARKLT8 = 7,
    WHALE = 8,
    WHALE2 = 9,
    IWHALE2 = 10,
    ISHARKL2 = 11,
    SHARKL2 = 12,
    MAX_VERSIONS,
}
VSP_VERSION_E;

/* -----------------------------------------------------------------------
** Standard Types
** ----------------------------------------------------------------------- */
#define STREAM_ID_H263				0
#define STREAM_ID_MPEG4				1
#define STREAM_ID_VP8				2
#define STREAM_ID_FLVH263			3
#define STREAM_ID_H264				4
#define STREAM_ID_REAL8				5
#define STREAM_ID_REAL9				6
#define STREAM_ID_H265				7
#define	SHARE_RAM_SIZE 0x100

#define ENC 1
#define DEC 0
/* -----------------------------------------------------------------------
** Control Register Address on ARM
** ----------------------------------------------------------------------- */
#define	SHARE_RAM_SIZE 0x100
#define	GLB_REG_SIZE 0x200
#define	PPA_SLICE_INFO_SIZE 0x200
#define	DCT_IQW_TABLE_SIZE 0x400
#define	FRAME_ADDR_TABLE_SIZE 0x200
#define	VLC_TABLE0_SIZE 2624
#define	VLC_TABLE1_SIZE 1728
#define	BSM_CTRL_REG_SIZE 0x1000

#define	VSP_REG_BASE_ADDR 0x60900000//AHB

#define	AHB_CTRL_BASE_ADDR VSP_REG_BASE_ADDR+0x0
#define	SHARE_RAM_BASE_ADDR VSP_REG_BASE_ADDR+0x200
#define	OR_BOOTROM_BASE_ADDR VSP_REG_BASE_ADDR+0x400
#define	GLB_REG_BASE_ADDR (VSP_REG_BASE_ADDR+0x1000)
#define	PPA_SLICE_INFO_BASE_ADDR (VSP_REG_BASE_ADDR+0x1200)
#define	DCT_IQW_TABLE_BASE_ADDR (VSP_REG_BASE_ADDR+0x1400)
#define	FRAME_ADDR_TABLE_BASE_ADDR (VSP_REG_BASE_ADDR+0x1800)
#define	CABAC_CONTEXT_BASE_ADDR (VSP_REG_BASE_ADDR+0x1a00)
#define	VLC_TABLE0_BASE_ADDR (VSP_REG_BASE_ADDR+0x2000)
#define	VLC_TABLE1_BASE_ADDR (VSP_REG_BASE_ADDR+0x3000)
#define   EIS_GRID_DATA_TABLE (VSP_REG_BASE_ADDR+0x7000)
#define	BSM_CTRL_REG_BASE_ADDR (VSP_REG_BASE_ADDR+0x8000)
#define	BSM1_CTRL_REG_BASE_ADDR (VSP_REG_BASE_ADDR+0x8100)

//ahb ctrl
#define ARM_ACCESS_CTRL_OFF 0x0
#define ARM_ACCESS_STATUS_OFF 0x04
#define MCU_CTRL_SET_OFF 0x08
#define ARM_INT_STS_OFF 0x10        //from OPENRISC
#define ARM_INT_MASK_OFF 0x14
#define ARM_INT_CLR_OFF 0x18
#define ARM_INT_RAW_OFF 0x1C
#define WB_ADDR_SET0_OFF 0x20
#define WB_ADDR_SET1_OFF 0x24

///#define USE_INTERRUPT

typedef struct vsp_iommu_map_data
{
    int fd;
    size_t size;
    unsigned long iova;
}VSP_IOMMU_MAP_DATA_T;

typedef struct vsp_ahbm_reg_tag
{
    volatile uint32 ARM_ACCESS_CTRL;				//[2]: Glb_regs_access_sw,
    //			0: access by ARM, ARM control mode;
    //			1: access by IMCU, IMCU control mode; It may be accessed by accelerators, when the hardware is processing
    //[1]: Share_ram_access_sw, 0: access by ARM; 1: access by IMCU;
    //[0]: boot_ram_access_sw, 0: access by ARM; 1: access by IMCU;

    volatile uint32 ARM_ACCESS_STATUS;					//[1] Share_ram_acc_status, 0: access by ARM; 1:access by IMCU
    //[0]: Boot_ram_acc_status, 0: access by ARM; 1:access by IMCU

    volatile uint32 MCU_CONTROL_SET;					//[1]: IMCU_WAKE, wake up open-risc, posedge of this signal will generate IRQ to Open-RISC
    //[0]: IMCU_EB, 1: enable the clock to open-risc

    volatile uint32 RSV0;
    volatile uint32 ARM_INT_STS;			//This interrupt only active in IMCU control mode.
    //[2]: VSP_ACC_INT
    //[1]: FRM_ERR
    //[0]: MCU_DONE

    volatile uint32 ARM_INT_MASK;				//
    volatile uint32 ARM_INT_CLR;				//
    volatile uint32 ARM_INT_RAW;				//
    volatile uint32 WB_ADDR_SET0;				//[28:0]: Iwb_addr_base, Base address of open-risc instruction access space; Dword align.
    volatile uint32 WB_ADDR_SET1;				//[28:0]: Dwb_addr_base, Base address of open-risc DATA access space; Dword align.
} VSP_AHBM_REG_T;

//glb reg
#define VSP_INT_SYS_OFF 0x0             //from VSP
#define VSP_INT_MASK_OFF 0x04
#define VSP_INT_CLR_OFF 0x08
#define VSP_INT_RAW_OFF 0x0c
#define AXIM_ENDIAN_OFF 0x10
#define AXIM_BURST_GAP_OFF 0x14
#define AXIM_PAUSE_OFF 0x18
#define AXIM_BURST_GAP_OFF 0x14
#define AXIM_STS_OFF 0x1c
#define VSP_MODE_OFF 0x20
#define IMG_SIZE_OFF 0x24
#define RAM_ACC_SEL_OFF 0x28
#define VSP_INT_GEN_OFF 0x2c
#define VSP_START_OFF 0x30
#define VSP_SIZE_SET_OFF 0x34
#define VSP_CFG0_OFF 0x3c
#define VSP_CFG1_OFF 0x40
#define VSP_CFG2_OFF 0x44
#define VSP_CFG3_OFF 0x48
#define VSP_CFG4_OFF 0x4c
#define VSP_CFG5_OFF 0x50
#define VSP_CFG6_OFF 0x54
#define VSP_CFG7_OFF 0x58
#define VSP_CFG8_OFF 0x5c
#define BSM0_FRM_ADDR_OFF 0x60
#define BSM1_FRM_ADDR_OFF 0x64
#define VSP_ADDRIDX0_OFF 0x68
#define VSP_ADDRIDX1_OFF 0x6c
#define VSP_ADDRIDX2_OFF 0x70
#define VSP_ADDRIDX3_OFF 0x74
#define VSP_ADDRIDX4_OFF 0x78
#define VSP_ADDRIDX5_OFF 0x7c
#define MCU_START_OFF 0x80//ro
#define VSP_CFG9_OFF 0x84//wo
#define VSP_CFG10_OFF 0x88//wo
#define VSP_CFG11_OFF  0x8c
#define VSP_DBG_STS0_OFF 0x100//mbx mby
#define VSP_DEINTELACE_OFF  0x1c0

typedef struct vsp_global_reg_tag
{
    volatile uint32 VSP_INT_STS;		//Interrupt generated by hardware accelerators.
    //[8]: MBW_MB_DONE, One MB have been wrote out to external memory
    //[7]: MCU_WAKEUP, Wake up interrupt from ARM
    //[6]: VLD_SLICE_DONE, VLD done of current slice
    //[5]: TIME_OUT, Time out of hardware
    //[4]: VLD_ERR, VLD ERROR
    //[3]: PPA_MB_DONE, PPA done of current MB
    //[2]: MBW_SLICE_DONE, Reconstruct data of current slice out done
    //[1]: VLC_SLICE_DONE, VLC current slice done. only active in encode mode
    //[0]: BSM_BUF_OVF, bsm buffer in external memory overflow

    volatile uint32 VSP_INT_MASK;

    volatile uint32 VSP_INT_CLR;

    volatile uint32 VSP_INT_RAW;

    volatile uint32 AXIM_ENDIAN_SET;		//[17]: rch_round_mode, 1: round-robin; 0: fixed priority
    //[16]: wch_round_mode, 1: round-robin; 0: fixed priority
    //[15:12]: rsv
    //[11]: WB_wr_mode, 1: need check AXI RSP when processing write command from open-risc
    //[10]: cache_disable, 1: disable MCA CACHE
    //[9:7]: wb_endian
    //[6]: uv_endian
    //[5:3]: bs_endian
    //[2:0]: mb_endian

    volatile uint32 AXIM_BURST_GAP;		//[31:16]: Wch_burst_gap
    //[15:0]: Rch_burst_gap

    volatile uint32 AXIM_PAUSE;		//[1]: Wch_vdb_pause
    //[0]: Rch_vdb_pause

    volatile uint32 AXIM_STS;		//[2]: Axim_rch_busy
    //[1]: Axim_wch_busy
    //[0]: Axim_busy

    volatile uint32 VSP_MODE;		//[8]: Watchdog_disable, 1: disable wish bone watch dog check
    //[7]: Auto_ck_gate_en, 1: enable clock gating
    //[6]: VLD_table_mode, 1: software update the table, 0: hardware auto update table
    //[5]: Manual_mode, 1: enable manual mode, the header parser and VLD decoder would be processed by MCU
    //[4]: Work_mode, 1: encoder, 0: decoder
    //[3:0}: VSP_standard, b'000: H.263, b'001:MPEG-4, b'010:VP8, b'011: FLV, b'100: H.264, b'101: real8, b'110: real9/10

    volatile uint32 IMG_SIZE;		//[24:16]: ORIG_IMG_WIDTH, uint 8 byte, for encoder only
    //[15:8]: MB_Y_MAX, mb number in Y direction
    //[7:0]: MB_X_MAX, mb number in X direction

    volatile uint32 RAM_ACC_SEL;		//[0]: SETTING_RAM_ACC_SEL, 1: access by hardware accelerator, 0: access by software.

    volatile uint32 VSP_INT_GEN;		//[1]: ERROR_Int_gen, write 0 then write 1, generate pos edge to trigger ARM interrupt
    //[0]: Done_int_gen, write 0 then write 1, generate pos edge to trigger ARM interrupt

    volatile uint32 VSP_START_OPT;	//[3]: CACHE_INI_START, start cache initial
    //[2]: ENCODE_START, active in encode mode
    //[1]: DECODE_START, active in decode mode
    //[0]: VLD_TABLE_START, start to fetch VLD data active

    volatile uint32 VSP_SIZE_SET;		//[11:0]: VLC_TABLE_SIZE, the size of VLC table in external memory, WORD unit

    volatile uint32 TIME_OUT_SET;		//

    volatile uint32 VSP_CFG0;		//[31]: Deblocking_eb, 1: enable de-block mode
    //[30:25]: SliceQP, Qp value, it will active in both decode and encode mode
    //[24:16]: Slice_num
    //[15:3]: Max_mb_num, mb number in currrent slice or frame
    //[2:0]: FRM_TYPE, b'00: I, b'01: P, b'10: B

    volatile uint32 VSP_CFG1;		//decoder mode
    //[31:29]: Num_MBLine_in_GOB,
    //[28:20]: Num_MB_in_GOB
    //[19:0]: Nalu_length, Byte unit

    //encoder mode
    //[31]: Bsm_bytealign_mode, 1: hardware auto byte-align at the end of each slice, only for encode mode
    //[30:25]: MB_Qp, Qp of current MB
    //[24:16]: Ipred_mode_cfg, IEA prediction mode setting
    //[15:12]: Ime_8x8_max, less than 3
    //[11:8]: Ime_16x16_max, Less than 8
    //[7:0]: Skip_threshold,

    volatile uint32 VSP_CFG2;		//decoder mode
    //[31]: ppa_info_vdb_eb, 1: PPA need write MB info to DDR
    //[30]: MCA_rounding_type, MCA ONLY, rounding sel for MPEG-4
    //[29]: DCT_H264_SCALE_EN
    //[28:16]: first_mb_num
    //[15]: MCA_vp8_bilinear_en
    //[14:8]: first_mb_y
    //[6:0]: first_mb_x

    //encoder mode
    //[31]: ppa_info_vld_eb, 1: PPA need write MB info to DDR
    //[30]: MCA_rounding_type, for MCA only
    //[29]: dct_h264_scale_en, for DCT
    //[25:16]: CUR_IMG_ST_X, Horizontal start position of current image; unit: 2 pixel
    //[15]: MCA_vp8_bilinear_en
    //[14:8]: first_mb_y, the first MB in Y direction
    //[6:0]: last_mb_y, the last MB in Y direction

    volatile uint32 VSP_CFG3;		//encoder mode
    //[30]: ROT_MODE, mirror, 1: enable, 0: disable
    //[29]: ROT_MODE, flip, 1: enable, 0: disable
    //[28]: ROT_MODE, 90', 1: enable, 0: disable
    //[25:16]: CUR_IMG_ST_Y, Start position of y, unit: 2 pixel

    //H264 decoder
    //[20:19]: Disable_deblocking filter_idc
    //[18:17]: Weighted_bipred_idc
    //[16]: Weighted_pred_flag
    //[15]: MCA_weighted_en
    //[14]: entropy_coding_mode_flag, 0: cavlc, 1: cabac
    //[13]: transform_8x8_mode_flag
    //[12]: direct_8x8_inference_flag
    //[11]: direct_spatial_mv_pred_flag, indicate the prediction mode in B direct mode, 1: spatial mode
    //[10]: constrained_intra_pred_flag, 1: INTRA MB only can predicted by pixel in Intra MB
    //[9:5]: BETA_OFFSET, for dbk only
    //[4:0], ALPHA_OFFSET, for dbk only

    //MPEG-4 decoder
    //[13]: Quant_type
    //[11:9]: Vop_fcode_backward
    //[8:6]: Vop_fcode_forward
    //[5:3]: Intra_dc_vlc_thr
    //[2]: is_rvlc
    //[1]: data_partitioned
    //[0]: short_header

    //REAL decoder
    //[15]: MCA_weight_en
    //[0]: real_format, 0: stream demuxed from rmvb file, 1: is raw stream.

    //vp8 decoder
    //[9:6]: Nbr_dct_partitions
    //[5:2]: Ref_frame_sign_bias
    //[1]: mb_no_skip_coeff
    //[0]: update_mb_segment_map

    volatile uint32 VSP_CFG4;		//
    //H264 decoder
    //[29:25]: List1_size
    //[24:20]: List0_size
    //[19:15]: Num_refidx_l0_active_minus1
    //[14:10]: Num_refidx_l1_active_minus1
    //[9:5]: Second_Chroma_qp_index_offset
    //[4:0]: Chroma_qp_index_offset

    //MPEG-4 decoder
    //[31:16]: Trd, time_pp
    //[15:0]: Trb, time_bp

    //REAL decoder
    //[31:16]: iRatio 1
    //[15:0]: iRatio 0

    //vp8 decoder
    //[31:16]: bd_value
    //[15:13]: filter_type
    //[11:8]: bd_bitcnt, bool decoder bitcnt inner a byte
    //[7:0]: bd_range, book decoder context from sw

    volatile uint32 VSP_CFG5;		//
    //H.264 decoder
    //[31:0]: Cur_poc

    //MPEG-4 decoder
    //[29:0]: inv_trd, 1/trd

    //vp8 decoder
    //[31:24]: prob_skip_false
    //[23:16]: prob_gf
    //[15:8]: prob_last
    //[7:0]: prob_intra

    volatile uint32 VSP_CFG6;		//
    //H264 decoder
    //[30:26]: List0_idx5_map[4:0]
    //[25:21]: List0_idx4_map[4:0]
    //[20:16]: List0_idx3_map[4:0]
    //[14:10]: List0_idx2_map[4:0]
    //[9:5]: List0_idx1_map[4:0]
    //[4:0]: List0_idx0_map[4:0]

    //vp8 decoder
    //[25]: full_pixel, round uv mv to integer, thatis force lowest 3bits to 0
    //[24:19]: filter_level3
    //[18:13]: filter_level2
    //[12:7]: filter_level1
    //[6:1]: filter_level0
    //[0]: mode_ref_lf_dalta_enable

    volatile uint32 VSP_CFG7;		//
    //H264 decoder
    //[30:26]: List1_idx5_map[4:0]
    //[25:21]: List1_idx4_map[4:0]
    //[20:16]: List1_idx3_map[4:0]
    //[14:10]: List1_idx2_map[4:0]
    //[9:5]: List1_idx1_map[4:0]
    //[4:0]: List1_idx0_map[4:0]

    //vp8 decoder
    //[31:24]: ref_lf_delta3, ref_frame3
    //[23:16]: ref_lf_dalta2, ref_frame2
    //[15:8]: ref_lf_dalta1, ref_frame1
    //[7:0]: ref_lf_dalta0, ref_lf_deltas of ref_frame0

    volatile uint32 VSP_CFG8;		//
    //vp8 decoder
    //[31:24]: mode_lf_dalta3, ref_frame3
    //[23:16]: mode_lf_delta2, ref_frame2
    //[15:8]: mode_lf_dalta1, ref_frame1
    //[7:0]: mode_lf_dalta0, mode_lf_deltas of ref_frame0

    volatile uint32 BSM0_FRM_ADDR;		//[28:0]: bsm_buf0_frm_addr

    volatile uint32 BSM1_FRM_ADDR;		//[28:0]: bsm_buf1_frm_addr

    volatile uint32 ADDR_IDX_CFG00;		//[31:30]: addr_l0_idx15[1:0]
    //[29:24]: addr_l0_idx4
    //[23:18]: addr_l0_idx3
    //[17:12]: addr_l0_idx2
    //[11:6]: addr_l0_idx1
    //[5:0]: addr_l0_idx0[5:0]

    //vp8 decoder
    //[26:18]: QP_UV_DC_0
    //[17:9]: QP_Y2_DC_0
    //[8:0]: QP_Y_DC_0

    volatile uint32 ADDR_IDX_CFG01;		//[31:30]: addr_l0_idx15[3:2]
    //[29:24]: addr_l0_idx9
    //[23:18]: addr_l0_idx8
    //[17:12]: addr_l0_idx7
    //[11:6]: addr_l0_idx6
    //[5:0]: addr_l0_idx5[5:0]

    //vp8 decoder
    //[26:18]: QP_UV_AC_0
    //[17:9]: QP_Y2_AC_0
    //[8:0]: QP_Y_AC_0

    volatile uint32 ADDR_IDX_CFG02;		//[31:30]: addr_l0_idx15[5:4]
    //[29:24]: addr_l0_idx14
    //[23:18]: addr_l0_idx13
    //[17:12]: addr_l0_idx12
    //[11:6]: addr_l0_idx11
    //[5:0]: addr_l0_idx10[5:0]

    //vp8 decoder
    //[26:18]: QP_UV_DC_1
    //[17:9]: QP_Y2_DC_1
    //[8:0]: QP_Y_DC_1

    volatile uint32 ADDR_IDX_CFG03;		//[31:30]: addr_l1_idx15[1:0]
    //[29:24]: addr_l1_idx4
    //[23:18]: addr_l1_idx3
    //[17:12]: addr_l10_idx2
    //[11:6]: addr_l1_idx1
    //[5:0]: addr_l1_idx0[5:0]

    //vp8 decoder
    //[26:18]: QP_UV_AC_1
    //[17:9]: QP_Y2_AC_1
    //[8:0]: QP_Y_AC_1

    volatile uint32 ADDR_IDX_CFG04;		//[31:30]: addr_l1_idx15[3:2]
    //[29:24]: addr_l1_idx9
    //[23:18]: addr_l1_idx8
    //[17:12]: addr_l1_idx7
    //[11:6]: addr_l1_idx6
    //[5:0]: addr_l1_idx5[5:0]

    //vp8 decoder
    //[26:18]: QP_UV_DC_2
    //[17:9]: QP_Y2_DC_2
    //[8:0]: QP_Y_DC_2

    volatile uint32 ADDR_IDX_CFG05;		//[31:30]: addr_l1_idx15[5:4]
    //[29:24]: addr_l1_idx14
    //[23:18]: addr_l1_idx13
    //[17:12]: addr_l1_idx12
    //[11:6]: addr_l1_idx11
    //[5:0]: addr_l1_idx10[5:0]

    //vp8 decoder
    //[26:18]: QP_UV_AC_2
    //[17:9]: QP_Y2_AC_2
    //[8:0]: QP_Y_AC_2

    volatile uint32 IMCU_STS;		//[0]: IMCU_START

    volatile uint32 VSP_CFG9;		//
    //vp8 decoder
    //[26:18]: QP_UV_DC_3
    //[17:9]: QP_Y2_DC_3
    //[8:0]: QP_Y_DC_3

    volatile uint32 VSP_CFG10;		//
    //vp8 decoder
    //[26:18]: QP_UV_AC_3
    //[17:9]: QP_Y2_AC_3
    //[8:0]: QP_Y_AC_3

    volatile uint32 VSP_DBG_STS0;		//[31:16], Mbw_dbg_info
    //[15:8]: CUR_MB_X
    //[7:0]: CUR_MB_Y

    volatile uint32 VSP_DBG_STS1;		//RSV

    volatile uint32 VSP_DBG_STS2;		//[31:0], VLD_DBG_STS

    volatile uint32 VSP_DBG_STS3;		//RSV

    volatile uint32 VSP_DBG_STS4;		//[31:16], PPA_DBG_STS

    volatile uint32 VSP_DBG_STS5;		//RSV

    volatile uint32 VSP_DBG_STS6;		//RSV

    volatile uint32 VSP_DBG_STS7;		//RSV

    volatile uint32 VSP_DBG_STS8;		//[31:16], MBC_DGB_STS

    volatile uint32 VSP_DBG_STS9;		//[31:16], DBK_DBG_STS

    volatile uint32 VSP_DBG_STS10;		//[31:16], MEA0_DBG_STS

    volatile uint32 VSP_DBG_STS11;		//[31:16], MEA1_DBG_STS

    volatile uint32 VSP_DBG_STS12;		//MEA2_DBG_STS
    //[25]: PARTITION_MODE
    //[24]: IS_INTRA
    //[23]: FME_CUR_RBUF_RDY
    //[22]: FME_FBUF_RBUG_RDY
    //[21]: MEA_PARA_RDY
    //[20]: MEA_PBUF_RDY
    //[19]: MEA_CBUF_RDY
    //[18:16]: FME_START
    //[13:0]: INTRA_NUM, intra MB numbers in current frame

    volatile uint32 VSP_DBG_STS13;		//RSV
} VSP_GLB_REG_T;

//bsm reg
#define BSM_CFG0_OFF 0x0
#define BSM_CFG1_OFF 0x4
#define BSM_OP_OFF 0x8
#define BSM_WDATA_OFF 0xc
#define BSM_RDATA_OFF 0x10
#define TOTAL_BITS_OFF 0x14
#define BSM_DBG0_OFF 0x18
#define BSM_DBG1_OFF 0x1c
#define BSM_RDY_OFF 0x20
#define USEV_RD_OFF 0x24
#define USEV_RDATA_OFF 0x28
#define DSTUF_NUM_OFF 0x2c
#define BSM_NAL_LEN 0x34
#define BSM_NAL_DATA_LEN 0x38

//IQW_TABLE
#define INTER4x4Y_OFF 0x00
#define INTER4x4U_OFF 0x10
#define INTER4x4V_OFF 0x20
#define INTRA4x4Y_OFF 0x40
#define INTRA4x4U_OFF 0x50
#define INTRA4x4V_OFF 0x60
#define INTER8x8_OFF 0x80
#define INTRA8x8_OFF 0x100

typedef struct  bsmr_control_register_tag
{
    volatile uint32 BSM_CFG0;		//[31]: BUFF0_STATUS,
    //[30]: BUFF1_STATUS,
    //	0: Buffer is inactive 	1: Buffer is active. sw set, hw clr.
    //	video should only use buff0, but for error conditions that buffer switch to buff1, both buff can be set to active
    //[21:0]: BUFFER_SIZE, Buffer size for bit stream, the unit is byte,reset value 22'h3fffff

    volatile uint32 BSM_CFG1;		//[31]: DESTUFFING_EN, Destuffing function enable, active high,
    // stuffing in encoder mode is automatically enabled for only H.264, this bit is not used.
    //[30]: startcode_check_mode, 1: startcode check mode for H264Dec only. 0: normal mode
    //[21:0]: BSM_OFFSET_ADDR, Start offset address in bit stream buffer , unit is byte

    volatile uint32 BSM_OPERATE;		//[29:24]: OPT_BITS, Number of bits to be flushed, only valid in decoding. The supported range is from 1 to 32 bits
    //[3]: BYTE_ALIGN, byte align bitstream.
    //[2]: COUNTER_CLR, Clear statistical counter
    //[1]: BSM_CLR, Move data remained in FIFO to external memory or discard data in FIFO; Decoder: clear fifo.
    //[0]: BSM_FLUSH, Remove n bit from bit stream buffer, only valid in decoding

    volatile uint32 BSM_WDATA;		//[31:0]: BSM_WDATA, The data to be added to the bit stream


    volatile uint32 BSM_RDATA;		//[31:0]: BSM_RDATA, Current 32-bit bit stream in the capture window

    volatile uint32 TOTAL_BITS;		//[24:0]: TOTAL_BITS, The number of bits added to or remove from the bit stream

    volatile uint32 BSM_DEBUG;		//[31]: BSM_STATUS, BSM is active/inactive
    //[30:28]: BSM_STATE, BSM control status
    //[27]: DATA_TRAN, 0: bsm can be cleard,1, can not be cleared
    //[16:12]: BSM_SHIFT_REG, The bit amount has been shifted in BS shifter
    //[9:8]: DESTUFFING_LEFT_DCNT, The remained data amount in the de-stuffing module, uinit is word
    //[4]: PING-PONG_BUF_SEL, Current ping-pong buffer ID, buffer0 or buffer1
    //[3:0]: FIFO_DEPTH, BSM FIFO depth

    volatile uint32 BSM_DEBUG1;		//[2]: startcode_found, startcode found, level
    //[1]: bsm_vdb_req, The data transfer request to AHBM
    //[0]: bsm_vdb_rfifo_fsm_empty, AHBM RFIFO empty flag

    volatile uint32 BSM_RDY;		//[1]: BSM_PS_BUSY, Specific char sequence searching is in process. Active high
    //[0]: 0: SW can't access BSM internal FIFO
    //	1: SW is allowed to access BSM internal FIFO
    //Note: when SW will read/write the
    //	  BSM FIFO (access the 0x08 register) this bit should be checked.

    volatile uint32 BSM_GLO_OPT;	//[2]: USEV_err, error of UE or SE
    //[1]: SE_V()
    //[0]: UE_V()

    volatile uint32 BSM_USEV_RDATA;	//[31:0]: UE or SE value, signed int

    volatile uint32 DSTUF_NUM;	//[7:0]: (De-)stuffing byte number

    volatile uint32 RSV;

    volatile uint32 BSM_TOTAL_LEN;	//[21:0]: startcode check mode, total len to next startcode

    volatile uint32 BSM_NET_LEN;	//[21:0]: startcode check mode, net len inside total len. Net_len = total_len - dstuf_num-startcode_len

} VSP_BSM_REG_T;

//RGB2YUV MODE
#define BT601_full     0x0
#define BT601_reduce   0x4

/* -----------------------------------------------------------------------
** Structs
** ----------------------------------------------------------------------- */

typedef struct tagVSPObject
{
    uint_32or64 s_vsp_Vaddr_base ;
    int32 s_vsp_fd ;
    uint32 vsp_freq_div;
    int32	error_flag;
    int32   vsp_version;
    int32 codec_id;
    int32 trace_enabled;
    int32 vsync_enabled;
} VSPObject, VPPObject;

//error id, added by xiaowei, 20130910
#define ER_SREAM_ID   		(1<<0)
#define ER_MEMORY_ID  		(1<<1)
#define ER_REF_FRM_ID   	(1<<2)
#define ER_HW_ID                        (1<<3)
#define ER_FORMAT_ID                        (1<<4)
#define ER_SPSPPS_ID                        (1<<5)

/* -----------------------------------------------------------------------
** Global
** ----------------------------------------------------------------------- */
int32 vpp_deint_init(VPPObject *vo);
int32 vpp_deint_release(VPPObject *vo);
int32 vpp_deint_process(VPPObject *vo, unsigned long src_frame,
                        unsigned long ref_frame, unsigned long dst_frame,
                        uint32 frame_no, DEINT_PARAMS_T *params);
int vpp_deint_get_iova(VPPObject *vo, int fd, unsigned long *iova,
                       size_t *size);
int vpp_deint_free_iova(VPPObject *vo, unsigned long iova, size_t size);
int vpp_deint_get_IOMMU_status(VPPObject *vo);

int32 VSP_CFG_FREQ(VSPObject *vo, uint32 frame_size);
int32 VSP_OPEN_Dev(VSPObject *vo);
int32 VSP_CLOSE_Dev(VSPObject *vo);
int32 VSP_POLL_COMPLETE(VSPObject *vo);
int32 VSP_ACQUIRE_Dev(VSPObject *vo);
int32 VSP_RELEASE_Dev(VSPObject *vo);
int32 ARM_VSP_RST(VSPObject *vo);
int32 VSP_SET_SCENE_MODE(VSPObject *vo, uint32*  scene_mode);
int32 VSP_GET_SCENE_MODE(VSPObject *vo, uint32*  scene_mode);
int32 VSP_Get_Codec_Counter(VSPObject *vo, uint32 * counter);
int VSP_Get_IOVA(VSPObject *vo, int fd, unsigned long *iova, size_t *size);
int VSP_Free_IOVA(VSPObject *vo,  unsigned long iova, size_t size);
int VSP_Get_IOMMU_Status(VSPObject *vo);
int VSP_NeedAlign(VSPObject *vo);

int32 vsp_read_reg_poll(VSPObject *vo, uint32 reg_addr, uint32 msk_data,uint32 exp_value, uint32 time, char *pstring);

#define VSP_WRITE_REG(reg_addr, value, pstring) (*(volatile uint32 *)((reg_addr) +((VSPObject *)vo)->s_vsp_Vaddr_base)  = (value))
#define VSP_READ_REG(reg_addr, pstring)	((*(volatile uint32 *)((reg_addr)+((VSPObject *)vo)->s_vsp_Vaddr_base)))
#define VSP_READ_REG_POLL(reg_addr, msk_data, exp_value, time, pstring) \
    vsp_read_reg_poll(((VSPObject *)vo), reg_addr, msk_data, exp_value, time, pstring)

/**---------------------------------------------------------------------------
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
#endif  //_VSP_DRV_SC8830_H_

