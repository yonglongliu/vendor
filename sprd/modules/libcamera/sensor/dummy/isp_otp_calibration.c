#include <sys/types.h>
#include "isp_type.h"

int32_t isp_cur_bv;
uint32_t isp_cur_ct;

int32_t isp_calibration_get_info(void *golden_info, void *cali_info) {
    UNUSED(golden_info);
    UNUSED(cali_info);
    return 0;
}

uint32_t isp_raw_para_update_from_file(void *sensor_info_ptr, int sensor_id) {
    UNUSED(sensor_info_ptr);
    UNUSED(sensor_id);
    return 0;
}

int read_position(void *handler, uint32_t *pos) {
    UNUSED(handler);
    UNUSED(pos);
    return 0;
}

int read_otp_awb_gain(void *handler, void *awbc_cfg) {
    UNUSED(handler);
    UNUSED(awbc_cfg);
    return 0;
}

int read_sensor_shutter(uint32_t *shutter_val) {
    UNUSED(shutter_val);
    return 0;
}

int read_sensor_gain(uint32_t *gain_val) {
    UNUSED(gain_val);
    return 0;
}

int32_t isp_calibration(void *param, void *result) {
    UNUSED(param);
    UNUSED(result);
    return 0;
}
