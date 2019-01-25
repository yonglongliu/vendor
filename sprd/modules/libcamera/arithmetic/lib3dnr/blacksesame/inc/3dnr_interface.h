/*add RET_3DNR_CANCELED state*/
#ifndef _3DNR_INTERFACE_H
#define _3DNR_INTERFACE_H


#include <stdint.h>

#ifndef F_FIRST_FRAME
#define F_FIRST_FRAME 1
#define F_LAST_FRAME  2
#endif // !F_FIRST_FRAME

#ifdef __cplusplus
extern "C" {
#endif

//#include "cmr_types.h"
//#include "isp_mw.h"
typedef  struct private_handle
{
    uint8_t *bufferY;
}private_handle_t;

//typedef cmr_int (* isp_ioctl_fun)(cmr_handle isp_handler, enum isp_ctrl_cmd cmd, void *param_ptr);

typedef struct c3dnr_cpu_buffer
{
    uint8_t *bufferY;
        uint8_t *bufferU;
        uint8_t *bufferV;
        int32_t fd; //wxz: for phy addr.
}c3dnr_cpu_buffer_t;
typedef struct c3dnr_gpu_buffer
{
    //private_handle_t *handle;
    void *handle;
}c3dnr_gpu_buffer_t;
typedef union c3dnr_buffer
{
    c3dnr_cpu_buffer_t cpu_buffer;
    c3dnr_gpu_buffer_t gpu_buffer;
}c3dnr_buffer_t;


typedef struct c3dnr_param_info
{
    uint16_t orig_width;
    uint16_t orig_height;
    uint16_t small_width;
    uint16_t small_height;
    uint16_t total_frame_num;
    uint16_t gain; // ben
    uint16_t SearchWindow_x;
    uint16_t SearchWindow_y;
    uint16_t low_thr;
    uint16_t ratio;
    int recur_str; // recursion stride for preview
    int *sigma_tmp;
    int *slope_tmp;
    int (*threthold)[6];
    int (*slope)[6];
    c3dnr_buffer_t *poutimg;
#ifdef USE_ISP_HW
    cmr_handle isp_handle; //wxz: call the isp_ioctl need the handle.
    isp_ioctl_fun isp_ioctrl;
#endif
    int yuv_mode; //0: nv12 1: nv21

    int control_en; // 
    int thread_num;
    int thread_num_acc;
    int preview_cpyBuf;
}c3dnr_param_info_t;

typedef struct c3dnr_pre_inparam
{
    uint16_t gain;
}c3dnr_pre_inparam_t;

int threednr_init(c3dnr_param_info_t *param);
int threednr_function(c3dnr_buffer_t *small_image, c3dnr_buffer_t *orig_image);
int threednr_function_pre(c3dnr_buffer_t *small_image, c3dnr_buffer_t *orig_image , c3dnr_buffer_t *video_image , c3dnr_pre_inparam_t* inputparam);
int threednr_deinit();
int threednr_callback();

#ifdef __cplusplus
}
#endif
#define CTRL_SYNC_EN (1<<0) // enable GPU synchronize every frame, if this bit is 0, GPU will run till next GPU command in next frame
#define CTRL_TIME_EN (1<<1) // enable cpu and gpu timing check
#define CTRL_DEBUG_EN (1<<2) // enable debug information

#define CTRL_DEBUG_BUF1_EN (1<<3) //
#define CTRL_DEBUG_WR1_EN (1<<4) //
#define CTRL_DEBUG_BUF2_EN (1<<5) //
#define CTRL_DEBUG_WR2_EN (1<<6) //
#define CTRL_DEBUG_BUF3_EN (1<<7) //
#define CTRL_DEBUG_WR3_EN (1<<8) //


#define CTRL_BIG_IMG_MV_EN (1<<16) //
#define CTRL_FUNC1_EN (1<<24) //
#define CTRL_FUNC2_EN (2<<24) //
#define CTRL_FUNC3_EN (3<<24) //

#define RET_3DNR_OK                 0
#define RET_3DNR_FAIL               -1
#define RET_3DNR_WRONG_PARAM        -2
#define RET_3DNR_NO_ENOUGH_MEMORY   -3
#define RET_3DNR_CANCELED          -4

#endif
