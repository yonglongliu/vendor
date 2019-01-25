#include "otp_common.h"
#include <string.h>

#if defined(__LP64__)
static char *otp_lib_path = "/system/lib64/libcamotp.so";
#else
static char *otp_lib_path = "/system/lib/libcamotp.so";
#endif

static char *otp_bin_path = "/data/misc/cameraserver/";
/**
 * NOTE: the pointer point otp raw data that not be parsed.
 *       General the data size is 8k,however it also depends
 *       on the specific configuration of the OTP driver.
 *       There are four element for multi camera otp handle.
 *       you can get you otp data by sensor id.
 *
 * Memory Map:
 *       --------------------------------------
 *       |   flag: otp raw data has been read |
 *       |        into memory(4-Byte)         |
 *       |   (1:In memory  0: Not in memory)  |
 *        -------------------------------------
 *       | the following otp data size(4-Byte)|
 *        -------------------------------------
 *       |    otp raw data (general is 8K)    |
 *        -------------------------------------
 **/
static cmr_u8 *otp_raw_buffer[4] = {NULL, NULL, NULL, NULL};

/**
 * NOTE: the pointer point otp formatted data that has be parsed.
 *       The data size depends on the specific configuration of
 *       the OTP driver.There are four element for multi camera
 *       otp handle.You can get you otp parsed data by sensor id.
 *
 * Memory Map:
 *        --------------------------------------------
 *       | the following otp parsed data size(4-Byte)|
 *        --------------------------------------------
 *       |              otp parsed data              |
 *        --------------------------------------------
 **/
static cmr_u8 *otp_formatted_data_buffer[4] = {NULL, NULL, NULL, NULL};

/** sensor_otp_rw_data_from_file:
 *  @otp_drv_handle: sensor driver instance.
 *  @cmd: the command of read or write otp
 *          data from binary file.
 * NOTE:
 * This function executes before sensor ready to read otp raw data
 * from sensor eeprom. If the formated binary file exists,
 * otp driver will load it to a buffer.But if doesn't exist,the
 * function will return.
 *
 * Return:
 * CMR_CAMERA_SUCCESS : read or write otp from file success
 * CMR_CAMERA_FAILURE : read or write otp from file failed
 **/

cmr_int sensor_otp_rw_data_from_file(cmr_u8 cmd, char *file_name,
                                     void **otp_data, long *otp_size) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    OTP_LOGV("E");
    CHECK_PTR((void *)otp_data);

    char otp_bin_ext_path[255];
    FILE *fp = NULL;

    snprintf(otp_bin_ext_path, sizeof(otp_bin_ext_path), "%s%s", otp_bin_path,
             file_name);

    switch (cmd) {
    case OTP_WRITE_FORMAT_TO_BIN:
        OTP_LOGI("write otp data:%s", otp_bin_ext_path);
        fp = fopen(otp_bin_ext_path, "wb");
        if (fp != NULL) {
            fwrite(*otp_data, 1, *otp_size, fp);
            fclose(fp);
        } else {
            OTP_LOGE("fp is null!!,write otp bin failed");
            ret = OTP_CAMERA_FAIL;
        }
        break;
    case OTP_READ_FORMAT_FROM_BIN: {
        OTP_LOGI("read data:%s", otp_bin_ext_path);
        int try_time = 3, mRead = 0;
        if (-1 != access(otp_bin_ext_path, 0)) {
            fp = fopen(otp_bin_ext_path, "rb");
            if (fp != NULL) {
                fseek(fp, 0L, SEEK_END);
                *otp_size = ftell(fp); // get bin size
                *otp_data = malloc(*otp_size);
                if (*otp_data) {
                    while (try_time--) {
                        fseek(fp, 0, SEEK_SET);
                        mRead = fread(*otp_data, 1, *otp_size, fp);
                        if (mRead == *otp_size)
                            break;
                        else
                            OTP_LOGE("error:otp lenght doesn't "
                                     "match,Read:0x%x,otp_data_len:0x%lx",
                                     mRead, *otp_size);
                    }
                    fclose(fp);
                    if ((!try_time) && (mRead != *otp_size)) {
                        free(*otp_data);
                        *otp_data = NULL;
                        ret = OTP_CAMERA_FAIL;
                    }
                } else {
                    OTP_LOGE("malloc otp data buffer failed");
                    fclose(fp);
                    ret = OTP_CAMERA_FAIL;
                }
            } else {
                OTP_LOGE("fp is null!!,read otp bin failed");
                ret = OTP_CAMERA_FAIL;
            }
        } else {
            OTP_LOGE("file don't exist");
            ret = OTP_CAMERA_FAIL;
        }
    } break;
    default:
        OTP_LOGE("your cmd is wrong,please check you cmd");
        ret = OTP_CAMERA_FAIL;
        break;
    }

    OTP_LOGV("X");
    return ret;
}

/** sensor_otp_lsc_decompress:
 *  @handle: sensor driver instance.
 *
 * NOTE:
 * decompress otp data read from otp eeprom.
 *
 * This function executes in module sensor context
 *
 * Return:
 * SENSOR_SUCCESS : decompress success
 * SENSOR_FAILURE : decompress failed
 **/
#if defined(CONFIG_CAMERA_ISP_DIR_3)
cmr_int sensor_otp_lsc_decompress(otp_base_info_cfg_t *otp_base_info,
                                  lsccalib_data_t *lsc_cal_data) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_base_info);
    CHECK_PTR(lsc_cal_data);

    cmr_u32 compress_bits_size;
    cmr_u32 channal_cmp_bytes_size = 0;
    cmr_u32 one_channal_decmp_size = 0;
    cmr_u32 gain_compressed_bits = 0;
    cmr_u32 gain_mak_bits = 0;
    cmr_u32 random_buf_size = 0;
    cmr_u32 *random_buf = NULL;
    cmr_u32 *ptr_random_buf = NULL;
    cmr_u32 cmp_uncompensate_size = 0;
    cmr_u32 i;

    cmr_u16 *lsc_rdm_src_data = NULL, *lsc_rdm_dst_data = NULL;

    /* get random lsc data */
    lsc_rdm_src_data = (cmr_u16 *)((cmr_u8 *)lsc_cal_data +
                                   lsc_cal_data->lsc_calib_random.offset);

    compress_bits_size = otp_base_info->gain_width *
                         otp_base_info->gain_height * GAIN_COMPRESSED_14BITS;
    channal_cmp_bytes_size = (compress_bits_size + GAIN_ORIGIN_BITS - 1) /
                             GAIN_ORIGIN_BITS * (GAIN_ORIGIN_BITS / 8);

    if (0 == channal_cmp_bytes_size) {
        return -1;
    }
    cmp_uncompensate_size = otp_base_info->gain_width *
                            otp_base_info->gain_height * gain_compressed_bits %
                            GAIN_ORIGIN_BITS;

    /*malloc random temp buffer*/
    random_buf_size = otp_base_info->gain_width * otp_base_info->gain_height *
                      sizeof(uint16_t) * CHANNAL_NUM;
    lsc_rdm_dst_data = (cmr_u16 *)malloc(random_buf_size);
    if (NULL == lsc_rdm_dst_data) {
        ret = -1;
        OTP_LOGE("malloc decompress buf failed!");
        goto exit;
    }

    gain_compressed_bits = GAIN_COMPRESSED_14BITS;
    gain_mak_bits = GAIN_MASK_14BITS;

    for (i = 0; i < CHANNAL_NUM; i++) {
        one_channal_decmp_size = sensor_otp_decompress_gain(
            lsc_rdm_src_data, channal_cmp_bytes_size, cmp_uncompensate_size,
            lsc_rdm_dst_data, gain_compressed_bits, gain_mak_bits);
        if (0 == one_channal_decmp_size) {
            ret = -1;
            goto exit;
        }
        lsc_rdm_src_data += channal_cmp_bytes_size / 2;
        lsc_rdm_dst_data += one_channal_decmp_size;
    }
    memcpy(lsc_rdm_src_data, lsc_rdm_dst_data, random_buf_size);
exit:

    if (NULL != lsc_rdm_dst_data) {
        free(lsc_rdm_dst_data);
        lsc_rdm_dst_data = NULL;
    }
    return ret;
}
#else
cmr_int sensor_otp_lsc_decompress(otp_base_info_cfg_t *otp_base_info,
                                  otp_section_info_t *lsc_cal_data) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_base_info);
    CHECK_PTR(lsc_cal_data);
    return ret;
}
#endif
/** sensor_otp_decompress_gain:
 *  @src: random lsc otp data.
 *  @src_bytes: the orign data size one channel.
 *  @src_uncompensate_bytes: the post data size of one channel.
 *  @dst:the destination buffer.
 *
 * NOTE:
 * The core decompression method.
 *
 * This function executes in module sensor context
 *
 * Return:
 * SENSOR_SUCCESS : decompress success
 * SENSOR_FAILURE : decompress failed
 **/
cmr_int sensor_otp_decompress_gain(cmr_u16 *src, cmr_u32 src_bytes,
                                   cmr_u32 src_uncompensate_bytes, cmr_u16 *dst,
                                   cmr_u32 GAIN_COMPRESSED_BITS,
                                   cmr_u32 GAIN_MASK) {
    cmr_u32 i = 0;
    cmr_u32 j = 0;
    cmr_u32 bit_left = 0;
    cmr_u32 bit_buf = 0;
    cmr_u32 offset = 0;
    cmr_u32 dst_gain_num = 0;

    if (NULL == src || NULL == dst) {
        return 0;
    }

    if (0 == src_bytes || 0 != (src_bytes & 1)) {
        return 0;
    }

    if (12 == GAIN_COMPRESSED_BITS) {
        for (i = 0; i < src_bytes / 2; i++) {
            bit_buf |= src[i] << bit_left;
            bit_left += GAIN_ORIGIN_BITS;

            if (bit_left > GAIN_COMPRESSED_BITS) {
                offset = 0;
                while (bit_left >= GAIN_COMPRESSED_BITS) {
                    dst[j] = (bit_buf & GAIN_MASK) << 2;
                    j++;
                    bit_left -= GAIN_COMPRESSED_BITS;
                    bit_buf = (bit_buf >> GAIN_COMPRESSED_BITS);
                }
            }
        }
    } else if (14 == GAIN_COMPRESSED_BITS) {
        for (i = 0; i < src_bytes / 2; i++) {
            bit_buf |= src[i] << bit_left;
            bit_left += GAIN_ORIGIN_BITS;

            if (bit_left > GAIN_COMPRESSED_BITS) {
                offset = 0;
                while (bit_left >= GAIN_COMPRESSED_BITS) {
                    dst[j] = (bit_buf & GAIN_MASK);
                    j++;
                    bit_left -= GAIN_COMPRESSED_BITS;
                    bit_buf = (bit_buf >> GAIN_COMPRESSED_BITS);
                }
            }
        }
    }

    if (GAIN_COMPRESSED_BITS == src_uncompensate_bytes) {
        dst_gain_num = j - 1;
    } else {
        dst_gain_num = j;
    }

    return dst_gain_num;
}

void sensor_otp_change_pattern(cmr_u32 pattern, cmr_u16 *interlaced_gain,
                               cmr_u16 *chn_gain[4], cmr_u16 gain_num) {
    cmr_u32 i = 0;
    cmr_u32 chn_idx[4] = {0};
    cmr_u16 *chn[4] = {NULL};

    chn[0] = chn_gain[0];
    chn[1] = chn_gain[1];
    chn[2] = chn_gain[2];
    chn[3] = chn_gain[3];

    switch (pattern) {
    case SENSOR_IMAGE_PATTERN_RGGB:
    default:
        chn_idx[0] = 0;
        chn_idx[1] = 1;
        chn_idx[2] = 2;
        chn_idx[3] = 3;
        break;

    case SENSOR_IMAGE_PATTERN_GRBG:
        chn_idx[0] = 1;
        chn_idx[1] = 0;
        chn_idx[2] = 3;
        chn_idx[3] = 2;
        break;

    case SENSOR_IMAGE_PATTERN_GBRG:
        chn_idx[0] = 2;
        chn_idx[1] = 3;
        chn_idx[2] = 0;
        chn_idx[3] = 1;
        break;

    case SENSOR_IMAGE_PATTERN_BGGR:
        chn_idx[0] = 3;
        chn_idx[1] = 2;
        chn_idx[2] = 1;
        chn_idx[3] = 0;
        break;
    }

    for (i = 0; i < gain_num; i++) {
        *interlaced_gain++ = *chn[chn_idx[0]]++;
        *interlaced_gain++ = *chn[chn_idx[1]]++;
        *interlaced_gain++ = *chn[chn_idx[2]]++;
        *interlaced_gain++ = *chn[chn_idx[3]]++;
    }
}
/** sensor_otp_dump_raw_data:
 *  @handle: sensor driver instance.
 *
 * NOTE:
 * Dump otp raw data,when debug.
 *
 * Return:
 * SENSOR_SUCCESS : decompress success
 * SENSOR_FAILURE : decompress failed
 **/
cmr_int sensor_otp_dump_raw_data(cmr_u8 *buffer, int size, char *dev_name) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    char value[255];
    char otp_bin_ext_path[255];

    CHECK_PTR(buffer);
    snprintf(otp_bin_ext_path, sizeof(otp_bin_ext_path), "%s%s_otp_dump.bin",
             otp_bin_path, dev_name);
    OTP_LOGD("otp_data_dump_path:%s", otp_bin_ext_path);
    FILE *fp = fopen(otp_bin_ext_path, "wb");
    if (fp != NULL) {
        fwrite(buffer, 1, size, fp);
        fclose(fp);
    } else {
        OTP_LOGE("fp is null!! dump otp raw data failed");
        ret = -1;
    }

    return ret;
}

cmr_int sensor_otp_dump_data2txt(cmr_u8 *buffer, int size, char *dev_name) {
    cmr_int ret = OTP_CAMERA_SUCCESS, i = 0;
    char value[255];
    char otp_bin_ext_path[255];

    snprintf(otp_bin_ext_path, sizeof(otp_bin_ext_path), "%s%s_otp_dump.txt",
             otp_bin_path, dev_name);

    FILE *fp_txt = fopen(otp_bin_ext_path, "w");
    if (fp_txt != NULL) {
        unsigned short *pbuffer = (unsigned short *)buffer;
        for (i = 0; i < (size / 2); i++) {
            fprintf(fp_txt, "0x%04x,", *pbuffer++);
            if (i % 16 == 15) {
                fprintf(fp_txt, "\n");
            }
        }
        fclose(fp_txt);
    } else {
        ret = OTP_CAMERA_FAIL;
    }

    return ret;
}
/** sensor_otp_drv_create:
 *  @handle: create sensor driver instance.
 *
 * NOTE:
 * The only interface provided to the upper layer.
 *
 * Return:
 * CMR_CAMERA_SUCCESS : read or write success.
 * CMR_CAMERA_FAILURE : read or write failed
 **/
cmr_int sensor_otp_drv_create(otp_drv_init_para_t *input_para,
                              cmr_handle *sns_otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(input_para);
    CHECK_PTR((void *)sns_otp_drv_handle);
    OTP_LOGV("In");

    otp_drv_cxt_t *otp_cxt = malloc(sizeof(otp_drv_cxt_t));
    if (!otp_cxt) {
        OTP_LOGE("otp handle create failed!");
        return OTP_CAMERA_FAIL;
    }
    memset(otp_cxt, 0, sizeof(otp_drv_cxt_t));
    if (input_para->sensor_name)
        memcpy(otp_cxt->dev_name, input_para->sensor_name, 32);
    otp_cxt->otp_raw_data.buffer = NULL;
    otp_cxt->otp_data = NULL;
    otp_cxt->hw_handle = input_para->hw_handle;
    otp_cxt->sensor_id = input_para->sensor_id;
    otp_cxt->sensor_ic_addr = input_para->sensor_ic_addr;
    OTP_LOGV("out");
    *sns_otp_drv_handle = otp_cxt;

    return ret;
}

/** sensor_otp_drv_delete:
 *  @handle: delete otp driver instance.
 *
 * NOTE:
 * The only interface provided to the upper layer.
 *
 * Return:
 * CMR_CAMERA_SUCCESS : read or write success.
 * CMR_CAMERA_FAILURE : read or write failed
 **/
cmr_int sensor_otp_drv_delete(void *otp_drv_handle) {
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("In");
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;

    if (otp_cxt->compat_convert_data) {
        free(otp_cxt->compat_convert_data);
        otp_cxt->compat_convert_data = NULL;
    }
    if (otp_cxt) {
        free(otp_cxt);
    }
    OTP_LOGV("out");
    return 0;
}

/** sensor_otp_drv_get_raw_buffer:
 *  @size: otp raw data size.
 *  @sensor_id: camera id
 *
 * Return: otp raw data buffer.
 **/
cmr_u8 *sensor_otp_get_raw_buffer(cmr_uint size, cmr_u32 sensor_id) {
    cmr_u32 cur_size = 0;
    OTP_LOGI("raw buffer:%p", otp_raw_buffer[sensor_id]);
    if (otp_raw_buffer[sensor_id] != NULL) {
        cur_size = *((cmr_u32 *)(otp_raw_buffer[sensor_id] + 4));
        if (cur_size != size) {
            free(otp_raw_buffer[sensor_id]);
            otp_raw_buffer[sensor_id] = malloc(size + 8);
            memset(otp_raw_buffer[sensor_id], 0, size);
            *((cmr_u32 *)(otp_raw_buffer[sensor_id] + 4)) = size;
        }
        return otp_raw_buffer[sensor_id] + 8;
    }
    otp_raw_buffer[sensor_id] = malloc(size + 8);
    memset(otp_raw_buffer[sensor_id], 0, size);
    *((cmr_u32 *)(otp_raw_buffer[sensor_id] + 4)) = size;
    return otp_raw_buffer[sensor_id] + 8;
}

cmr_u8 *sensor_otp_copy_raw_buffer(cmr_uint size, cmr_u32 sensor_id,
                                   cmr_u32 sensor_id2) {
    cmr_u32 cur_size = 0;
    OTP_LOGI("raw buffer:%p %d %d", otp_raw_buffer[sensor_id], sensor_id,
             sensor_id2);
    if (otp_raw_buffer[sensor_id] != NULL) {
        cur_size = *((cmr_u32 *)(otp_raw_buffer[sensor_id] + 4));
        /*      if (cur_size != size) {
                  free(otp_raw_buffer[sensor_id2]);
                  otp_raw_buffer[sensor_id2] = malloc(size + 8);
                  memset(otp_raw_buffer[sensor_id2], 0, size);
                  *((cmr_u32 *)(otp_raw_buffer[sensor_id2] + 4)) = size;
              }
              return otp_raw_buffer[sensor_id2] + 8;*/
    }
    otp_raw_buffer[sensor_id2] = malloc(size + 8);
    if (otp_raw_buffer[sensor_id] != NULL && cur_size >= size)
        memcpy(otp_raw_buffer[sensor_id2], otp_raw_buffer[sensor_id], size);
    *((cmr_u32 *)(otp_raw_buffer[sensor_id2] + 4)) = size;
    return otp_raw_buffer[sensor_id2] + 8;
}

cmr_u8 *sensor_otp_get_formatted_buffer(cmr_uint size, cmr_u32 sensor_id) {
    cmr_u32 cur_size = 0;
    OTP_LOGI("formatted buffer:%p", otp_formatted_data_buffer[sensor_id]);
    if (otp_formatted_data_buffer[sensor_id] != NULL) {
        cur_size = *((cmr_u32 *)otp_formatted_data_buffer[sensor_id]);
        if (cur_size != size) {
            free(otp_formatted_data_buffer[sensor_id]);
            otp_formatted_data_buffer[sensor_id] = malloc(size + 4);
            memset(otp_formatted_data_buffer[sensor_id], 0, size);
            *((cmr_u32 *)otp_formatted_data_buffer[sensor_id]) = size;
        }
        return otp_formatted_data_buffer[sensor_id] + 4;
    }
    otp_formatted_data_buffer[sensor_id] = malloc(size + 4);
    memset(otp_formatted_data_buffer[sensor_id], 0, size);
    *((cmr_u32 *)otp_formatted_data_buffer[sensor_id]) = size;
    return otp_formatted_data_buffer[sensor_id] + 4;
}

/** sensor_otp_set_buffer_state:
 *
 *  @sensor_id: camera id
 *  @state: buffer is in memory.
 *          0:not in memory.
 *          1:in memory and has been pared.
 **/
void sensor_otp_set_buffer_state(cmr_u32 sensor_id, cmr_u32 state) {
    if (otp_raw_buffer[sensor_id] != NULL) {
        *((cmr_u32 *)otp_raw_buffer[sensor_id]) = state;
    }
    OTP_LOGI("set buffer flag(0:not in memory,1:in memory):%d", state);
}

/*get buffer state*/
cmr_u32 sensor_otp_get_buffer_state(cmr_u32 sensor_id) {
    cmr_u32 in_memory = 0;
    if (otp_raw_buffer[sensor_id] != NULL) {
        in_memory = *((cmr_u32 *)otp_raw_buffer[sensor_id]);
    }
    OTP_LOGI("get buffer flag(0:not in memory,1:in memory):%d", in_memory);
    return in_memory;
}
