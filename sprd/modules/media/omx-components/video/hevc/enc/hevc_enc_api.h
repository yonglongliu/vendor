/******************************************************************************
 ** File Name:    avc_enc_api.h                                                   *
 ** Author:                                     		                      *
 ** DATE:         6/17/2013                                                   *
 ** Copyright:    2013 Spreadtrum, Incorporated. All Rights Reserved.           *
 ** Description:  declaration  of interface between avc encoder library and it's user                      *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 6/17/2013  Xiaowei.Luo   			      Create.                                     *
 *****************************************************************************/
#ifndef _HEVC_ENC_API_H_
#define _HEVC_ENC_API_H_

/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/

/**---------------------------------------------------------------------------*
 **                             Compiler Flag                                 *
 **---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C" {
#endif

typedef unsigned char        BOOLEAN;
typedef unsigned char        uint8;
typedef unsigned short        uint16;
typedef unsigned int            uint32;
typedef signed char        int8;
typedef signed short        int16;
typedef signed int        int32;

/**
This enumeration is for profiles. The value follows the profile_idc  in sequence
parameter set rbsp. See Annex A.
@publishedAll
*/
typedef enum {
    AVC_BASELINE = 66,
    AVC_MAIN = 77,
    AVC_EXTENDED = 88,
    AVC_HIGH = 100,
    AVC_HIGH10 = 110,
    AVC_HIGH422 = 122,
    AVC_HIGH444 = 144
} HEVCProfile;

/**
This enumeration is for levels. The value follows the level_idc in sequence
parameter set rbsp. See Annex A.
@published All
*/
typedef enum {
    AVC_LEVEL_AUTO = 0,
    AVC_LEVEL1_B = 9,
    AVC_LEVEL1 = 10,
    AVC_LEVEL1_1 = 11,
    AVC_LEVEL1_2 = 12,
    AVC_LEVEL1_3 = 13,
    AVC_LEVEL2 = 20,
    AVC_LEVEL2_1 = 21,
    AVC_LEVEL2_2 = 22,
    AVC_LEVEL3 = 30,
    AVC_LEVEL3_1 = 31,
    AVC_LEVEL3_2 = 32,
    AVC_LEVEL4 = 40,
    AVC_LEVEL4_1 = 41,
    AVC_LEVEL4_2 = 42,
    AVC_LEVEL5 = 50,
    AVC_LEVEL5_1 = 51
} HEVCLevel;

/* constants used in the structures below */
#define MAXIMUMVALUEOFcpb_cnt   32  /* used in HRDParams */
#define MAX_NUM_REF_FRAMES_IN_PIC_ORDER_CNT_CYCLE 255   /* used in SeqParamSet */
#define MAX_NUM_SLICE_GROUP  8      /* used in PicParamSet */
#define MAX_REF_PIC_LIST_REORDERING 32  /* 32 is maximum according to Annex A, SliceHeader */
#define MAX_DEC_REF_PIC_MARKING 64   /* 64 is the maximum possible given the max num ref pictures to 31. */
#define MAX_FS (16+1)  /* pre-defined size of frame store array */
#define MAX_LEVEL_IDX  15  /* only 15 levels defined for now */
#define MAX_REF_PIC_LIST 33 /* max size of the RefPicList0 and RefPicList1 */

/**
This enumerate the status of certain flags.
@publishedAll
*/
typedef enum {
    HEVC_OFF = 0,
    HEVC_ON = 1
} HEVCFlag;

typedef struct tagHEVCEncParam {
    /* if profile/level is set to zero, encoder will choose the closest one for you */
    HEVCProfile profile; /* profile of the bitstream to be compliant with*/
    HEVCLevel   level;   /* level of the bitstream to be compliant with*/

    int width;      /* width of an input frame in pixel */
    int height;     /* height of an input frame in pixel */

    int poc_type; /* picture order count mode, 0,1 or 2 */
    /* for poc_type == 0 */
    uint32 log2_max_poc_lsb_minus_4; /* specify maximum value of POC Lsb, range 0..12*/
    /* for poc_type == 1 */
    uint32 delta_poc_zero_flag; /* delta POC always zero */
    int offset_poc_non_ref; /* offset for non-reference pic */
    int offset_top_bottom; /* offset between top and bottom field */
    uint32 num_ref_in_cycle; /* number of reference frame in one cycle */
    int *offset_poc_ref; /* array of offset for ref pic, dimension [num_ref_in_cycle] */

    int num_ref_frame;  /* number of reference frame used */
    int num_slice_group;  /* number of slice group */
    int fmo_type;   /* 0: interleave, 1: dispersed, 2: foreground with left-over
                    3: box-out, 4:raster scan, 5:wipe, 6:explicit */
    /* for fmo_type == 0 */
    uint32 run_length_minus1[MAX_NUM_SLICE_GROUP];   /* array of size num_slice_group, in round robin fasion */
    /* fmo_type == 2*/
    uint32 top_left[MAX_NUM_SLICE_GROUP-1];           /* array of co-ordinates of each slice_group */
    uint32 bottom_right[MAX_NUM_SLICE_GROUP-1];       /* except the last one which is the background. */
    /* fmo_type == 3,4,5 */
    HEVCFlag change_dir_flag;  /* slice group change direction flag */
    uint32 change_rate_minus1;
    /* fmo_type == 6 */
    uint32 *slice_group; /* array of size MBWidth*MBHeight */

    HEVCFlag db_filter;  /* enable deblocking loop filter */
    int32 disable_db_idc;  /* 0: filter everywhere, 1: no filter, 2: no filter across slice boundary */
    int32 alpha_offset;   /* alpha offset range -6,...,6 */
    int32 beta_offset;    /* beta offset range -6,...,6 */

    HEVCFlag constrained_intra_pred; /* constrained intra prediction flag */

    HEVCFlag auto_scd;   /* scene change detection on or off */
    int idr_period; /* idr frame refresh rate in number of target encoded frame (no concept of actual time).*/
    int intramb_refresh;    /* minimum number of intra MB per frame */
    HEVCFlag data_par;   /* enable data partitioning */

    HEVCFlag fullsearch; /* enable full-pel full-search mode */
    int search_range;   /* search range for motion vector in (-search_range,+search_range) pixels */
    HEVCFlag sub_pel;    /* enable sub pel prediction */
    HEVCFlag submb_pred; /* enable sub MB partition mode */
    HEVCFlag rdopt_mode; /* RD optimal mode selection */
    HEVCFlag bidir_pred; /* enable bi-directional for B-slice, this flag forces the encoder to encode
                        any frame with POC less than the previously encoded frame as a B-frame.
                        If it's off, then such frames will remain P-frame. */

    HEVCFlag rate_control; /* rate control enable, on: RC on, off: constant QP */
    int initQP;     /* initial QP */
    uint32 bitrate;    /* target encoding bit rate in bits/second */
    uint32 CPB_size;  /* coded picture buffer in number of bits */
    uint32 init_CBP_removal_delay; /* initial CBP removal delay in msec */

    uint32 frame_rate;  /* frame rate in the unit of frames per 1000 second */
    /* note, frame rate is only needed by the rate control, AVC is timestamp agnostic. */

    HEVCFlag out_of_band_param_set; /* flag to set whether param sets are to be retrieved up front or not */

    HEVCFlag use_overrun_buffer;  /* do not throw away the frame if output buffer is not big enough.
                                    copy excess bits to the overrun buffer */
} HEVCEncParams;

// Encoder video capability structure
typedef struct {
    HEVCProfile profile;
    HEVCLevel   level;
    int32 max_width;
    int32 max_height;
} MMEncCapability;

typedef enum {
    MMENC_OK = 0,
    MMENC_ERROR = -1,
    MMENC_PARAM_ERROR = -2,
    MMENC_MEMORY_ERROR = -3,
    MMENC_INVALID_STATUS = -4,
    MMENC_OUTPUT_BUFFER_OVERFLOW = -5,
    MMENC_HW_ERROR = -6
} MMEncRet;

// Decoder buffer for decoding structure
typedef struct {
    uint8 *common_buffer_ptr;     // Pointer to buffer used when decoding
    unsigned long common_buffer_ptr_phy;
    uint32 size;    // Number of bytes decoding buffer

    int32  frameBfr_num;   //YUV frame buffer number

    uint8 *int_buffer_ptr;  // internal memory address
    int32  int_size;    //internal memory size
} MMCodecBuffer;

typedef MMCodecBuffer MMEncBuffer;

typedef enum {
    MMENC_YUV420P_YU12 = 0,
    MMENC_YUV420P_YV12 = 1,
    MMENC_YUV420SP_NV12 = 2,   /*u/v interleaved*/
    MMENC_YUV420SP_NV21 = 3,   /*v/u interleaved*/
    MMENC_ARGB32 = 4,
    MMENC_RGBA32 = 5,
} MMENC_YUV_FORMAT_E;

// Encoder video format structure
typedef struct
{
    int32  frame_width;    //target frame width
    int32  frame_height;   //target frame Height

    int32  org_width;      //input original frame width (=1.2*frame_width)
    int32  org_height;     //input original frame Height (=1.2*frame_height)

    int32  is_h263;        // 1 : H.263, 0 : MP4
    int32  time_scale;

    int32  yuv_format;
    int32  cabac_en;

    BOOLEAN  EISMode;
} MMEncVideoInfo;


// Encoder config structure
typedef struct {
    uint32  RateCtrlEnable;            // 0 : disable  1: enable
    uint32  targetBitRate;             // 400 ~  (bit/s)
    uint32  FrameRate;
    uint32  PFrames;

    uint32  vbv_buf_size;               //vbv buffer size, to determine the max transfer delay

    uint32  QP_IVOP;                    // first I frame's QP; 1 ~ 31, default QP value if the Rate Control is disabled
    uint32  QP_PVOP;                    // first P frame's QP; 1 ~ 31, default QP value if the Rate Control is disabled

    uint32  h263En;                     // 1 : H.263, 0 : MP4

    uint32  profileAndLevel;

    uint32 PrependVPSSPSPPSEnalbe;  // 0: disable, 1: disable

    uint32 Color_Format;            //0:420SP, 1: 420P, 2: ARGB32
    uint32 CTUSize_Slice;           //CTU size of height per slice
    uint32 EncSceneMode;
    BOOLEAN openPnmode;
} MMEncConfig;

// Encoder input structure
typedef struct {
    uint8   *p_src_y;
    uint8   *p_src_u;
    uint8   *p_src_v;

    uint8   *p_src_y_phy;
    uint8   *p_src_u_phy;
    uint8   *p_src_v_phy;

    bool  needIVOP;
    bool  ischangebitrate;
    int32  time_stamp;  //time stamp
    int32    bs_remain_len;  //remained bitstream length
    int32    channel_quality;  //0: good, 1: ok, 2: poor
    int32    org_img_width;
    int32    org_img_height;
    int32    crop_x;
    int32    crop_y;
    int32    bitrate;
    double  matrix[3][3];
    int32    yuv_format;
} MMEncIn;

// Encoder output structure
typedef struct {
    uint8  *pOutBuf;  //Output buffer
    int32  strmSize;  //encoded stream size, if 0, should skip this frame.
    int32  vopType;  //0: I VOP, 1: P VOP, 2: B VOP
} MMEncOut;

typedef struct tagvideoEncControls {
    void *videoEncoderData;
    int  videoEncoderInit;
} VideoEncControls;

typedef void (*FunctionType_FlushBSCache)(void *aUserData);

/**
This structure has to be allocated and maintained by the user of the library.
This structure is used as a handle to the library object.
*/
typedef struct tagHEVCHandle {
    /** A pointer to the internal data structure. Users have to make sure that this value
        is NULL at the beginning.
    */
    void        *videoEncoderData;

    /** A pointer to user object which has the following member functions used for
    callback purpose.  !!! */
    void        *userData;

    /** Flag to enable debugging */
    uint32  debugEnable;

    FunctionType_FlushBSCache VSP_FlushBSCache;
} HEVCHandle;

/**----------------------------------------------------------------------------*
**                           Function Prototype                               **
**----------------------------------------------------------------------------*/
/*****************************************************************************/
//  Description:   Get capability of h264 encoder
//  Global resource dependence:
//  Author:
//  Note:
/*****************************************************************************/
MMEncRet H265EncGetCodecCapability(HEVCHandle *hevcHandle, MMEncCapability *Capability);

/*****************************************************************************/
//  Description:   Pre-Init h264 encoder
//  Global resource dependence:
//  Author:
//  Note:
/*****************************************************************************/
MMEncRet H265EncPreInit(HEVCHandle *hevcHandle, MMCodecBuffer *pInterMemBfr);

/*****************************************************************************/
//  Description:   Init h264 encoder
//  Global resource dependence:
//  Author:
//  Note:
/*****************************************************************************/
MMEncRet H265EncInit (HEVCHandle *hevcHandle, MMCodecBuffer *pExtaMemBfr,MMCodecBuffer *pBitstreamBfr, MMCodecBuffer *pBitstreamBfr_pn, MMEncVideoInfo *pVideoFormat);

/*****************************************************************************/
//  Description:   Set h264 encode config
//  Global resource dependence:
//  Author:
//  Note:
/*****************************************************************************/
MMEncRet H265EncSetConf(HEVCHandle *hevcHandle, MMEncConfig *pConf);

/*****************************************************************************/
//  Description:   Get h264 encode config
//  Global resource dependence:
//  Author:
//  Note:
/*****************************************************************************/
MMEncRet H265EncGetConf(HEVCHandle *hevcHandle, MMEncConfig *pConf);

/*****************************************************************************/
//  Description:   Encode one frame
//  Global resource dependence:
//  Author:
//  Note:
/*****************************************************************************/
MMEncRet H265EncStrmEncode (HEVCHandle *hevcHandle, MMEncIn *pInput, MMEncOut *pOutput);

/*****************************************************************************/
//  Description:   generate sps or pps header
//  Global resource dependence:
//  Author:
//  Note:
/*****************************************************************************/
MMEncRet H265EncGenHeader(HEVCHandle *hevcHandle, MMEncOut *pOutput, int is_sps);

/*****************************************************************************/
//  Description:   Close h264 encoder
//  Global resource dependence:
//  Author:
//  Note:
/*****************************************************************************/
MMEncRet H265EncRelease(HEVCHandle *hevcHandle);

int H265Enc_get_iova(HEVCHandle *hevcHandle, int fd, unsigned long *iova, size_t *size);
int H265Enc_free_iova(HEVCHandle *hevcHandle, unsigned long iova, size_t size);
int H265Enc_get_IOMMU_status(HEVCHandle *hevcHandle);

typedef MMEncRet (*FT_H265EncGetCodecCapability)(HEVCHandle *hevcHandle, MMEncCapability *Capability);
typedef MMEncRet (*FT_H265EncPreInit)(HEVCHandle *hevcHandle, MMCodecBuffer *pInterMemBfr);
typedef MMEncRet (*FT_H265EncInit)(HEVCHandle *hevcHandle, MMCodecBuffer *pExtaMemBfr,MMCodecBuffer *pBitstreamBfr, MMCodecBuffer *pBitstreamBfr_pn, MMEncVideoInfo *pVideoFormat);
typedef MMEncRet (*FT_H265EncSetConf)(HEVCHandle *hevcHandle, MMEncConfig *pConf);
typedef MMEncRet (*FT_H265EncGetConf)(HEVCHandle *hevcHandle, MMEncConfig *pConf);
typedef MMEncRet (*FT_H265EncStrmEncode) (HEVCHandle *hevcHandle, MMEncIn *pInput, MMEncOut *pOutput);
typedef MMEncRet (*FT_H265EncGenHeader)(HEVCHandle *hevcHandle, MMEncOut *pOutput, int is_sps);
typedef MMEncRet (*FT_H265EncRelease)(HEVCHandle *hevcHandle);

typedef int (*FT_H265Enc_get_iova)(HEVCHandle *hevcHandle, int fd, unsigned long *iova, size_t *size);
typedef int (*FT_H265Enc_free_iova)(HEVCHandle *hevcHandle, unsigned long iova, size_t size);
typedef int (*FT_H265Enc_get_IOMMU_status)(HEVCHandle *hevcHandle);
typedef int (*FT_H265Enc_Need_Align)(HEVCHandle *hevcHandle);

/**----------------------------------------------------------------------------*
**                         Compiler Flag                                      **
**----------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
#endif
// End
