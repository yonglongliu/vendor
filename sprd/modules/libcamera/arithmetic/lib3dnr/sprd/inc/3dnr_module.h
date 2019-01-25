#ifndef _3DNR_MODULE_H_
#define _3DNR_MODULE_H_
#include "interface.h"

#define T3DNR_PREVIEW_MODE  0	//only preview view, for each frame, 3ndr filtered image will be output
#define T3DNR_CAPTURE_MODE  1	//only output final 3DNR filtered image in fullsize
#define T3DNR_FULLSIZE_MODE 2	//process 5 fullsize image stored in DDR

#define CAPTURE_MODE_PREVIEW_STAGE 3
#define CAPTURE_MODE_CAPTURE_STAGE 4

#define FULLSIZE_MODE_PREVIEW_STAGE 4
#define FULLSIZE_MODE_FUSE_STAGE    5

#define T3DNR_CAPTURE_MODE_PREVIEW_STAGE (T3DNR_CAPTURE_MODE + CAPTURE_MODE_PREVIEW_STAGE)
#define T3DNR_CAPTURE_MODE_CAPTURE_STAGE (T3DNR_CAPTURE_MODE + CAPTURE_MODE_CAPTURE_STAGE)

#define T3DNR_FULLSIZE_MODE_PREVIEW_STAGE (T3DNR_FULLSIZE_MODE + FULLSIZE_MODE_PREVIEW_STAGE)
#define T3DNR_FULLSIZE_MODE_FUSE_STAGE (T3DNR_FULLSIZE_MODE + FULLSIZE_MODE_FUSE_STAGE)

typedef enum c3dnr_status {
	C3DNR_STATUS_IDLE = 0,
	C3DNR_STATUS_INITED,
	C3DNR_STATUS_DEINITED,
	C3DNR_STATUS_BUSY,
} c3dnr_status_t;

typedef struct c3dnr_info {
	cmr_u8 operation_mode;		//0: preview mode, 1: capture mode, 2: fullsize capture mode
	cmr_u8 fusion_mode;		//1bit unsigned {0, 1} imge fusion mode:0: adaptive weight 1: fixed weight
	cmr_u8 MV_mode;			//1bit unsigned {0, 1} MV_mode:0 MV calculated by integral projection 1: MV set by users
	cmr_s32 *MV_x;			//6bit signed [-32, 31] output MV_x calculated by integral projection
	cmr_s32 *MV_y;			//6bit signed [-32, 31] output MV_y calcualted by integral projection
	//cmr_s32 MV_x_output;  	//6bit signed [-32, 31] output MV_x calculated by integral projection
	//cmr_s32 MV_y_output;
	cmr_u8 filter_switch;		//1bit unsigned{0, 1} whether use 3x3 filter to smooth current pixel to calculate pixel difference for Y component 0: not filtering 1: filtering
	cmr_u8 *y_src_weight;		//8bit unsigned [0, 255] weight for fusing current frame and reference frame set by user for Y component
	cmr_u8 *u_src_weight;		//8bit unsigned [0, 255] weight for fusing current frame and reference frame set by user for U component
	cmr_u8 *v_src_weight;		//8bit unsigned [0, 255] weight for fusing current frame and reference frame set by user for V component
	cmr_u32 frame_num;		//count;
	cmr_u32 curr_frameno;
	cmr_s32 orig_width;
	cmr_s32 orig_height;
	cmr_s32 small_width;
	cmr_s32 small_height;
	cmr_u32 imageNum;
	cmr_s32 **xProj1D;
	cmr_s32 **yProj1D;
	c3dnr_buffer_t pdst_img;
	c3dnr_buffer_t intermedbuf;
	c3dnr_buffer_t *pout_blendimg;
	c3dnr_buffer_t *pfirst_blendimg;
	c3dnr_buffer_t *psecond_blendimg;
	c3dnr_buffer_t *porigimg;
	cmr_u8 *ref_imageY;
	cmr_u8 *ref_imageU;
	cmr_u8 *ref_imageV;
	cmr_s32 *extra;
	pthread_t monitor_thread;
	cmr_handle isp_handle;		//the isp handle for isp_ioctl
	isp_ioctl_fun isp_ioctrl;
	c3dnr_buffer_t out_img[2];	//for preview
	cmr_u8 is_first_frame;
	cmr_u8 blend_num;
	c3dnr_status_t status;
	cmr_u32 is_cancel;
} c3dnr_info_t;

typedef enum c3dnr_frame_type {
	FIRST_FRAME = 0,
	NORMAL_FRAME,
	LAST_FRAME
} c3dnr_frame_type_t;

cmr_s32 initModule(cmr_s32 small_width, cmr_s32 small_height, cmr_s32 orig_width, cmr_s32 orig_height,
	       cmr_u32 total_imageNum);
void Config3DNR(c3dnr_param_info_t *param);
void threeDNR_1D_process(c3dnr_buffer_t *curr_image ,cmr_s32 width ,cmr_s32 height , cmr_u32 fram_no, c3dnr_mode_e mode);
c3dnr_info_t *get_3dnr_moduleinfo();
#endif
