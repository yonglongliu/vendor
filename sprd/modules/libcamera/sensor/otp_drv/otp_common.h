#ifndef _OTP_COMMON_H_
#define _OTP_COMMON_H_

#include "cmr_common.h"
#include "sensor_drv_u.h"
#include <cutils/properties.h>
#include "otp_info.h"

cmr_int sensor_otp_rw_data_from_file(cmr_u8 cmd, char *sensor_name,
                                     void **otp_data, long *format_otp_size);
#if defined(CONFIG_CAMERA_ISP_DIR_3)
cmr_int sensor_otp_lsc_decompress(otp_base_info_cfg_t *otp_base_info,
                                  lsccalib_data_t *lsc_cal_data);
#else
cmr_int sensor_otp_lsc_decompress(otp_base_info_cfg_t *otp_base_info,
                                  otp_section_info_t *lsc_cal_data);
#endif
cmr_int sensor_otp_decompress_gain(cmr_u16 *src, cmr_u32 src_bytes,
                                   cmr_u32 src_uncompensate_bytes, cmr_u16 *dst,
                                   cmr_u32 GAIN_COMPRESSED_BITS,
                                   cmr_u32 GAIN_MASK);
void sensor_otp_change_pattern(cmr_u32 pattern, cmr_u16 *interlaced_gain,
                               cmr_u16 *chn_gain[4], cmr_u16 gain_num);
cmr_int sensor_otp_dump_raw_data(cmr_u8 *buffer, int size, char *dev_name);

cmr_int sensor_otp_dump_data2txt(cmr_u8 *buffer, int size, char *dev_name);

cmr_int sensor_otp_drv_create(otp_drv_init_para_t *input_para,
                              cmr_handle *sns_af_drv_handle);
cmr_int sensor_otp_drv_delete(void *otp_drv_handle);
cmr_u8 *sensor_otp_get_raw_buffer(cmr_uint size, cmr_u32 sensor_id);
cmr_u8 *sensor_otp_get_formatted_buffer(cmr_uint size, cmr_u32 sensor_id);
void sensor_otp_set_buffer_state(cmr_u32 sensor_id, cmr_u32 state);
cmr_u32 sensor_otp_get_buffer_state(cmr_u32 sensor_id);
cmr_u8 *sensor_otp_copy_raw_buffer(cmr_uint size, cmr_u32 sensor_id,
                                   cmr_u32 sensor_id2);

#endif
