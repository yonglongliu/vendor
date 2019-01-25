LOCAL_SHARED_LIBRARIES := libutils libmemion libcamera_client libcutils libhardware libcamera_metadata
LOCAL_SHARED_LIBRARIES += libui libbinder libdl libcamsensor libcamoem libpowermanager

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_DIR)),2.1)
LOCAL_SHARED_LIBRARIES += libcamcommon libcamdrv
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_DIR)),2.2)
LOCAL_SHARED_LIBRARIES += libcamcommon libcamdrv
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_DIR)),2.3)
LOCAL_SHARED_LIBRARIES += libcamcommon libcamdrv
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_DIR)),2.4)
LOCAL_SHARED_LIBRARIES += libcamcommon libcamdrv
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_DIR)),3)
LOCAL_SHARED_LIBRARIES += libcamcommon libcamisp
LOCAL_CFLAGS += -DCONFIG_ISP_3
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_FACE_BEAUTY)),true)
LOCAL_SHARED_LIBRARIES += libcamfb
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_FACE_DETECT)),true)
LOCAL_STATIC_LIBRARIES += libsprdfa libsprdfar
LOCAL_SHARED_LIBRARIES += libsprdfd
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_EIS)),true)
LOCAL_SHARED_LIBRARIES += libgyrostab
endif

ifeq ($(strip $(TARGET_BOARD_BOKEH_MODE_SUPPORT)),sbs)
LOCAL_SHARED_LIBRARIES += libsprddepth libsprdbokeh
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_GYRO)),true)
LOCAL_SHARED_LIBRARIES +=libgui
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_HDR_CAPTURE)),true)
LOCAL_CFLAGS += -DCONFIG_SPRD_HDR_LIB
LOCAL_SHARED_LIBRARIES += libsprd_easy_hdr
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_UV_DENOISE)),true)
LOCAL_SHARED_LIBRARIES += libuvdenoise
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_Y_DENOISE)),true)
LOCAL_SHARED_LIBRARIES += libynoise
endif


ifeq ($(strip $(TARGET_BOARD_CONFIG_CAMERA_RT_REFOCUS)),true)
LOCAL_SHARED_LIBRARIES += libalRnBLV
endif

ifeq ($(strip $(TARGET_BOARD_BLUR_MODE_SUPPORT)),true)
LOCAL_SHARED_LIBRARIES += libbokeh_gaussian libbokeh_gaussian_cap libBokeh2Frames
endif

ifeq ($(strip $(TARGET_BOARD_BOKEH_MODE_SUPPORT)),true)
LOCAL_SHARED_LIBRARIES += libsprdbokeh libsprddepth libbokeh_depth
#else ifeq ($(strip $(TARGET_BOARD_SPRD_RANGEFINDER_SUPPORT)),true)
#LOCAL_SHARED_LIBRARIES += libsprddepth libalParseOTP
endif

ifeq ($(strip $(TARGET_BOARD_ARCSOFT_BOKEH_MODE_SUPPORT)),true)
LOCAL_SHARED_LIBRARIES += libarcsoft_dualcam_refocus
LOCAL_SHARED_LIBRARIES += libalParseOTP
endif

ifeq ($(strip $(TARGET_BOARD_STEREOVIDEO_SUPPORT)),true)
LOCAL_SHARED_LIBRARIES += libimagestitcher
else ifeq ($(strip $(TARGET_BOARD_STEREOPREVIEW_SUPPORT)),true)
LOCAL_SHARED_LIBRARIES += libimagestitcher
else ifeq ($(strip $(TARGET_BOARD_STEREOCAPTURE_SUPPORT)),true)
LOCAL_SHARED_LIBRARIES += libimagestitcher
endif

ifdef CONFIG_HAS_CAMERA_HINTS_VERSION
LOCAL_CFLAGS += -DCONFIG_HAS_CAMERA_HINTS_VERSION=$(CONFIG_HAS_CAMERA_HINTS_VERSION)
else
LOCAL_CFLAGS += -DCONFIG_HAS_CAMERA_HINTS_VERSION=0
endif
