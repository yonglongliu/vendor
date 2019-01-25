#define LOG_TAG "hw_sensor"

#include "hw_sensor_drv.h"
#include <cutils/trace.h>

#define CHECK_HANDLE(handle)                                                   \
    if (NULL == handle) {                                                      \
        ALOGE("Handle is invalid " #handle);                                   \
        return CMR_CAMERA_FAIL;                                                \
    }
#define CHECK_PTR(expr)                                                        \
    if ((expr) == NULL) {                                                      \
        ALOGE("ERROR: NULL pointer detected " #expr);                          \
        return HW_FAILED;                                                      \
    }

struct hw_drv_cxt {
    /*sensor device file descriptor*/
    cmr_s32 fd_sensor;
    /*sensor_id will be used mipi init*/
    cmr_u32 sensor_id;
    /*0-bit:reg value width,1-bit:reg addre width*/
    /*5,6,7-bit:i2c frequency*/
    cmr_u8 i2c_bus_config;
    /*sensor module handle,will be deleted later*/
    cmr_handle caller_handle; /**/
    /**/
};

static cmr_int _hw_sensor_dev_init(cmr_handle hw_handle, cmr_u32 sensor_id) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = HW_SUCCESS;
    cmr_s8 sensor_dev_name[50] = HW_SENSOR_DEV_NAME;

    CHECK_HANDLE(hw_handle);
    struct hw_drv_cxt *hw_drv_cxt = (struct hw_drv_cxt *)hw_handle;

    HW_LOGI("start open sensor device node.");

    if (SENSOR_FD_INIT == hw_drv_cxt->fd_sensor) {
        hw_drv_cxt->fd_sensor = open(HW_SENSOR_DEV_NAME, O_RDWR, 0);
        HW_LOGI("fd 0x%lx", hw_drv_cxt->fd_sensor);
        if (SENSOR_FD_INIT == hw_drv_cxt->fd_sensor) {
            HW_LOGE("Failed to open sensor device.errno : %d", errno);
            fprintf(stderr, "Cannot open '%s': %d, %s", sensor_dev_name, errno,
                    strerror(errno));
        } else {
            HW_LOGV("OK to open device.");
            /*the sensor id should be set after open OK*/
            ret = ioctl(hw_drv_cxt->fd_sensor, SENSOR_IO_SET_ID, &sensor_id);
            if (0 != ret) {
                HW_LOGE("SENSOR_IO_SET_ID %u failed %ld", sensor_id, ret);
                ret = HW_FAILED;
            }
        }
    }

    ATRACE_END();
    return ret;
}

static cmr_int _hw_sensor_dev_deinit(cmr_handle hw_handle) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = HW_SUCCESS;
    CHECK_HANDLE(hw_handle);
    struct hw_drv_cxt *hw_drv_cxt = (struct hw_drv_cxt *)hw_handle;

    if (-1 != hw_drv_cxt->fd_sensor) {
        ret = close(hw_drv_cxt->fd_sensor);
        hw_drv_cxt->fd_sensor = SENSOR_FD_INIT;
        HW_LOGV("is done, ret = %ld ", ret);
    }
    ATRACE_END();
    return ret;
}

/**
 * hw_sensor_drv_cfg - config hardware interface
 * @hw_handle: hardware object.
 * @input_ptr: config parameter.
 *
 * NOTE:the interface not for customer.
 **/
cmr_int hw_sensor_drv_cfg(cmr_handle hw_handle,
                          struct hw_drv_cfg_param *input_ptr) {
    cmr_u8 ret = HW_SUCCESS;
    CHECK_HANDLE(hw_handle);
    struct hw_drv_cxt *hw_drv_cxt = (struct hw_drv_cxt *)hw_handle;
    hw_drv_cxt->i2c_bus_config = input_ptr->i2c_bus_config;
    /*add other info if you need*/
    return ret;
}

/**
 * hw_sensor_drv_create - power on sensor sequence
 * @input_ptr: hardware init parameter.
 * @hw_drv_handle: hardware instance pointer.
 *
 **/
cmr_int hw_sensor_drv_create(struct hw_drv_init_para *input_ptr,
                             cmr_handle *hw_drv_handle) {
    cmr_u8 ret = HW_SUCCESS;
    struct hw_drv_cxt *hw_drv_cxt = NULL;
    CHECK_PTR(input_ptr);
    HW_LOGI("sensor_id:%d", input_ptr->sensor_id);

    *hw_drv_handle = (struct hw_drv_cxt *)malloc(sizeof(struct hw_drv_cxt));
    if (NULL == *hw_drv_handle) {
        HW_LOGE("hw_drv handle malloc failed\n");
        return HW_FAILED;
    }
    hw_drv_cxt = *hw_drv_handle;
    hw_drv_cxt->fd_sensor = SENSOR_FD_INIT;
    hw_drv_cxt->sensor_id = input_ptr->sensor_id;
    hw_drv_cxt->caller_handle = input_ptr->caller_handle;
    ret = _hw_sensor_dev_init(*hw_drv_handle, input_ptr->sensor_id);

    return hw_drv_cxt->fd_sensor;
}

/**
 * hw_sensor_drv_delete - delete hw instance.
 * @hw_handle: hardware object.
 **/
cmr_int hw_sensor_drv_delete(cmr_handle hw_handle) {

    cmr_u8 ret = HW_SUCCESS;
    CHECK_HANDLE(hw_handle);
    HW_LOGI("hw dev delete");
    ret = _hw_sensor_dev_deinit(hw_handle);
    free(hw_handle);

    return ret;
}

/**
 * hw_sensor_drv_ioctl - extended interface.
 *
 * NOTE: add some new function here if you want.
 **/
cmr_int hw_sensor_drv_ioctl(cmr_handle hw_handle, int cmd, void *param) {
    cmr_u8 ret = HW_SUCCESS;
    CHECK_HANDLE(hw_handle);
    CHECK_HANDLE(param);
    HW_LOGI("hw dev ioctl");
    ret = HW_SUCCESS;
    /*add your cmd here*/
    switch (cmd) {
    default:
        break;
    }
    return ret;
}

static cmr_int _hw_sensor_set_avdd(cmr_handle hw_handle, cmr_u32 vdd_value) {
    cmr_int ret = HW_SUCCESS;
    CHECK_HANDLE(hw_handle);
    struct hw_drv_cxt *hw_drv_cxt = (struct hw_drv_cxt *)hw_handle;

    ret = ioctl(hw_drv_cxt->fd_sensor, SENSOR_IO_SET_AVDD, &vdd_value);
    if (0 != ret) {
        HW_LOGE("failed,  vdd_value = %d, ret=%ld ", vdd_value, ret);
        ret = -1;
    }

    return ret;
}

static cmr_int _hw_sensor_set_dvdd(cmr_handle hw_handle, cmr_u32 vdd_value) {
    cmr_int ret = HW_SUCCESS;
    CHECK_HANDLE(hw_handle);
    struct hw_drv_cxt *hw_drv_cxt = (struct hw_drv_cxt *)hw_handle;

    ret = ioctl(hw_drv_cxt->fd_sensor, SENSOR_IO_SET_DVDD, &vdd_value);
    if (0 != ret) {
        HW_LOGE("failed,  vdd_value = %d, ret=%ld ", vdd_value, ret);
        ret = HW_FAILED;
    }

    return ret;
}

static cmr_int _hw_sensor_set_iovdd(cmr_handle hw_handle, cmr_u32 vdd_value) {
    cmr_int ret = HW_SUCCESS;
    CHECK_HANDLE(hw_handle);
    struct hw_drv_cxt *hw_drv_cxt = (struct hw_drv_cxt *)hw_handle;

    ret = ioctl(hw_drv_cxt->fd_sensor, SENSOR_IO_SET_IOVDD, &vdd_value);
    if (0 != ret) {
        HW_LOGE("failed,  vdd_value = %d, ret=%ld ", vdd_value, ret);
        ret = -1;
    }

    return ret;
}

/**
 * hw_sensor_set_voltage - power on sensor sequence
 * @hw_handle: hardware object.
 *
 * NOTE: this interface will be deleted,please use
 *       hw_sensor_set_avdd_val,hw_sensor_set_dvdd_val,
 *       hw_sensor_set_iovdd_val interface.
 **/
cmr_int hw_sensor_set_voltage(cmr_handle hw_handle, SENSOR_AVDD_VAL_E dvdd_val,
                              SENSOR_AVDD_VAL_E avdd_val,
                              SENSOR_AVDD_VAL_E iodd_val) {
    cmr_int ret = HW_SUCCESS;
    CHECK_HANDLE(hw_handle);

    ret = _hw_sensor_set_dvdd(hw_handle, (cmr_u32)dvdd_val);
    if (HW_SUCCESS != ret)
        return ret;

    ret = _hw_sensor_set_avdd(hw_handle, (cmr_u32)avdd_val);
    if (HW_SUCCESS != ret)
        return ret;

    ret = _hw_sensor_set_iovdd(hw_handle, (cmr_u32)iodd_val);
    if (HW_SUCCESS != ret)
        return ret;

    HW_LOGI("avdd_val = %d,  dvdd_val=%d, iodd_val=%d ", avdd_val, dvdd_val,
            iodd_val);

    return ret;
}

cmr_int hw_sensor_set_avdd_val(cmr_handle hw_handle,
                               SENSOR_AVDD_VAL_E vdd_val) {
    cmr_int ret = HW_SUCCESS;
    CHECK_HANDLE(hw_handle);

    ret = _hw_sensor_set_avdd(hw_handle, (cmr_u32)vdd_val);
    HW_LOGI("vdd_val is %d, set result is =%ld ", vdd_val, ret);
    return ret;
}

cmr_int hw_sensor_set_dvdd_val(cmr_handle hw_handle,
                               SENSOR_AVDD_VAL_E vdd_val) {
    cmr_int ret = HW_SUCCESS;
    CHECK_HANDLE(hw_handle);
    struct hw_drv_cxt *hw_drv_cxt = (struct hw_drv_cxt *)hw_handle;

    ret = _hw_sensor_set_dvdd(hw_handle, (cmr_u32)vdd_val);
    HW_LOGI("vdd_val is %d, set result is =%ld ", vdd_val, ret);
    return ret;
}

cmr_int hw_sensor_set_iovdd_val(cmr_handle hw_handle,
                                SENSOR_AVDD_VAL_E vdd_val) {
    cmr_int ret = HW_SUCCESS;
    CHECK_HANDLE(hw_handle);
    struct hw_drv_cxt *hw_drv_cxt = (struct hw_drv_cxt *)hw_handle;

    ret = _hw_sensor_set_iovdd(hw_handle, (cmr_u32)vdd_val);
    HW_LOGI("vdd_val is %d, set result is =%ld ", vdd_val, ret);
    return ret;
}

cmr_int hw_sensor_set_monitor_val(cmr_handle hw_handle,
                                  SENSOR_AVDD_VAL_E vdd_val) {
    cmr_int ret = HW_SUCCESS;

    CHECK_HANDLE(hw_handle);
    struct hw_drv_cxt *hw_drv_cxt = (struct hw_drv_cxt *)hw_handle;

    ret = ioctl(hw_drv_cxt->fd_sensor, SENSOR_IO_SET_CAMMOT, &vdd_val);
    if (0 != ret) {
        HW_LOGE("failed,  vdd_value = %d, ret=%ld ", vdd_val, ret);
        ret = HW_FAILED;
    }
    HW_LOGI("vdd_val = %d ", vdd_val);

    return ret;
}

cmr_int hw_sensor_power_config(cmr_handle hw_handle,
                               struct sensor_power_info_tag power_cfg) {
    cmr_int ret = HW_SUCCESS;

    CHECK_HANDLE(hw_handle);
    struct hw_drv_cxt *hw_drv_cxt = (struct hw_drv_cxt *)hw_handle;

    ret = ioctl(hw_drv_cxt->fd_sensor, SENSOR_IO_POWER_CFG, &power_cfg);
    if (0 != ret) {
        HW_LOGE("failed,  ret=%ld ", ret);
        ret = HW_FAILED;
    }
    ATRACE_END();
    return ret;
}

cmr_int hw_sensor_power_down(cmr_handle hw_handle, cmr_u32 power_level) {
    ATRACE_BEGIN(__FUNCTION__);
    cmr_int ret = HW_SUCCESS;

    CHECK_HANDLE(hw_handle);
    struct hw_drv_cxt *hw_drv_cxt = (struct hw_drv_cxt *)hw_handle;

    HW_LOGV("power_level %d", power_level);

    ret = ioctl(hw_drv_cxt->fd_sensor, SENSOR_IO_PD, &power_level);
    if (0 != ret) {
        HW_LOGE("failed,  power_level = %d, ret=%ld ", power_level, ret);
        ret = HW_FAILED;
    }

    ATRACE_END();
    return ret;
}


cmr_int hw_sensor_get_sensorid0(cmr_handle hw_handle, cmr_u32 level) {
    ATRACE_BEGIN(__FUNCTION__);
    cmr_int ret = HW_SUCCESS;

    CHECK_HANDLE(hw_handle);
    struct hw_drv_cxt *hw_drv_cxt = (struct hw_drv_cxt *)hw_handle;

    HW_LOGV("level %d", level);

    ret = ioctl(hw_drv_cxt->fd_sensor, SENSOR_IO_GET_CAMERAID0, &level);
    HW_LOGI("sensor get sensorid0  %d", ret);
    ATRACE_END();
    return ret;
}
cmr_int hw_sensor_get_sensorid1(cmr_handle hw_handle, cmr_u32 level) {
    ATRACE_BEGIN(__FUNCTION__);
    cmr_int ret = HW_SUCCESS;

    CHECK_HANDLE(hw_handle);
    struct hw_drv_cxt *hw_drv_cxt = (struct hw_drv_cxt *)hw_handle;

    ret = ioctl(hw_drv_cxt->fd_sensor, SENSOR_IO_GET_CAMERAID1, &level);
    HW_LOGI("sensor get sensorid1  %d", ret);
    ATRACE_END();
    return ret;
}

cmr_int hw_sensor_get_sensorid2(cmr_handle hw_handle, cmr_u32 level) {
    ATRACE_BEGIN(__FUNCTION__);
    cmr_int ret = HW_SUCCESS;

    CHECK_HANDLE(hw_handle);
    struct hw_drv_cxt *hw_drv_cxt = (struct hw_drv_cxt *)hw_handle;

    ret = ioctl(hw_drv_cxt->fd_sensor, SENSOR_IO_GET_CAMERAID2, &level);
    HW_LOGI("sensor get sensorid2  %d", ret);
    ATRACE_END();
    return ret;
}


cmr_int hw_sensor_reset(cmr_handle hw_handle, cmr_u32 level,
                        cmr_u32 pulse_width) {
    cmr_int ret = HW_SUCCESS;
    cmr_u32 rst_val[2];

    CHECK_HANDLE(hw_handle);
    struct hw_drv_cxt *hw_drv_cxt = (struct hw_drv_cxt *)hw_handle;
    HW_LOGV("in");

    rst_val[0] = level;
    rst_val[1] = pulse_width;
    if (rst_val[1] < SENSOR_RESET_PULSE_WIDTH_DEFAULT) {
        rst_val[1] = SENSOR_RESET_PULSE_WIDTH_DEFAULT;
    } else if (rst_val[1] > SENSOR_RESET_PULSE_WIDTH_MAX) {
        rst_val[1] = SENSOR_RESET_PULSE_WIDTH_MAX;
    }
    ret = ioctl(hw_drv_cxt->fd_sensor, SENSOR_IO_RST, rst_val);
    if (ret) {
        ret = HW_FAILED;
    }
    HW_LOGV("OK out.");
    return ret;
}

cmr_int hw_sensor_set_reset_level(cmr_handle hw_handle, cmr_u32 plus_level) {
    ATRACE_BEGIN(__FUNCTION__);
    cmr_int ret = HW_SUCCESS;
    CHECK_HANDLE(hw_handle);
    struct hw_drv_cxt *hw_drv_cxt = (struct hw_drv_cxt *)hw_handle;

    ret = ioctl(hw_drv_cxt->fd_sensor, SENSOR_IO_RST_LEVEL, &plus_level);
    if (0 != ret) {
        HW_LOGE("failed,  level = %d, ret=%ld ", plus_level, ret);
        ret = -1;
    }

    return ret;
}

cmr_int hw_sensor_write_data(cmr_handle hw_handle, cmr_u8 *regPtr,
                             cmr_u32 length) {
    cmr_int ret = HW_SUCCESS;

    CHECK_HANDLE(hw_handle);
    CHECK_PTR(regPtr);
    struct hw_drv_cxt *hw_drv_cxt = (struct hw_drv_cxt *)hw_handle;

    ret = write(hw_drv_cxt->fd_sensor, regPtr, length);
    if (0 != ret) {
        HW_LOGE("failed,  buff[0] = %d, size=%d, ret=%ld ", regPtr[0], length,
                ret);
        ret = HW_FAILED;
    }

    return ret;
}

cmr_int hw_sensor_set_mclk(cmr_handle hw_handle, cmr_u32 mclk) {
    cmr_int ret = HW_SUCCESS;
    CHECK_HANDLE(hw_handle);
    struct hw_drv_cxt *hw_drv_cxt = (struct hw_drv_cxt *)hw_handle;

    ret = ioctl(hw_drv_cxt->fd_sensor, SENSOR_IO_SET_MCLK, &mclk);
    if (0 != ret) {
        HW_LOGE("failed,  mclk = %d, ret = %ld  ", mclk, ret);
        ret = HW_FAILED;
    }
    HW_LOGI("mclk %d, ret %ld ", mclk, ret);

    return ret;
}

cmr_int hw_sensor_mipi_init(cmr_handle hw_handle,
                            struct hw_mipi_init_param init_param) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = HW_SUCCESS;
    SENSOR_IF_CFG_T if_cfg;

    CHECK_HANDLE(hw_handle);
    struct hw_drv_cxt *hw_drv_cxt = (struct hw_drv_cxt *)hw_handle;

    cmr_u32 lane_num = init_param.lane_num;
    cmr_u32 bps = init_param.bps_per_lane;

    cmr_bzero((void *)&if_cfg, sizeof(SENSOR_IF_CFG_T));
    if_cfg.if_type = INTERFACE_MIPI;
    if_cfg.is_open = INTERFACE_OPEN;
    if_cfg.lane_num = lane_num;
    if_cfg.bps_per_lane = bps;

    HW_LOGI("Lane num %d, bps %d", lane_num, bps);
    ret = ioctl(hw_drv_cxt->fd_sensor, SENSOR_IO_IF_CFG, &if_cfg);
    if (0 != ret) {
        HW_LOGE("failed, ret=%ld, lane=%d, bps=%d,fd=%d", ret, lane_num, bps,
                hw_drv_cxt->fd_sensor);
        ret = -1;
    }
    // usleep(15*1000);
    ATRACE_END();
    return ret;
}

cmr_int hw_sensor_mipi_switch(cmr_handle hw_handle,
                              struct hw_mipi_init_param init_param) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = HW_SUCCESS;
    SENSOR_IF_CFG_T if_cfg;

    CHECK_HANDLE(hw_handle);
    struct hw_drv_cxt *hw_drv_cxt = (struct hw_drv_cxt *)hw_handle;

    cmr_u32 lane_num = init_param.lane_num;
    cmr_u32 bps = init_param.bps_per_lane;

    cmr_bzero((void *)&if_cfg, sizeof(SENSOR_IF_CFG_T));

    ret = ioctl(hw_drv_cxt->fd_sensor, SENSOR_IO_IF_SWITCH, &if_cfg);
    if (0 != ret) {
        HW_LOGE("failed, ret=%ld, fd=%d", ret, hw_drv_cxt->fd_sensor);
        ret = -1;
    }
    ATRACE_END();
    return ret;
}

cmr_int hw_sensor_mipi_deinit(cmr_handle hw_handle) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_int ret = HW_SUCCESS;
    SENSOR_IF_CFG_T if_cfg;
    CHECK_HANDLE(hw_handle);
    struct hw_drv_cxt *hw_drv_cxt = (struct hw_drv_cxt *)hw_handle;

    cmr_bzero((void *)&if_cfg, sizeof(SENSOR_IF_CFG_T));
    if_cfg.if_type = INTERFACE_MIPI;
    if_cfg.is_open = INTERFACE_CLOSE;

    ret = ioctl(hw_drv_cxt->fd_sensor, SENSOR_IO_IF_CFG, &if_cfg);
    if (0 != ret) {
        HW_LOGE("failed, 0x%lx", ret);
        ret = -1;
    }
    HW_LOGI("close mipi");

    ATRACE_END();
    return ret;
}

cmr_int hw_sensor_set_mipi_level(cmr_handle hw_handle, cmr_u32 plus_level) {
    cmr_u8 ret = HW_SUCCESS;

    CHECK_HANDLE(hw_handle);
    struct hw_drv_cxt *hw_drv_cxt = (struct hw_drv_cxt *)hw_handle;
    HW_LOGI("Sensor_SetMIPILevel IN");

    ret = ioctl(hw_drv_cxt->fd_sensor, SENSOR_IO_SET_MIPI_SWITCH, &plus_level);
    if (0 != ret) {
        HW_LOGE("failed,  level = %d, ret=%ld ", plus_level, ret);
        ret = HW_FAILED;
    }

    return ret;
}

cmr_int hw_sensor_i2c_init(cmr_handle hw_handle, cmr_u32 sensor_id) {
    cmr_int ret = HW_SUCCESS;

    CHECK_HANDLE(hw_handle);
    struct hw_drv_cxt *hw_drv_cxt = (struct hw_drv_cxt *)hw_handle;

    HW_LOGV("E");
    ret = ioctl(hw_drv_cxt->fd_sensor, SENSOR_IO_SET_ID, &sensor_id);
    if (0 != ret) {
        HW_LOGE("failed,  senor_id = %d, ret=%ld", sensor_id, ret);
        ret = HW_FAILED;
    }
    HW_LOGV("X");
    return ret;
}

cmr_int hw_sensor_i2c_deinit(cmr_handle hw_handle, cmr_u32 sensor_id) {
    UNUSED(sensor_id);
    cmr_int ret = HW_SUCCESS;

    CHECK_HANDLE(hw_handle);
    struct hw_drv_cxt *hw_drv_cxt = (struct hw_drv_cxt *)hw_handle;

    /*SENSOR_DRV_CHECK_ZERO(sensor_cxt);

    HW_LOGI("E");

    HW_LOGI("X, now dummy handle");*/
    return ret;
}

cmr_int hw_sensor_i2c_set_clk(cmr_handle hw_handle) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_u32 freq;
    cmr_u32 clock;
    cmr_int ret;

    CHECK_HANDLE(hw_handle);
    struct hw_drv_cxt *hw_drv_cxt = (struct hw_drv_cxt *)hw_handle;

    freq = hw_drv_cxt->i2c_bus_config & SENSOR_I2C_CLOCK_MASK;

    switch (freq) {
    case SENSOR_I2C_FREQ_20:
        clock = 20000;
        break;

    case SENSOR_I2C_FREQ_50:
        clock = 50000;
        break;

    case SENSOR_I2C_FREQ_100:
        clock = 100000;
        break;

    case SENSOR_I2C_FREQ_200:
        clock = 200000;
        break;

    case SENSOR_I2C_FREQ_400:
        clock = 400000;
        break;

    default:
        clock = 100000;
        HW_LOGI("no valid freq, set clock to 100k ");
        break;
    }

    HW_LOGI("clock = %d ", clock);

    ret = ioctl(hw_drv_cxt->fd_sensor, SENSOR_IO_SET_I2CCLOCK, &clock);
    if (0 != ret) {
        HW_LOGE("failed,  clock = %d, ret=%ld ", clock, ret);
        ret = HW_FAILED;
    }

    ATRACE_END();
    return ret;
}

cmr_int hw_sensor_i2c_set_addr(cmr_handle hw_handle, cmr_u16 i2c_dev_addr) {
    ATRACE_BEGIN(__FUNCTION__);
    cmr_int ret = HW_SUCCESS;
    cmr_u16 i2c_addr = i2c_dev_addr;

    CHECK_HANDLE(hw_handle);
    struct hw_drv_cxt *hw_drv_cxt = (struct hw_drv_cxt *)hw_handle;

    ret = ioctl(hw_drv_cxt->fd_sensor, SENSOR_IO_I2C_ADDR, &i2c_addr);
    if (0 != ret) {
        HW_LOGE("failed,  addr = 0x%x, ret=%ld ", i2c_addr, ret);
        ret = HW_FAILED;
    }

    ATRACE_END();
    return ret;
}

/**
 * hw_sensor_read_reg- sensor ic read register interface.
 * @hw_handle: hardware object.
 * @reg_addr: register address to read.
 *
 * NOTE: the interface can only be used by sensor ic.
 *       not for otp and vcm device.
 **/
cmr_int hw_sensor_read_reg(cmr_handle hw_handle, cmr_u16 reg_addr) {
    ATRACE_BEGIN(__FUNCTION__);
    cmr_u32 i = 0;
    cmr_u16 ret_val = 0xffff;
    cmr_int ret = -1;
    SENSOR_REG_BITS_T reg;

    CHECK_HANDLE(hw_handle);
    struct hw_drv_cxt *hw_drv_cxt = (struct hw_drv_cxt *)hw_handle;

    reg.reg_addr = reg_addr;
    reg.reg_bits = hw_drv_cxt->i2c_bus_config;

    ret = ioctl(hw_drv_cxt->fd_sensor, SENSOR_IO_I2C_READ, &reg);
    if (0 != ret) {
        HW_LOGE("failed,  addr = 0x%x, value=0x%x, bit=%d, ret=%ld ",
                reg.reg_addr, reg.reg_value, reg.reg_bits, ret);
        ret = HW_FAILED;
    } else if (HW_SUCCESS == ret) {
        ret_val = reg.reg_value;
    }
    ATRACE_END();

    return ret_val;
}

/**
 * hw_sensor_read_reg_8bits - sensor ic read register(8bit) interface
 * @hw_handle: hardware object.
 * @reg_addr: register address to read.
 * @reg_val : buffer to save data.
 *
 * NOTE: the interface will be delete,please use {@link hw_sensor_read_reg}
 **/
cmr_int hw_sensor_read_reg_8bits(cmr_handle hw_handle, cmr_u8 reg_addr,
                                 cmr_u8 *reg_val) {
    ATRACE_BEGIN(__FUNCTION__);
    cmr_u32 i = 0;
    cmr_u16 ret_val = 0xffff;
    cmr_int ret = -1;
    SENSOR_REG_BITS_T reg;

    CHECK_HANDLE(hw_handle);
    struct hw_drv_cxt *hw_drv_cxt = (struct hw_drv_cxt *)hw_handle;

    reg.reg_addr = reg_addr;
    reg.reg_bits = SENSOR_I2C_REG_8BIT | SENSOR_I2C_VAL_8BIT;

    ret = ioctl(hw_drv_cxt->fd_sensor, SENSOR_IO_I2C_READ, &reg);
    if (0 != ret) {
        HW_LOGE("failed,  addr = 0x%x, value=0x%x, bit=%d, ret=%ld ",
                reg.reg_addr, reg.reg_value, reg.reg_bits, ret);
        ret = HW_FAILED;
    } else if (HW_SUCCESS == ret) {
        *reg_val = reg.reg_value;
    }
    ATRACE_END();

    return ret_val;
}

/**
 * hw_sensor_read_i2c - vcm and otp device read register interface
 * @hw_handle: hardware object.
 * @slave_addr: vcm and otp device address.
 * @cmd_buffer: buffer saved command and data.
 * @cmd_length: buffer length.
 *      cmd_length(32-16 bit): the size we want to read one time.
 *      cmd_lenght(15-0 bit): the register length we read.
 *
 * NOTE: the interface can only be used by vcm and otp device,not for
 *       sensor ic driver(mipi_raw.c).
 **/
cmr_int hw_sensor_read_i2c(cmr_handle hw_handle, cmr_u16 slave_addr,
                           cmr_u8 *cmd_buffer, cmr_u32 cmd_length) {
    ATRACE_BEGIN(__FUNCTION__);
    SENSOR_I2C_T i2c_tab;
    cmr_int ret = HW_SUCCESS;

    CHECK_HANDLE(hw_handle);
    CHECK_PTR(cmd_buffer);
    struct hw_drv_cxt *hw_drv_cxt = (struct hw_drv_cxt *)hw_handle;

    i2c_tab.slave_addr = slave_addr;
    i2c_tab.i2c_data = cmd_buffer;
    i2c_tab.i2c_count = cmd_length & 0xffff;
    i2c_tab.read_len = (cmd_length >> 16) & 0xffff;

    if ((((cmd_length >> 16) & 0xffff)) == 0)
        i2c_tab.read_len = cmd_length & 0xffff;

    HW_LOGV("Sensor_ReadI2C, slave_addr=0x%x, ptr=0x%p, count=%d\n",
            i2c_tab.slave_addr, i2c_tab.i2c_data, i2c_tab.i2c_count);

    ret = ioctl(hw_drv_cxt->fd_sensor, SENSOR_IO_GRC_I2C_READ, &i2c_tab);
    if (0 != ret) {
        HW_LOGE("failed to read i2c, slave_addr=0x%x, ptr=0x%p, count=%d\n",
                i2c_tab.slave_addr, i2c_tab.i2c_data, i2c_tab.i2c_count);
        ret = HW_FAILED;
    }
    ATRACE_END();

    return ret;
}

cmr_u16 hw_sensor_grc_read_i2c(cmr_handle hw_handle, cmr_u16 slave_addr,
                               cmr_u16 addr, cmr_int bits) {
    cmr_u16 ret = HW_SUCCESS;
    cmr_u8 cmd[2] = {0x00};
    cmr_u16 reg = 0;
    struct sensor_i2c_tag i2c_tab;

    CHECK_HANDLE(hw_handle);
    struct hw_drv_cxt *hw_drv_cxt = (struct hw_drv_cxt *)hw_handle;

    i2c_tab.slave_addr = slave_addr;

    switch (bits) {
    case BITS_ADDR8_REG8:
        cmd[0] = 0x00ff & addr;
        i2c_tab.i2c_data = cmd;
        i2c_tab.i2c_count = 1;
        i2c_tab.read_len = 1;
        break;
    case BITS_ADDR8_REG16:
        cmd[0] = 0x00ff & addr;
        i2c_tab.i2c_data = cmd;
        i2c_tab.i2c_count = 1;
        i2c_tab.read_len = 2;
        break;
    case BITS_ADDR16_REG8:
        cmd[0] = (0xff00 & addr) >> 8;
        cmd[1] = 0x00ff & addr;
        i2c_tab.i2c_data = cmd;
        i2c_tab.i2c_count = 2;
        i2c_tab.read_len = 1;
        break;
    case BITS_ADDR16_REG16:
        cmd[0] = (0xff00 & addr) >> 8;
        cmd[1] = 0x00ff & addr;
        i2c_tab.i2c_data = cmd;
        i2c_tab.i2c_count = 2;
        i2c_tab.read_len = 2;
        break;
    default:
        HW_LOGE("failed to set bits");
        goto exit;
        break;
    }
    ret = ioctl(hw_drv_cxt->fd_sensor, SENSOR_IO_GRC_I2C_READ, &i2c_tab);
    if (0 != ret) {
        HW_LOGE("failed to read i2c, slave_addr=0x%x, ptr=0x%p, count=%d\n",
                i2c_tab.slave_addr, i2c_tab.i2c_data, i2c_tab.i2c_count);
        ret = HW_FAILED;
        goto exit;
    }

    switch (bits) {
    case BITS_ADDR8_REG8:
    case BITS_ADDR16_REG8:
        reg = i2c_tab.i2c_data[0];
        break;
    case BITS_ADDR8_REG16:
    case BITS_ADDR16_REG16:
        reg = (0x00ff & i2c_tab.i2c_data[0]) << 8;
        reg = (0x00ff & i2c_tab.i2c_data[1]) | reg;
        break;
    default:
        HW_LOGE("failed to set bits");
        break;
    }
    return reg;

exit:
    return ret;
}

/**
 * hw_sensor_write_reg- sensor ic write register interface.
 * @hw_handle: hardware object.
 * @reg_addr: register address to write.
 *
 * NOTE: the interface can only be used by sensor ic.
 *       not for otp and vcm device.
 **/
cmr_int hw_sensor_write_reg(cmr_handle hw_handle, cmr_u16 subaddr,
                            cmr_u16 data) {
    ATRACE_BEGIN(__FUNCTION__);
    cmr_int ret = -1;
    SENSOR_REG_BITS_T reg;

    CHECK_HANDLE(hw_handle);
    struct hw_drv_cxt *hw_drv_cxt = (struct hw_drv_cxt *)hw_handle;

    reg.reg_addr = subaddr;
    reg.reg_value = data;
    reg.reg_bits = hw_drv_cxt->i2c_bus_config;

    ret = ioctl(hw_drv_cxt->fd_sensor, SENSOR_IO_I2C_WRITE, &reg);
    if (0 != ret) {
        HW_LOGE("failed,  addr = 0x%x, value=%x, bit=%d, ret=%ld ",
                reg.reg_addr, reg.reg_value, reg.reg_bits, ret);
        ret = HW_FAILED;
    }

    ATRACE_END();
    return ret;
}

/**
 * hw_sensor_write_reg_8bits - sensor ic write register(8bit) interface
 * @hw_handle: hardware object.
 * @subaddr: register address to write.
 * @data : data to write.
 *
 * NOTE: the interface will be delete,please use {@link hw_sensor_write_reg}
 **/
cmr_int hw_sensor_write_reg_8bits(cmr_handle hw_handle, cmr_u16 subaddr,
                                  cmr_u8 data) {
    ATRACE_BEGIN(__FUNCTION__);
    cmr_int ret = -1;
    SENSOR_REG_BITS_T reg;

    CHECK_HANDLE(hw_handle);
    struct hw_drv_cxt *hw_drv_cxt = (struct hw_drv_cxt *)hw_handle;

    reg.reg_addr = subaddr;
    reg.reg_value = data;
    reg.reg_bits = SENSOR_I2C_REG_8BIT | SENSOR_I2C_VAL_8BIT;

    ret = ioctl(hw_drv_cxt->fd_sensor, SENSOR_IO_I2C_WRITE, &reg);
    if (0 != ret) {
        HW_LOGE("failed,  addr = 0x%x, value=%x, bit=%d, ret=%ld ",
                reg.reg_addr, reg.reg_value, reg.reg_bits, ret);
        ret = HW_FAILED;
    }

    ATRACE_END();
    return ret;
}

/**
 * hw_sensor_write_i2c - vcm and otp device write register interface
 * @hw_handle: hardware object.
 * @slave_addr: vcm and otp device address.
 * @cmd_buffer: buffer saved command and data.
 * @cmd_length: buffer length.
 *
 * NOTE: the interface can only be used by vcm and otp device,not for
 *       sensor ic driver(mipi_raw.c).
 **/
cmr_int hw_sensor_write_i2c(cmr_handle hw_handle, cmr_u16 slave_addr,
                            cmr_u8 *cmd_buffer, cmr_u16 cmd_length) {
    ATRACE_BEGIN(__FUNCTION__);
    cmr_int ret = HW_SUCCESS;
    SENSOR_I2C_T i2c_tab;

    CHECK_HANDLE(hw_handle);
    CHECK_PTR(cmd_buffer);
    struct hw_drv_cxt *hw_drv_cxt = (struct hw_drv_cxt *)hw_handle;

    i2c_tab.slave_addr = slave_addr;
    i2c_tab.i2c_data = cmd_buffer;
    i2c_tab.i2c_count = cmd_length;

    HW_LOGV("slave_addr=0x%x, ptr=0x%p, count=%d", i2c_tab.slave_addr,
            i2c_tab.i2c_data, i2c_tab.i2c_count);

    ret = ioctl(hw_drv_cxt->fd_sensor, SENSOR_IO_GRC_I2C_WRITE, &i2c_tab);
    if (0 != ret) {
        HW_LOGE("failed to write i2c, slave_addr=0x%x, ptr=0x%p, count=%d",
                i2c_tab.slave_addr, i2c_tab.i2c_data, i2c_tab.i2c_count);
        ret = HW_FAILED;
    }

    ATRACE_END();
    return ret;
}

cmr_int hw_sensor_muti_i2c_write(cmr_handle hw_handle,
                                 struct sensor_muti_aec_i2c_tag *aec_i2c_info) {
    cmr_int ret = HW_SUCCESS;
    CHECK_HANDLE(hw_handle);
    CHECK_PTR(aec_i2c_info);
    struct hw_drv_cxt *hw_drv_cxt = (struct hw_drv_cxt *)hw_handle;

    ret = ioctl(hw_drv_cxt->fd_sensor, SENSOR_IO_MUTI_I2C_WRITE, aec_i2c_info);
    if (0 != ret) {
        HW_LOGE("failed to write muti i2c");
        ret = HW_FAILED;
    }

    return ret;
}

cmr_u16 hw_sensor_grc_write_i2c(cmr_handle hw_handle, cmr_u16 slave_addr,
                                cmr_u16 addr, cmr_u16 reg, cmr_int bits) {
    cmr_int ret = HW_SUCCESS;
    cmr_u8 cmd[4] = {0x00};
    struct sensor_i2c_tag i2c_tab;
    CHECK_HANDLE(hw_handle);
    struct hw_drv_cxt *hw_drv_cxt = (struct hw_drv_cxt *)hw_handle;

    i2c_tab.slave_addr = slave_addr;

    switch (bits) {
    case BITS_ADDR8_REG8:
        cmd[0] = 0x00ff & addr;
        cmd[1] = 0x00ff & reg;
        i2c_tab.i2c_data = cmd;
        i2c_tab.i2c_count = 2;
        break;
    case BITS_ADDR8_REG16:
        cmd[0] = 0x00ff & addr;
        cmd[1] = (0xff00 & reg) >> 8;
        cmd[2] = 0x00ff & reg;
        i2c_tab.i2c_data = cmd;
        i2c_tab.i2c_count = 3;
        break;
    case BITS_ADDR16_REG8:
        cmd[0] = (0xff00 & addr) >> 8;
        cmd[1] = 0x00ff & addr;
        cmd[2] = 0x00ff & reg;
        i2c_tab.i2c_data = cmd;
        i2c_tab.i2c_count = 3;
        break;
    case BITS_ADDR16_REG16:
        cmd[0] = (0xff00 & addr) >> 8;
        cmd[1] = 0x00ff & addr;
        cmd[2] = (0xff00 & reg) >> 8;
        cmd[3] = 0x00ff & reg;
        i2c_tab.i2c_data = cmd;
        i2c_tab.i2c_count = 4;
        break;
    default:
        HW_LOGE("failed to set bits");
        goto exit;
        break;
    }
    ret = ioctl(hw_drv_cxt->fd_sensor, SENSOR_IO_GRC_I2C_WRITE, &i2c_tab);
    if (0 != ret) {
        HW_LOGE("failed to write i2c, slave_addr=0x%x, ptr=0x%p, count=%d",
                i2c_tab.slave_addr, i2c_tab.i2c_data, i2c_tab.i2c_count);
        ret = -1;
    }

exit:
    return ret;

    return reg;
}

cmr_int hw_sensor_get_flash_level(cmr_handle hw_handle,
                                  struct sensor_flash_level *level) {
    int ret = HW_SUCCESS;
    CHECK_HANDLE(hw_handle);
    struct hw_drv_cxt *hw_drv_cxt = (struct hw_drv_cxt *)hw_handle;

    ret = ioctl(hw_drv_cxt->fd_sensor, SENSOR_IO_GET_FLASH_LEVEL, level);
    if (0 != ret) {
        HW_LOGE("_Sensor_Device_GetFlashLevel failed, ret=%d \n", ret);
        ret = HW_FAILED;
    }

    return ret;
}

// TBSPLIT
static cmr_int _hw_sensor_dev_WriteRegTab(cmr_handle hw_handle,
                                          SENSOR_REG_TAB_PTR reg_tab) {
    cmr_int ret = HW_SUCCESS;
    CHECK_HANDLE(hw_handle);
    struct hw_drv_cxt *hw_drv_cxt = (struct hw_drv_cxt *)hw_handle;

    ret = ioctl(hw_drv_cxt->fd_sensor, SENSOR_IO_I2C_WRITE_REGS, reg_tab);
    if (0 != ret) {
        HW_LOGE("failed,  ptr=%p, count=%d, bits=%d, burst=%d ",
                reg_tab->sensor_reg_tab_ptr, reg_tab->reg_count,
                reg_tab->reg_bits, reg_tab->burst_mode);

        ret = HW_FAILED;
    }

    return ret;
}

cmr_int
hw_Sensor_SendRegTabToSensor(cmr_handle hw_handle,
                             SENSOR_REG_TAB_INFO_T *sensor_reg_tab_info_ptr) {
    ATRACE_BEGIN(__FUNCTION__);

    cmr_u32 i;
    cmr_u16 subaddr;
    cmr_u16 data;
    cmr_u8 ret = -1;
    CHECK_HANDLE(hw_handle);
    CHECK_PTR(sensor_reg_tab_info_ptr);
    struct hw_drv_cxt *hw_drv_cxt = (struct hw_drv_cxt *)hw_handle;
    HW_LOGV("E");

    SENSOR_REG_TAB_T regTab;
    regTab.reg_count = sensor_reg_tab_info_ptr->reg_count;
    regTab.reg_bits = hw_drv_cxt->i2c_bus_config;
    regTab.burst_mode = 0;
    regTab.sensor_reg_tab_ptr = sensor_reg_tab_info_ptr->sensor_reg_tab_ptr;

    ret = _hw_sensor_dev_WriteRegTab(hw_handle, &regTab);
    if ((0 != sensor_reg_tab_info_ptr->ext_reg_count) && (0 == ret) &&
        (NULL != sensor_reg_tab_info_ptr->sensor_ext_reg_tab_ptr)) {
        HW_LOGV("write expend register tab value: count=%d",
                sensor_reg_tab_info_ptr->ext_reg_count);
        regTab.reg_count = sensor_reg_tab_info_ptr->ext_reg_count;
        regTab.sensor_reg_tab_ptr =
            sensor_reg_tab_info_ptr->sensor_ext_reg_tab_ptr;
        ret = _hw_sensor_dev_WriteRegTab(hw_handle, &regTab);
    }
    HW_LOGI("reg_count %d, senosr_id: %ld", sensor_reg_tab_info_ptr->reg_count,
            hw_drv_cxt->sensor_id);

    HW_LOGV("X");

    ATRACE_END();
    return ret;
}
