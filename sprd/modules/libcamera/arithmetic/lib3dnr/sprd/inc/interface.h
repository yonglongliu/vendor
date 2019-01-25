#ifndef _3DNR_INTERFACE_H
#define _3DNR_INTERFACE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "cmr_types.h"

typedef struct c3dnr_buffer {
	cmr_u8 *bufferY;
	cmr_u8 *bufferU;
	cmr_u8 *bufferV;
	cmr_s32 fd;		//for phy addr.
} c3dnr_buffer_t;

typedef struct c3dn_io_info {
	c3dnr_buffer_t image[3];
	cmr_u32 width;
	cmr_u32 height;
	cmr_s8 mv_x;
	cmr_s8 mv_y;
	cmr_u8 blending_no;
} c3dnr_io_info_t;

typedef cmr_int (*isp_ioctl_fun) (cmr_handle isp_handler, c3dnr_io_info_t *param_ptr);

typedef struct c3dnr_param_info {
	cmr_u16 orig_width;
	cmr_u16 orig_height;
	cmr_u16 small_width;
	cmr_u16 small_height;
	cmr_u16 total_frame_num;
	c3dnr_buffer_t *poutimg;
	cmr_handle isp_handle;		//call the isp_ioctl need the handle.
	isp_ioctl_fun isp_ioctrl;
	c3dnr_buffer_t out_img[2];	//for preview
} c3dnr_param_info_t;

cmr_s32 threednr_init(c3dnr_param_info_t *param);
cmr_s32 threednr_function(c3dnr_buffer_t *small_image, c3dnr_buffer_t *orig_image);
cmr_s32 threednr_function_pre(c3dnr_buffer_t *small_image, c3dnr_buffer_t *orig_image);
cmr_s32 threednr_deinit();
cmr_s32 threednr_callback();
cmr_s32 threednr_cancel();

#ifdef __cplusplus
}
#endif
#endif
