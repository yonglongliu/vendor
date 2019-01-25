#include "port.h"

#include <cutils/atomic.h>

#if 0 // USE_LOG

int log_level;

void ALOG(int level, char file, va_list __arg)
{
	if(level <= log_level)
	{
	   printf(file);
	   vprintf(__arg);
	 }
}
#endif

#if !USE_ARITH

#include "arithmetic/inc/FaceFinder.h"

int FaceFinder_Init(int width, int height, MallocFun Mfp, FreeFun Ffp) {
    return 0;
}

int FaceFinder_Function(unsigned char *src, FaceFinder_Data **ppDstFaces,
                        int *pDstFaceNum, int skip) {
    return 0;
}

int FaceFinder_Finalize(FreeFun Ffp) { return 0; }

#include "arithmetic/inc/FaceSolid.h"

int FaceSolid_Init(int width, int height, MallocFun Mfp, FreeFun Ffp) {
    return 0;
}

int FaceSolid_Function(unsigned char *src, ACCESS_FaceRect **ppDstFaces,
                       int *pDstFaceNum, int skip) {
    return 0;
}

int FaceSolid_Finalize(FreeFun Ffp) { return 0; }

#include "arithmetic/inc/HDR2.h"

int HDR_Function(BYTE *Y0, BYTE *Y1, BYTE *Y2, BYTE *output, int height,
                 int width, char *format) {
    return 0;
}

#endif

#if (BUILD_ISPCODE_MODE == 3) || (BUILD_ISPCODE_MODE == 4) ||                  \
    (BUILD_ISPCODE_MODE == 5)

#include "ae_misc.h"
#include "isp_awb.h"

void *ae_misc_init(struct ae_misc_init_in *in_param,
                   struct ae_misc_init_out *out_param) {}
int32_t ae_misc_deinit(void *handle, void *in_param, void *out_param) {
    return 0;
}
int32_t ae_misc_calculation(void *handle, struct ae_misc_calc_in *in_param,
                            struct ae_misc_calc_out *out_param) {
    return 0;
}
int32_t ae_misc_io_ctrl(void *handle, enum ae_misc_io_cmd cmd, void *in_param,
                        void *out_param) {
    return 0;
}

awb_handle_t awb_init(struct awb_init_param *in_param, void *out_param) {
    return 1;
}

uint32_t awb_deinit(awb_handle_t handle, void *in_param, void *out_param) {
    return 0;
}

uint32_t awb_calculation(awb_handle_t handle, struct awb_calc_param *param,
                         struct awb_calc_result *result) {
    return 0;
}

int32_t awb_param_unpack(void *pack_data, uint32_t data_size,
                         struct awb_param_tuning *tuning_param) {
    return 0;
}

#include "af_alg.h"

af_alg_handle_t af_alg_init(struct af_alg_init_param *init_param,
                            struct af_alg_init_result *result) {}

int32_t af_alg_deinit(af_alg_handle_t handle) {}
int32_t af_alg_calculation(af_alg_handle_t handle,
                           struct af_alg_calc_param *alg_calc_in,
                           struct af_alg_result *alg_calc_result) {}
uint32_t af_alg_ioctrl(af_alg_handle_t handle, enum af_alg_cmd cmd,
                       void *param0, void *param1) {}

#include "lsc_adv.h"
lsc_adv_handle_t lsc_adv_init(struct lsc_adv_init_param *param, void *result) {
    return NULL;
}
int32_t lsc_adv_pre_calc(lsc_adv_handle_t handle,
                         struct lsc_adv_pre_calc_param *param,
                         struct lsc_adv_pre_calc_result *result);
int32_t lsc_adv_calculation(lsc_adv_handle_t handle,
                            enum lsc_gain_calc_mode mode,
                            struct lsc_adv_calc_param *param,
                            struct lsc_adv_calc_result *result);
int32_t lsc_adv_deinit(lsc_adv_handle_t handle, void *param, void *result) {
    return 0;
}

#include "isp_calibration_lsc.h"
int32_t
isp_calibration_lsc_calc(struct isp_calibration_lsc_calc_in *in_param,
                         struct isp_calibration_lsc_calc_out *out_param) {
    return 0;
}
int32_t isp_calibration_lsc_get_golden_info(
    void *golden_data, uint32_t golden_data_size,
    struct isp_calibration_lsc_golden_info *golden_info) {
    return 0;
}

#endif

#if 0
#include "AF_Interface.h"

void AF_Start_Debug(sft_af_handle_t handle)
{
}

void AF_Stop_Debug(sft_af_handle_t handle)
{
}

void AF_Start(sft_af_handle_t handle, BYTE bTouch)
{
}

void AF_Running(sft_af_handle_t handle)
{
}

void AF_Stop(sft_af_handle_t handle)
{
}

void AF_Prv_Start_Notice(	sft_af_handle_t handle)
{
}

void AF_Deinit_Notice(	sft_af_handle_t handle)
{
}

void AF_Set_Mode(sft_af_handle_t handle, uint32_t mode)
{
}

#include "AF_Main.h"
void AF_SetDefault(	sft_af_handle_t handle)
{
	return;
}
void AF_Control_CAF(		AFDATA *afCtl, 	AFDATA_RO *afv, AFDATA_FV *fv,	BYTE bEnb)
{
	return;
}

#endif

#ifndef CONFIG_CAMERA_AFL_AUTO_DETECTION
int antiflcker_sw_init() { return 0; }

int antiflcker_sw_deinit() { return 0; }

int antiflcker_sw_process(int input_img_width, int input_img_height,
                          int *debug_sat_img_H_scaling, int exposure_time,
                          int reg_flicker_thrd_frame,
                          int reg_flicker_thrd_frame_still,
                          int reg_flicker_thrd_video_still) {
    return 0;
}
#endif

size_t strlcpy(register char *dst, register const char *src, size_t n) {
    const char *src0 = src;
    char dummy[1];

    if (!n) {
        dst = dummy;
    } else {
        --n;
    }

    while ((*dst = *src) != 0) {
        if (n) {
            --n;
            ++dst;
        }
        ++src;
    }

    return src - src0;
}

int uv_proc_func_neon58(void *param) { return 0; }
int uv_proc_func_neon0(void *param_uv_in) { return 0; }
int uv_proc_func_neon1(void *param_uv_in) { return 0; }
int uv_proc_func_neon2(void *param_uv_in) { return 0; }
int uv_proc_func_neon730(void *param_uv_in) { return 0; }

size_t __strlen_chk(const char *s, size_t s_len) {
    size_t ret = strlen(s);
    return ret;
}

#if (MINICAMERA == 1)
int property_get(const char *key, char *value, const char *default_value) {
    strcpy(value, default_value);
}
#endif

static pthread_attr_t attr[100];
int threadcnt = 0;

void pthread_hook(pthread_t __th) {
    if (!pthread_getattr_np(__th, &attr[threadcnt])) {
        threadcnt++;
    } else {
        ALOGW("lookat: pthread_getattr_np %d fail\n", threadcnt);
    }
}

void pthread_stackshow() {
    int i, j;
    unsigned int *pstack;
    size_t size;

    ALOGW("lookat: thread total %d\n", threadcnt);
    for (i = threadcnt - 1; i > 0; i--) {
        ALOGW("lookat: thread %d\n", i);
        if (!pthread_attr_getstack(&attr[i], &pstack, &size) && pstack) {
            ALOGW("lookat: statckinfo: 0x%x, %p\n", size, pstack);
            for (j = 0; j < size / sizeof(unsigned int *); j++) {
                ALOGW("lookat: statck %d, %p\n", j, pstack[j]);
            }
        }
    }
}
