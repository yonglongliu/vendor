#ifndef _HW_SENSOR_DRV_
#define _HW_SENSOR_DRV_
#include "cmr_common.h"
#include <fcntl.h>

/*kernel type redefine*/
typedef struct sensor_i2c_tag SENSOR_I2C_T, *SENSOR_I2C_T_PTR;
typedef struct sensor_reg_tag SENSOR_REG_T, *SENSOR_REG_T_PTR;
typedef struct sensor_reg_bits_tag SENSOR_REG_BITS_T, *SENSOR_REG_BITS_T_PTR;
typedef struct sensor_reg_tab_tag SENSOR_REG_TAB_T, *SENSOR_REG_TAB_PTR;
typedef struct sensor_flash_level SENSOR_FLASH_LEVEL_T;
typedef struct sensor_if_cfg_tag SENSOR_IF_CFG_T;
typedef struct sensor_socid_tag SENSOR_SOCID_T;
typedef struct sensor_version_info VERSION_INFO_T;

#define HW_LOGI SENSOR_LOGI
#define HW_LOGD SENSOR_LOGD
#define HW_LOGE SENSOR_LOGE
#define HW_LOGV SENSOR_LOGV

#define HW_SUCCESS CMR_CAMERA_SUCCESS
#define HW_FAILED CMR_CAMERA_FAIL

#define HW_SENSOR_DEV_NAME "/dev/sprd_sensor"

#define SENSOR_FD_INIT CMR_CAMERA_FD_INIT

#define SENSOR_RESET_PULSE_WIDTH_DEFAULT 10
#define SENSOR_RESET_PULSE_WIDTH_MAX 200

/*I2C REG/VAL BIT count*/
#define SENSOR_I2C_VAL_8BIT 0x00
#define SENSOR_I2C_VAL_16BIT 0x01
#define SENSOR_I2C_REG_8BIT (0x00 << 1)
#define SENSOR_I2C_REG_16BIT (0x01 << 1)
#define SENSOR_I2C_CUSTOM (0x01 << 2)

/*I2C ACK/STOP BIT count*/
#define SNESOR_I2C_ACK_BIT (0x00 << 3)
#define SNESOR_I2C_NOACK_BIT (0x00 << 3)
#define SNESOR_I2C_STOP_BIT (0x00 << 3)
#define SNESOR_I2C_NOSTOP_BIT (0x00 << 3)

/*I2C FEEQ BIT count*/
#define SENSOR_I2C_CLOCK_MASK (0x07 << 5)
#define SENSOR_I2C_FREQ_20 (0x01 << 5)
#define SENSOR_I2C_FREQ_50 (0x02 << 5)
#define SENSOR_I2C_FREQ_100 (0x00 << 5)
#define SENSOR_I2C_FREQ_200 (0x03 << 5)
#define SENSOR_I2C_FREQ_400 (0x04 << 5)

/*Hardward signal polarity*/
#define SENSOR_HW_SIGNAL_PCLK_N 0x00
#define SENSOR_HW_SIGNAL_PCLK_P 0x01
#define SENSOR_HW_SIGNAL_VSYNC_N (0x00 << 2)
#define SENSOR_HW_SIGNAL_VSYNC_P (0x01 << 2)
#define SENSOR_HW_SIGNAL_HSYNC_N (0x00 << 4)
#define SENSOR_HW_SIGNAL_HSYNC_P (0x01 << 4)

enum {
    BITS_ADDR8_REG8,
    BITS_ADDR8_REG16,
    BITS_ADDR16_REG8,
    BITS_ADDR16_REG16,
};

typedef enum {
    SENSOR_MAIN = 0,
    SENSOR_SUB,
    SENSOR_DEVICE2,
    SENSOR_DEVICE3,
    SENSOR_DEV_2,
    SENSOR_ATV = 5,
    SENSOR_ID_MAX
} SENSOR_ID_E;

typedef enum {
    SENSOR_AVDD_3800MV = 0,
    SENSOR_AVDD_3300MV,
    SENSOR_AVDD_3000MV,
    SENSOR_AVDD_2800MV,
    SENSOR_AVDD_2500MV,
    SENSOR_AVDD_2000MV,
    SENSOR_AVDD_1800MV,
    SENSOR_AVDD_1500MV,
    SENSOR_AVDD_1300MV,
    SENSOR_AVDD_1200MV,
    SENSOR_AVDD_1000MV,
    SENSOR_AVDD_CLOSED,
    SENSOR_AVDD_UNUSED
} SENSOR_AVDD_VAL_E;

typedef enum {
    SENSOR_MCLK_12M = 12,
    SENSOR_MCLK_13M = 13,
    SENSOR_MCLK_24M = 24,
    SENSOR_MCLK_26M = 26,
    SENSOR_MCLK_MAX
} SENSOR_M_CLK_E;

typedef enum {
    SENSOR_INTERFACE_TYPE_CCIR601 = 0,
    SENSOR_INTERFACE_TYPE_CCIR656,
    SENSOR_INTERFACE_TYPE_SPI,
    SENSOR_INTERFACE_TYPE_SPI_BE,
    SENSOR_INTERFACE_TYPE_CSI2,

    SENSOR_INTERFACE_TYPE_MAX
} SENSOR_INF_TYPE_E;

typedef struct sensor_reg_tab_info_tag {
    /*sensor common setting register tab*/
    SENSOR_REG_T_PTR sensor_reg_tab_ptr;
    cmr_u32 reg_count;
    /*module extend setting register tab*/
    SENSOR_REG_T_PTR sensor_ext_reg_tab_ptr;
    cmr_u32 ext_reg_count;

    cmr_u16 width;
    cmr_u16 height;
    cmr_u32 xclk_to_sensor;
    cmr_u32 image_format;

} SENSOR_REG_TAB_INFO_T, *SENSOR_REG_TAB_INFO_T_PTR;

/* SAMPLE:
type = SENSOR_INTERFACE_TYPE_CSI2; bus_width = 3;
MIPI CSI2 and Lane3
*/
typedef struct {
    SENSOR_INF_TYPE_E type;
    cmr_u32 bus_width;   /* lane number or bit-width */
    cmr_u32 pixel_width; /* bits per pixel */
    cmr_u32 is_loose;    /* 0 packet, 1 half word per pixel */
} SENSOR_INF_T;

typedef struct _sensor_rect_tag {
    cmr_u16 x;
    cmr_u16 y;
    cmr_u16 w;
    cmr_u16 h;
} SENSOR_RECT_T, *SENSOR_RECT_T_PTR;

struct hw_mipi_init_param {
    cmr_u32 lane_num;
    cmr_u32 bps_per_lane;
};
struct hw_drv_init_para {
    cmr_u32 sensor_id;
    /*sensor module handle,will be delete later*/
    cmr_handle caller_handle;
    /*add some items*/
};

struct hw_drv_cfg_param {
    cmr_u8 i2c_bus_config;
    /*add some items*/
};

/*create & delete hw_drv instance interface*/
cmr_int hw_sensor_drv_create(struct hw_drv_init_para *input_ptr,
                             cmr_handle *hw_drv_handle);
cmr_int hw_sensor_drv_delete(cmr_handle hw_handle);

cmr_int hw_sensor_drv_cfg(cmr_handle hw_handle,
                          struct hw_drv_cfg_param *input_ptr);

/*reset,mclk,power down control signal interface*/
cmr_int hw_sensor_set_reset_level(cmr_handle handle, cmr_u32 plus_level);
#define Sensor_SetResetLevel(plus_level)                                       \
    hw_sensor_set_reset_level(handle, plus_level)

cmr_int hw_sensor_reset(cmr_handle handle, cmr_u32 level, cmr_u32 pulse_width);
#define Sensor_Reset(level) hw_sensor_reset(handle, level, pulse_width)

cmr_int hw_sensor_set_mclk(cmr_handle handle, cmr_u32 mclk);
#define Sensor_SetMCLK(mclk) hw_sensor_set_mclk(handle, mclk)

cmr_int hw_sensor_power_down(cmr_handle handle, cmr_u32 power_level);
#define Sensor_PowerDown(power_level) hw_sensor_power_down(handle, power_level)

cmr_int hw_sensor_power_config(cmr_handle handle,
                               struct sensor_power_info_tag power_cfg);

/* sensor hardware power config interface */
cmr_int hw_sensor_set_voltage(cmr_handle handle, SENSOR_AVDD_VAL_E dvdd_val,
                              SENSOR_AVDD_VAL_E avdd_val,
                              SENSOR_AVDD_VAL_E iodd_val);
#define Sensor_SetVoltage(dvdd_val, avdd_val, iodd_val)                        \
    hw_sensor_set_voltage(handle, dvdd_val, avdd_val, iodd_val)

cmr_int hw_sensor_set_avdd_val(cmr_handle handle, SENSOR_AVDD_VAL_E vdd_val);
#define Sensor_SetAvddVoltage(vdd_val) hw_sensor_set_avdd_val(handle, vdd_val)

cmr_int hw_sensor_set_dvdd_val(cmr_handle handle, SENSOR_AVDD_VAL_E vdd_val);
#define Sensor_SetDvddVoltage(vdd_val) hw_sensor_set_dvdd_val(handle, vdd_val)

cmr_int hw_sensor_set_iovdd_val(cmr_handle handle, SENSOR_AVDD_VAL_E vdd_val);
#define Sensor_SetIovddVoltage(vdd_val) hw_sensor_set_iovdd_val(handle, vdd_val)

cmr_int hw_sensor_set_monitor_val(cmr_handle handle, SENSOR_AVDD_VAL_E vdd_val);
#define Sensor_SetMonitorVoltage(vdd_val)                                      \
    hw_sensor_set_monitor_val(handle, vdd_val)

/*i2c and mipi hardware interface init,deinit,control*/
cmr_int hw_sensor_mipi_init(cmr_handle handle,
                            struct hw_mipi_init_param init_param);
cmr_int hw_sensor_mipi_switch(cmr_handle hw_handle,
                              struct hw_mipi_init_param init_param);
cmr_int hw_sensor_mipi_deinit(cmr_handle handle);

cmr_int hw_sensor_set_mipi_level(cmr_handle handle, cmr_u32 plus_level);
#define Sensor_SetMIPILevel(plus_level)                                        \
    hw_sensor_set_mipi_level(handle, plus_level)

cmr_int hw_sensor_i2c_init(cmr_handle handle, cmr_u32 sensor_id);

cmr_int hw_sensor_i2c_deinit(cmr_handle handle, cmr_u32 sensor_id);

cmr_int hw_sensor_i2c_set_clk(cmr_handle handle);

cmr_int hw_sensor_i2c_set_addr(cmr_handle handle, cmr_u16 i2c_dev_addr);

cmr_int hw_sensor_get_sensorid0(cmr_handle hw_handle, cmr_u32 level);

cmr_int hw_sensor_get_sensorid1(cmr_handle hw_handle, cmr_u32 level);

cmr_int hw_sensor_get_sensorid2(cmr_handle hw_handle, cmr_u32 level);
/*i2c read and write interface*/
/*---------------------------i2c-write-interface--------------------------*/
cmr_int hw_sensor_write_data(cmr_handle handle, cmr_u8 *regPtr, cmr_u32 length);
#define Sensor_WriteData(regPtr, length)                                       \
    hw_sensor_write_data(handle, regPtr, length)
/**
 * hw_sensor_write_reg- sensor ic write register interface.
 * @handle: hardware object.
 * @reg_addr: register address to write.
 *
 * NOTE: the interface can only be used by sensor ic.
 *       not for otp and vcm device.
 **/
cmr_int hw_sensor_write_reg(cmr_handle handle, cmr_u16 subaddr, cmr_u16 data);
#define hw_Sensor_WriteReg(handle, subaddr, data)                              \
    hw_sensor_write_reg(handle, subaddr, data)
#define Sensor_WriteReg(subaddr, data) hw_Sensor_WriteReg(handle, subaddr, data)

cmr_int hw_sensor_write_reg_8bits(cmr_handle handle, cmr_u16 reg_addr,
                                  cmr_u8 value);
#define Sensor_WriteReg_8bits(reg_addr, value)                                 \
    hw_sensor_write_reg_8bits(handle, reg_addr, value)

cmr_int
hw_Sensor_SendRegTabToSensor(cmr_handle handle,
                             SENSOR_REG_TAB_INFO_T *sensor_reg_tab_info_ptr);
cmr_int hw_Sensor_Device_WriteRegTab(cmr_handle handle,
                                     SENSOR_REG_TAB_PTR reg_tab);
#define Sensor_Device_WriteRegTab(reg_tab)                                     \
    hw_Sensor_Device_WriteRegTab(handle, reg_tab)

/**
 * hw_sensor_write_i2c - vcm and otp device write register interface
 * @handle: hardware object.
 * @slave_addr: vcm and otp device address.
 * @cmd_buffer: buffer saved command and data.
 * @cmd_length: buffer length.
 *
 * NOTE: the interface can only be used by vcm and otp device,not for
 *       sensor ic.
 **/
cmr_int hw_sensor_write_i2c(cmr_handle handle, cmr_u16 slave_addr, cmr_u8 *cmd,
                            cmr_u16 cmd_length);
#define hw_Sensor_WriteI2C(handle, slave_addr, cmd, cmd_length)                \
    hw_sensor_write_i2c(handle, slave_addr, cmd, cmd_length)
#define Sensor_WriteI2C(slave_addr, cmd, cmd_length)                           \
    hw_Sensor_WriteI2C(handle, slave_addr, cmd, cmd_length)

cmr_int hw_sensor_muti_i2c_write(cmr_handle handle,
                                 struct sensor_muti_aec_i2c_tag *aec_i2c_info);

cmr_u16 hw_sensor_grc_write_i2c(cmr_handle handle, cmr_u16 slave_addr,
                                cmr_u16 addr, cmr_u16 reg, cmr_int bits);
#define sensor_grc_write_i2c(slave_addr, addr, reg, bits)                      \
    hw_sensor_grc_write_i2c(handle, slave_addr, addr, reg, bits)

/*---------------------------i2c-read-interface--------------------------*/

/**
 * hw_sensor_write_reg- sensor ic write register interface.
 * @handle: hardware object.
 * @reg_addr: register address to write.
 *
 * NOTE: the interface can only be used by sensor ic.
 *       not for otp and vcm device.
 **/
cmr_int hw_sensor_read_reg(cmr_handle handle, cmr_u16 reg_addr);
#define hw_Sensor_ReadReg(handle, reg_addr) hw_sensor_read_reg(handle, reg_addr)
#define Sensor_ReadReg(reg_addr) hw_Sensor_ReadReg(handle, reg_addr)

cmr_int hw_sensor_read_reg_8bits(cmr_handle handle, cmr_u8 reg_addr,
                                 cmr_u8 *reg_val);
#define Sensor_ReadReg_8bits(reg_addr, reg_val)                                \
    hw_sensor_read_reg_8bits(handle, reg_addr, reg_val)

cmr_u16 hw_sensor_grc_read_i2c(cmr_handle handle, cmr_u16 slave_addr,
                               cmr_u16 addr, cmr_int bits);
#define sensor_grc_read_i2c(slave_addr, addr, bits)                            \
    hw_sensor_grc_read_i2c(handle, slave_addr, addr, bits)
/**
 * hw_sensor_read_i2c - vcm and otp device read register interface
 * @handle: hardware object.
 * @slave_addr: vcm and otp device address.
 * @cmd_buffer: buffer saved command and data.
 * @cmd_length: buffer length.
 *      cmd_length(32-16 bit): the size we want to read one time.
 *      cmd_lenght(15-0 bit): the register length we read.
 *
 * NOTE: the interface can only be used by vcm and otp device,not for
 *       sensor ic.
 **/
cmr_int hw_sensor_read_i2c(cmr_handle handle, cmr_u16 slave_addr,
                           cmr_u8 *cmd_buffer, cmr_u32 cmd_length);
#define hw_Sensor_ReadI2C(handle, slave_addr, cmd, cmd_length)                 \
    hw_sensor_read_i2c(handle, slave_addr, cmd, cmd_length)

#define Sensor_ReadI2C(slave_addr, cmd, cmd_length)                            \
    hw_Sensor_ReadI2C(handle, slave_addr, cmd, cmd_length)

#define Sensor_ReadI2C_SEQ(slave_addr, cmd, reg_len, read_len)                 \
    hw_Sensor_ReadI2C(handle, slave_addr, cmd, reg_len | (read_len << 16))

cmr_int hw_sensor_get_flash_level(cmr_handle handle,
                                  struct sensor_flash_level *level);

/*expend interface,if you need to add some other function ,please add you cmd */
cmr_int hw_sensor_drv_ioctl(cmr_handle handle, int cmd, void *param);

#endif
