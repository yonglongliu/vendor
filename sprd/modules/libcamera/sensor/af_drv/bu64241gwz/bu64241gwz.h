#ifndef _BU64241_H_
#define _BU64241_H_
#include "sns_af_drv.h"

#define BU64241GWZ_VCM_SLAVE_ADDR (0x18 >> 1)

static uint32_t _BU64241GWZ_set_motor_bestmode(cmr_handle sns_af_drv_handle);
static uint32_t  _BU64241GWZ_get_test_vcm_mode(cmr_handle sns_af_drv_handle);
static uint32_t  _BU64241GWZ_set_test_vcm_mode(cmr_handle sns_af_drv_handle, char* vcm_mode);
static int _BU64241GWZ_drv_init(cmr_handle sns_af_drv_handle);
static int BU64241GWZ_drv_create(struct af_drv_init_para *input_ptr, cmr_handle* sns_af_drv_handle);
static int BU64241GWZ_drv_delete(cmr_handle sns_af_drv_handle, void* param);
static int BU64241GWZ_drv_set_pos(cmr_handle sns_af_drv_handle, uint16_t pos);
static int BU64241GWZ_drv_get_pos(cmr_handle sns_af_drv_handle, uint16_t *pos);
static int BU64241GWZ_drv_ioctl(cmr_handle sns_af_drv_handle, enum sns_cmd cmd, void* param);

#endif
