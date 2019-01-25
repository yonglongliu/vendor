#ifndef _SENSOR_IC_DRV_
#define _SENSOR_IC_DRV_

#include "../hw_drv/hw_sensor_drv.h"

#define SENSOR_IC_SUCCESS CMR_CAMERA_SUCCESS
#define SENSOR_IC_FAILED CMR_CAMERA_FAIL

#define SENSOR_IC_CHECK_HANDLE(handle)                                         \
    if (NULL == handle) {                                                      \
        SENSOR_LOGE("Handle is invalid " #handle);                             \
        return SENSOR_IC_FAILED;                                               \
    }
/*no reture value*/
#define SENSOR_IC_CHECK_HANDLE_VOID(handle)                                    \
    if (NULL == handle) {                                                      \
        SENSOR_LOGE("Handle is invalid " #handle);                             \
        return;                                                                \
    }

#define SENSOR_IC_CHECK_PTR(expr)                                              \
    if ((expr) == NULL) {                                                      \
        SENSOR_LOGE("ERROR: NULL pointer detected " #expr);                    \
        return SENSOR_IC_FAILED;                                               \
    }

/*no reture value*/
#define SENSOR_IC_CHECK_PTR_VOID(expr)                                         \
    if ((expr) == NULL) {                                                      \
        SENSOR_LOGE("ERROR: NULL pointer detected " #expr);                    \
        return;                                                                \
    }

#define SENSOR_IDENTIFY_CODE_COUNT 0x02

#ifndef PNULL
#define PNULL ((void *)0)
#endif

typedef enum {
    SENSOR_IMAGE_FORMAT_YUV422 = 0,
    SENSOR_IMAGE_FORMAT_YUV420,
    SENSOR_IMAGE_FORMAT_YVU420,
    SENSOR_IMAGE_FORMAT_YUV420_3PLANE,
    SENSOR_IMAGE_FORMAT_RAW,
    SENSOR_IMAGE_FORMAT_RGB565,
    SENSOR_IMAGE_FORMAT_RGB666,
    SENSOR_IMAGE_FORMAT_RGB888,
    SENSOR_IMAGE_FORMAT_JPEG,
    SENSOR_IMAGE_FORMAT_MAX
} SENSOR_IMAGE_FORMAT;

/*sensor access_value cmd*/
typedef enum {
    SENSOR_IOCTL_RESET = 0,
    SENSOR_IOCTL_POWER,
    SENSOR_IOCTL_ENTER_SLEEP,
    SENSOR_IOCTL_IDENTIFY,
    SENSOR_IOCTL_WRITE_REG,
    SENSOR_IOCTL_READ_REG,
    SENSOR_IOCTL_CUS_FUNC_1,
    SENSOR_IOCTL_CUS_FUNC_2,
    SENSOR_IOCTL_AE_ENABLE,
    SENSOR_IOCTL_HMIRROR_ENABLE,
    SENSOR_IOCTL_VMIRROR_ENABLE,
    SENSOR_IOCTL_BRIGHTNESS,
    SENSOR_IOCTL_CONTRAST,
    SENSOR_IOCTL_SHARPNESS,
    SENSOR_IOCTL_SATURATION,
    SENSOR_IOCTL_PREVIEWMODE,
    SENSOR_IOCTL_IMAGE_EFFECT,
    SENSOR_IOCTL_BEFORE_SNAPSHOT,
    SENSOR_IOCTL_AFTER_SNAPSHOT,
    SENSOR_IOCTL_FLASH,
    SENSOR_IOCTL_READ_EV,
    SENSOR_IOCTL_WRITE_EV,
    SENSOR_IOCTL_READ_GAIN,
    SENSOR_IOCTL_WRITE_GAIN,
    SENSOR_IOCTL_READ_GAIN_SCALE,
    SENSOR_IOCTL_SET_FRAME_RATE,
    SENSOR_IOCTL_AF_ENABLE,
    SENSOR_IOCTL_AF_GET_STATUS,
    SENSOR_IOCTL_SET_WB_MODE,
    SENSOR_IOCTL_GET_SKIP_FRAME,
    SENSOR_IOCTL_ISO,
    SENSOR_IOCTL_EXPOSURE_COMPENSATION,
    SENSOR_IOCTL_CHECK_IMAGE_FORMAT_SUPPORT,
    SENSOR_IOCTL_CHANGE_IMAGE_FORMAT,
    SENSOR_IOCTL_ZOOM,
    SENSOR_IOCTL_CUS_FUNC_3,
    SENSOR_IOCTL_FOCUS,
    SENSOR_IOCTL_EXT_FUNC,
    SENSOR_IOCTL_ANTI_BANDING_FLICKER,
    SENSOR_IOCTL_VIDEO_MODE,
    SENSOR_IOCTL_PICK_JPEG_STREAM,
    SENSOR_IOCTL_SET_METER_MODE,
    SENSOR_IOCTL_GET_STATUS,
    SENSOR_IOCTL_STREAM_ON,
    SENSOR_IOCTL_STREAM_OFF,
    SENSOR_IOCTL_ACCESS_VAL,
    /*add you sub comman here*/

    SENSOR_IOCTL_MAX
} SENSOR_IOCTL_CMD_E;

/*sensor access_value cmd*/
typedef enum {
    SENSOR_VAL_TYPE_SHUTTER = 0,
#if 1
    /*TODO:to be deleted*/
    SENSOR_VAL_TYPE_READ_VCM,
    SENSOR_VAL_TYPE_WRITE_VCM,
    SENSOR_VAL_TYPE_WRITE_OTP,
    SENSOR_VAL_TYPE_READ_OTP,
    SENSOR_VAL_TYPE_READ_DUAL_OTP,
    SENSOR_VAL_TYPE_ERASE_OTP,
    SENSOR_VAL_TYPE_PARSE_OTP,
    SENSOR_VAL_TYPE_PARSE_DUAL_OTP,
    SENSOR_VAL_TYPE_GET_RELOADINFO,
    SENSOR_VAL_TYPE_GET_AFPOSITION,
    SENSOR_VAL_TYPE_WRITE_OTP_GAIN,
    SENSOR_VAL_TYPE_READ_OTP_GAIN,
    SENSOR_VAL_TYPE_GET_GOLDEN_DATA,
    SENSOR_VAL_TYPE_GET_GOLDEN_LSC_DATA,
    SENSOR_VAL_TYPE_INIT_OTP,
#endif
    SENSOR_VAL_TYPE_GET_STATIC_INFO,
    SENSOR_VAL_TYPE_GET_FPS_INFO,
    SENSOR_VAL_TYPE_GET_PDAF_INFO,
    SENSOR_VAL_TYPE_SET_SENSOR_CLOSE_FLAG,
    SENSOR_VAL_TYPE_GET_BV,
    SENSOR_VAL_TYPE_SET_SENSOR_MULTI_MODE,
    SENSOR_VAL_TYPE_SET_RAW_INFOR,
    SENSOR_VAL_TYPE_MAX
} SENSOR_IOCTL_VAL_TYPE;

/**
 * Get sensor privage data
 **/
typedef enum {
    SENSOR_CMD_GET_TRIM_TAB,
    SENSOR_CMD_GET_RESOLUTION,
    SENSOR_CMD_GET_MODULE_CFG,
    SENSOR_CMD_GET_EXIF,

    SENSOR_CMD_GET_MAX,
} sensor_get_private_data;

/** If the camera module vendor name is not
 *  in the list above, please add to the following.
 *  if you have added a vendor name,don't forget
 *  add module name at array {@camera_module_name_str}
 *  following.
 **/
enum camera_module_id {
    MODULE_DEFAULT = 0x00, /*NULL*/
    MODULE_SUNNY = 0x01,
    MODULE_TRULY,
    MODULE_RTECH,
    MODULE_QTECH,
    MODULE_ALTEK, /*5*/
    MODULE_CMK,
    MODULE_SHINE,
    MODULE_DARLING,
    MODULE_BROAD,
    MODULE_DMEGC, /*10*/
    MODULE_SEASONS,
    MODULE_SUNWIN,
    MODULE_OFLIM,
    MODULE_HONGSHI,

    MODULE_SUNNINESS, /*15*/
    MODULE_RIYONG,
    MODULE_TONGJU,
    /*add camera vendor name index here*/
    MODULE_A_KERR,
    MODULE_LITEARRAY,
    MODULE_HUAQUAN,
    MODULE_KINGCOM,
    MODULE_BOOYI,
    MODULE_LAIMU,
    MODULE_WDSEN,
    MODULE_SUNRISE,

    MODULE_MAX, /*NOTE:This must be the last line*/
};

struct hdr_info_t {
    cmr_u32 capture_max_shutter;
    cmr_u32 capture_shutter;
    cmr_u32 capture_gain;
};

struct sensor_ev_info_t {
    cmr_u16 preview_shutter;
    float preview_gain;
    cmr_u16 preview_framelength;
};

typedef struct sensor_extend_info_tag {
    cmr_u32 focus_mode;
    cmr_u32 exposure_mode;
} SENSOR_EXTEND_INFO_T, *SENSOR_EXTEND_INFO_T_PTR;

struct module_fov_info {
    /**
     * Cmos width and height.You can get this information from sensor datasheet
     * [1]:width.[2]:height
     **/
    float physical_size[2];

    /**
     * Sensor focal length.You should Confirm the information from the
     * module vendor.
     **/
    float focal_lengths; /*focal*/
};

struct sensor_static_info {
    /* f-number,focal ratio,actual f-number*100 */
    cmr_u32 f_num;

    /* focal_length */
    cmr_u32 focal_length; // actual focal_length*100

    /* max fps of sensor's all settings */
    cmr_u32 max_fps;

    /* max_adgain,AD-gain */
    cmr_u32 max_adgain;

    /* Whether support ois,1:support,0:none*/
    cmr_u32 ois_supported;

    /* Whether support pdaf,1:support,0:none*/
    cmr_u32 pdaf_supported;

    /* exp_valid_frame_num;N+2-1 */
    cmr_u32 exp_valid_frame_num;

    /* black level */
    cmr_u32 clamp_level;

    /*adgain_valid_frame_num;N+1-1*/
    cmr_u32 adgain_valid_frame_num;

    /*module fov information*/
    struct module_fov_info fov_info;
};

typedef struct sensor_static_info_tab {
    cmr_u16 module_id;
    struct sensor_static_info static_info;
} SENSOR_STATIC_INFO_T;

typedef struct sensor_mode_fps_tag {
    enum sensor_mode mode;
    cmr_u32 max_fps;           // max fps in current sensor
                               // mode,10*1000000/(line_time*frame_line)
    cmr_u32 min_fps;           // min fps, we set it to 1.
    cmr_u32 is_high_fps;       // if max_fps > 30,then is high fps.
    cmr_u32 high_fps_skip_num; // max_fps/30
} SENSOR_MODE_FPS_T, *SENSOR_MODE_FPS_T_PTR;

struct sensor_fps_info {
    cmr_u32 is_init;
    SENSOR_MODE_FPS_T sensor_mode_fps[SENSOR_MODE_MAX];
};

typedef struct {
    /**
     * module id is very important,
     * you need to confirm this info
     * again and again
     */
    cmr_u16 module_id;
    struct sensor_fps_info fps_info;
} SENSOR_MODE_FPS_INFO_T;

struct sensor_ext_reg_tab {
    SENSOR_REG_T_PTR sensor_reg_tab_ptr;
    cmr_u32 reg_count;
};

struct sensor_res_tab_info {
    cmr_u16 module_id;
    SENSOR_REG_TAB_INFO_T reg_tab[SENSOR_MODE_MAX];
};

struct sensor_trim_tag {
    cmr_u16 trim_start_x;
    cmr_u16 trim_start_y;
    cmr_u16 trim_width;
    cmr_u16 trim_height;
    cmr_u32 line_time;
    cmr_u32 bps_per_lane; /* bps_per_lane = 2 * mipi_clock */
    cmr_u32 frame_line;
    SENSOR_RECT_T scaler_trim;
};

typedef struct sensor_trim_info {
    cmr_u16 module_id;
    struct sensor_trim_tag trim_info[SENSOR_MODE_MAX];
} SENSOR_TRIM_T, *SENSOR_TRIM_T_PTR;

struct sensor_ic_ioctl_func_tag {
    cmr_uint ioctl_type;
    /*param can be a value or a pointer*/
    cmr_int (*ops)(cmr_handle handle, cmr_uint param);
};

typedef struct sensor_ae_info_tag {
    /*min frame rate*/
    cmr_u32 min_frate;

    /*max frame rate*/
    cmr_u32 max_frate;

    /*line exposure time*/
    cmr_u32 line_time;
    cmr_u32 gain;
} SENSOR_AE_INFO_T, *SENSOR_AE_INFO_T_PTR;

typedef struct sensor_video_info_tag {
    SENSOR_AE_INFO_T ae_info[SENSOR_VIDEO_MODE_MAX];
    SENSOR_REG_T **setting_ptr;
} SENSOR_VIDEO_INFO_T, *SENSOR_VIDEO_INFO_T_PTR;

/**
 *  callbcak from sensor_drv_u.if you
 *  need to add additional interfaces.
 *  Please note the format.
 **/
struct sensor_ic_ctrl_cb {
    cmr_int (*set_exif_info)(cmr_handle sns_module_handle, cmr_u32 cmd,
                             cmr_u32 param);
    cmr_int (*get_exif_info)(cmr_handle sns_module_handle, void **param);
    cmr_int (*set_mode)(cmr_handle sns_module_handle, cmr_u32 mode);
    cmr_int (*set_mode_wait_done)(cmr_handle sns_module_handle);
    cmr_int (*get_mode)(cmr_handle sns_module_handle, cmr_u32 *mode);
    /*add ops here,if you need*/
};

struct sensor_ic_drv_init_para {
    /*hardware instance to communicate with kernel*/
    cmr_handle hw_handle;

    /*callback handle*/
    cmr_handle caller_handle;
    cmr_u8 sensor_id;

    /*current module id,get from sensor_cfg.c*/
    cmr_u16 module_id;

    /*callback from sensor_drv_u.c*/
    struct sensor_ic_ctrl_cb ops_cb;
    /*you can add your param here*/
};

struct sensor_ic_exif_info {
    cmr_u16 exposure_line;
    cmr_u32 exposure_time;
    cmr_u32 aperture_value;
    cmr_u32 max_aperture_value;
    cmr_u32 numerator;
};

/*
 * The following is sensor ic control interface,you can not
 * add the interface arbitrarily. If you want to add an interface,
 * you can add your function at ioctl item as described next.
 *
 * 1.add you sub command at above SENSOR_IOCTL_CMD_E enumeration
 * 2.Implement your control function at sensor driver(mipi_raw.c).
 * 3.add you control function at ioctl table.
 *
 * NOTE: you can't add interface arbitrarily in this structure.
 **/
struct sensor_ic_ops {
    /*create a sensor ic instance */
    cmr_int (*create_handle)(struct sensor_ic_drv_init_para *init_param,
                             cmr_handle *sns_ic_drv_handle);

    /*delete sensor ic instance.you can release buffer etc*/
    cmr_int (*delete_handle)(cmr_handle handle, void *param);

    /*
     *  you can get private data of current sensor instance
     *  with the interface.
     *  for example:
     *       1.exif info of current sensor instance.
     *       2.resolution setting table.
     *       3.module cfg info,such as i2_dev_addr,bayer_patter,etc
     */
    cmr_int (*get_data)(cmr_handle handle, cmr_uint cmd, void **param);

    /**
     *  Some common control interfaces
     */
    cmr_int (*power)(cmr_handle handle, cmr_uint param);
    cmr_int (*identify)(cmr_handle handle, cmr_uint param);
    cmr_int (*write_exp)(cmr_handle handle, cmr_uint param);
    cmr_int (*ex_write_exp)(cmr_handle handle, cmr_uint param);
    cmr_int (*write_gain_value)(cmr_handle handle, cmr_uint param);
    cmr_int (*write_ae_value)(cmr_handle handle, cmr_uint param);
    cmr_int (*read_aec_info)(cmr_handle handle, void *param);

    /*expend ops */
    struct sensor_ic_ioctl_func_tag ext_ops[SENSOR_IOCTL_MAX];

    /* not used currently*/
    cmr_int (*ioctl)(cmr_handle handle, int cmd, void *param);
};

/**
 * current module config info
 **/
struct module_cfg_info {
    /* salve i2c write address */
    // cmr_u8 salve_i2c_addr_w;

    /* salve i2c read address */
    // cmr_u8 salve_i2c_addr_r;

    // cmr_u8 i2c_addr[2];

    /**
     * Different modules may have different addresses with same sensor ic,
     * you should configure i2c device address here.If the field is not 0x00
     * it is a valid i2c device address.
     **/
    cmr_u8 major_i2c_addr;
    cmr_u8 minor_i2c_addr;

    cmr_u8 reg_addr_value_bits;

    /* voltage of avdd */
    SENSOR_AVDD_VAL_E avdd_val;

    /* voltage of iovdd */
    SENSOR_AVDD_VAL_E iovdd_val;

    /* voltage of dvdd */
    SENSOR_AVDD_VAL_E dvdd_val;

    /*  pattern of input image form sensor */
    cmr_u32 image_pattern;

    /* skip frame num before preview */
    cmr_u32 preview_skip_num;

    /* skip frame num before capture */
    cmr_u32 capture_skip_num;

    /* deci frame num during preview */
    cmr_u32 preview_deci_num;

    /* skip frame num for flash capture */
    cmr_u32 flash_capture_skip_num;

    /* skip frame num on mipi cap */
    cmr_u32 mipi_cap_skip_num;

    /* deci frame num during video preview */
    cmr_u32 video_preview_deci_num;

    cmr_u16 threshold_eb;
    cmr_u16 threshold_mode;
    cmr_u16 threshold_start;
    cmr_u16 threshold_end;
    cmr_u8 i2c_dev_handler;

    /*camera mipi setting info,you must config the info*/
    SENSOR_INF_T sensor_interface;

    /* skip frame num while change setting */
    cmr_u32 change_setting_skip_num;

    /* horizontal  view angle*/
    cmr_u16 horizontal_view_angle;

    /* vertical view angle*/
    cmr_u16 vertical_view_angle;
};

struct sensor_module_info {
    cmr_u16 module_id;
    struct module_cfg_info module_info;
};

/*sensor ic common configure information*/
typedef struct sensor_info_tag {
    /* bit2: 0:negative; 1:positive -> polarily of horizontal synchronization
     * signal
     * bit4: 0:negative; 1:positive -> polarily of vertical synchronization
     * signal
     * other bit: reseved
     */
    cmr_u8 hw_signal_polarity;

    /* preview mode */
    cmr_u32 environment_mode;

    /* image effect */
    cmr_u32 image_effect;

    /* while balance mode */
    cmr_u32 wb_mode;

    /* bit[0:7]: count of step in brightness, contrast, sharpness, saturation
     * bit[8:31] reseved
     */
    cmr_u32 step_count;

    /* reset pulse level */
    cmr_u16 reset_pulse_level;

    /* reset pulse width(ms) */
    cmr_u16 reset_pulse_width;

    /* 1: high level valid; 0: low level valid */
    cmr_u32 power_down_level;

    /* count of identify code */
    cmr_u32 identify_count;

    /* supply two code to identify sensor.
     * for Example: index = 0-> Device id, index = 1 -> version id
     * customer could ignore it.
     */
    SENSOR_REG_T identify_code[SENSOR_IDENTIFY_CODE_COUNT];

    /* max width of source image */
    cmr_u16 source_width_max;

    /* max height of source image */
    cmr_u16 source_height_max;

    /* name of sensor */
    const cmr_s8 *name;

    /* define in SENSOR_IMAGE_FORMAT_E enum,SENSOR_IMAGE_FORMAT_MAX
     * if set to SENSOR_IMAGE_FORMAT_MAX here,
     * image format depent on SENSOR_REG_TAB_INFO_T
     */
    SENSOR_IMAGE_FORMAT image_format;

    /* point to resolution table information structure */
    // SENSOR_REG_TAB_INFO_T_PTR resolution_tab_info_ptr;
    struct sensor_res_tab_info *resolution_tab_info_ptr;

    /* point to ioctl function table */
    struct sensor_ic_ops *sns_ops;

    /* information and table about Rawrgb sensor */
    struct sensor_raw_info **raw_info_ptr;

    SENSOR_VIDEO_INFO_T_PTR video_tab_info_ptr;

    /* extend information about sensor */
    SENSOR_EXTEND_INFO_T_PTR ext_info_ptr;

    struct sensor_module_info *module_info_tab;
    cmr_u32 module_info_tab_size;
    /*sensor version info that will be used in isp module.*/
    const cmr_s8 *sensor_version_info;

    /*sensor foucus enable info*/
    cmr_u8 focus_eb;
} SENSOR_INFO_T;

struct sensor_ic_ad_gain {
    cmr_u16 a_gain;
    cmr_u16 d_gain;
};

struct sensor_ic_drv_cxt {
    cmr_u8 sensor_id;
    /*hardware handle*/
    cmr_handle hw_handle;

    /*module context*/
    cmr_handle caller_handle;
    cmr_u8 is_sensor_close;

    /*multi camera mode flag*/
    cmr_int is_multi_mode;
    /**
     * The flag indicates fps information of all module
     * is initialized or not.
     * 1:initialized,0:not init
     **/
    cmr_u8 is_fps_info_init;

    /**
     * The field is module name index,you can get more
     * information in
     * to load different static infomation
     **/
    cmr_u16 module_id;

    /*current frame length*/
    cmr_u32 frame_length;

    /*default frame length*/
    cmr_u32 frame_length_def;

    /*default frame length*/
    cmr_u32 line_time_def;

    /*current exposure line*/
    cmr_u16 exp_line;

    /*current exposure time*/
    cmr_u32 exp_time;

    union {
        struct sensor_ic_ad_gain ad_gain;
        cmr_u32 gain;
    } sensor_gain;

    struct sensor_ic_exif_info exif_info;

    /**
     *  TODO:Some sensor drivers has exif data of type
     *  EXIF_SPEC_PIC_TAKING_COND_T.So in order to compatible
     *  with this situation,we defined a temporary pointer to
     *  point exif data type of EXIF_SPEC_PIC_TAKING_COND_T.
     *
     *  if you need to write exif info,please write to above
     *  exif_info field.
     *
     *  this pointer point static variable,don't need free.
     **/
    void *exif_ptr;
    cmr_u8 exif_malloc;

    struct hdr_info_t hdr_info;
    struct sensor_ev_info_t sensor_ev_info;

    /*current runtime state*/
    struct sensor_static_info *static_info;
    struct sensor_fps_info *fps_info;
    struct sensor_reg_tab_info_tag *res_tab_info;
    struct sensor_trim_tag *trim_tab_info;
    struct module_cfg_info *module_info;

    struct sensor_ic_ctrl_cb ops_cb;

    /*if private data size is greater than 4 bytes
      you should mallc new buffer.*/
    union {
        cmr_u8 *buffer;
        cmr_u8 data[4];
        void *priv_handle;
    } privata_data;
};

cmr_int sensor_ic_drv_create(struct sensor_ic_drv_init_para *init_param,
                             cmr_handle *sns_ic_drv_handle);

cmr_int sensor_ic_drv_delete(cmr_handle handle, void *param);

cmr_int sensor_ic_get_private_data(cmr_handle handle, cmr_uint cmd,
                                   void **param);

cmr_int sensor_ic_get_init_exif_info(cmr_handle handle, void **exif_info_in);

cmr_int sensor_ic_set_match_static_info(cmr_handle handle, cmr_u16 vendor_num,
                                        void *static_info_ptr);
cmr_int sensor_ic_set_match_fps_info(cmr_handle handle, cmr_u16 vendor_num,
                                     void *fps_info_ptr);
cmr_int sensor_ic_set_match_resolution_info(cmr_handle handle,
                                            cmr_u16 vendor_num, void *param);

cmr_int sensor_ic_set_match_trim_info(cmr_handle handle, cmr_u16 vendor_num,
                                      void *param);

cmr_int sensor_ic_set_match_module_info(cmr_handle handle, cmr_u16 vendor_num,
                                        void *module_info_ptr);

void sensor_ic_print_static_info(cmr_s8 *log_tag,
                                 struct sensor_ex_info *ex_info);
#endif
