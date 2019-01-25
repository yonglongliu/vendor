LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

TARGET_BOARD_CAMERA_READOTP_METHOD?=0

PLATFORM_VERSION_FILTER = sp9850ka sc9850kh
ANDROID_MAJOR_VER := $(word 1, $(subst ., , $(PLATFORM_VERSION)))

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/vsp/inc \
	$(LOCAL_PATH)/vsp/src \
	$(LOCAL_PATH)/sensor/inc \
	$(LOCAL_PATH)/sensor \
	$(LOCAL_PATH)/jpeg/inc \
	$(LOCAL_PATH)/jpeg/src \
	$(LOCAL_PATH)/common/inc \
	$(LOCAL_PATH)/hal1.0/inc \
	$(LOCAL_PATH)/tool/auto_test/inc \
	$(LOCAL_PATH)/tool/mtrace \
	external/skia/include/images \
	external/skia/include/core\
	external/jhead \
	external/sqlite/dist \
	system/media/camera/include \
	$(TOP)vendor/sprd/modules/libmemion \
	$(TOP)/vendor/sprd/external/kernel-headers \
	$(TOP)/vendor/sprd/modules/libmemion \
	$(TARGET_OUT_INTERMEDIATES)/KERNEL/usr/include/video

ifeq (1, $(strip $(shell expr $(ANDROID_MAJOR_VER) \>= 8)))
LOCAL_C_INCLUDES += \
    $(TOP)/frameworks/native/libs/sensor/include
endif

LOCAL_C_INCLUDES += $(GPU_GRALLOC_INCLUDES)
LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_OUT_INTERMEDIATES)/KERNEL/usr

ISP_HW_VER = 3v0

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_DIR)),2.1)
TARGET_BOARD_CAMERA_ISP_3AMOD:=1  # TBD only test
ifneq ($(filter $(strip $(TARGET_BOARD_PLATFORM)),$(strip $(PLATFORM_VERSION_FILTER))),)
LOCAL_CFLAGS += -DCONFIG_CAMERA_ISP_DIR_2_1_A
ISP_HW_VER = 2v1a
else
ISP_HW_VER = 2v1
endif


ISPALG_DIR := ispalg/isp2.x
ISPDRV_DIR := camdrv/isp2.1
LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/$(ISPDRV_DIR)/isp_tune \
	$(LOCAL_PATH)/$(ISPALG_DIR)/common/inc \
	$(LOCAL_PATH)/$(ISPDRV_DIR)/middleware/inc \
	$(LOCAL_PATH)/$(ISPDRV_DIR)/driver/inc
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_DIR)),2.2)
TARGET_BOARD_CAMERA_ISP_3AMOD:=1  # TBD only test
ISP_HW_VER = 2v1
ISPALG_DIR := ispalg/isp2.x
ISPDRV_DIR := camdrv/isp2.2
LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/$(ISPDRV_DIR)/isp_tune \
	$(LOCAL_PATH)/$(ISPALG_DIR)/common/inc \
	$(LOCAL_PATH)/$(ISPDRV_DIR)/middleware/inc
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_DIR)),2.3)
TARGET_BOARD_CAMERA_ISP_3AMOD:=1  # TBD only test
ISP_HW_VER = 2v1
ISPALG_DIR := ispalg/isp2.x
ISPDRV_DIR := camdrv/isp2.3
LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/$(ISPDRV_DIR)/isp_tune \
	$(LOCAL_PATH)/$(ISPALG_DIR)/common/inc \
	$(LOCAL_PATH)/$(ISPDRV_DIR)/middleware/inc \
	$(LOCAL_PATH)/$(ISPDRV_DIR)/driver/inc
endif

#for pike2, based on isp2.1
ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_DIR)),2.4)
TARGET_BOARD_CAMERA_ISP_3AMOD:=1  # TBD only test
ISP_HW_VER = 2v4
ISPALG_DIR := ispalg/isp2.x
ISPDRV_DIR := camdrv/isp2.4
LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/$(ISPDRV_DIR)/isp_tune \
	$(LOCAL_PATH)/$(ISPALG_DIR)/common/inc \
	$(LOCAL_PATH)/$(ISPDRV_DIR)/middleware/inc \
	$(LOCAL_PATH)/$(ISPDRV_DIR)/driver/inc
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_DIR)),3)
ISP_HW_VER = 3v0
LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/isp3.0/dummy \
	$(LOCAL_PATH)/isp3.0/common/inc \
	$(LOCAL_PATH)/isp3.0/middleware/inc
endif

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/oem$(ISP_HW_VER)/inc \
    $(LOCAL_PATH)/oem$(ISP_HW_VER)/isp_calibration/inc

LOCAL_SRC_FILES+= \
	hal1.0/src/SprdCameraParameters.cpp \
	common/src/cmr_msg.c \
	tool/mtrace/mtrace.c

LOCAL_SRC_FILES += \
	tool/auto_test/src/SprdCamera_autest_Interface.cpp \
	test.cpp

ifeq ($(strip $(TARGET_CAMERA_OIS_FUNC)),true)
	LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/sensor/ois
endif

LOCAL_C_INCLUDES += \
                $(LOCAL_PATH)/arithmetic/inc \
                $(LOCAL_PATH)/arithmetic/facebeauty/inc \
                $(LOCAL_PATH)/arithmetic/sprdface/inc \
                $(LOCAL_PATH)/arithmetic/depth/inc \
                $(LOCAL_PATH)/arithmetic/bokeh/inc

ifeq ($(strip $(TARGET_BOARD_CAMERA_EIS)),true)
	LOCAL_C_INCLUDES += \
			$(LOCAL_PATH)/arithmetic/eis/inc
endif

ifeq ($(strip $(TARGET_BOARD_CONFIG_CAMERA_RT_REFOCUS)),true)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/sensor/al3200
endif


ifeq ($(TARGET_BOARD_CAMERA_HAL_VERSION), $(filter $(TARGET_BOARD_CAMERA_HAL_VERSION), HAL1.0 hal1.0 1.0))
LOCAL_SRC_FILES += hal1.0/src/SprdCameraHardwareInterface.cpp
LOCAL_SRC_FILES += hal1.0/src/SprdCameraFlash.cpp
else
LOCAL_SRC_FILES+= \
	hal3_$(ISP_HW_VER)/SprdCamera3Factory.cpp \
	hal3_$(ISP_HW_VER)/SprdCamera3Hal.cpp \
	hal3_$(ISP_HW_VER)/SprdCamera3HWI.cpp \
	hal3_$(ISP_HW_VER)/SprdCamera3Channel.cpp \
	hal3_$(ISP_HW_VER)/SprdCamera3Mem.cpp \
	hal3_$(ISP_HW_VER)/SprdCamera3OEMIf.cpp \
	hal3_$(ISP_HW_VER)/SprdCamera3Setting.cpp \
	hal3_$(ISP_HW_VER)/SprdCamera3Stream.cpp \
	hal3_$(ISP_HW_VER)/SprdCamera3Flash.cpp  \
	hal3_$(ISP_HW_VER)/SprdCameraSystemPerformance.cpp

LOCAL_SRC_FILES+= \
	hal3_$(ISP_HW_VER)/multiCamera/SprdCamera3Wrapper.cpp \
	hal3_$(ISP_HW_VER)/multiCamera/SprdCamera3MultiBase.cpp

ifneq ($(filter $(strip $(TARGET_BOARD_PLATFORM)),$(strip $(PLATFORM_VERSION_FILTER))),)
else
ifneq ($(strip $(ISP_HW_VER)),2v4)
LOCAL_SRC_FILES+= \
    hal3_$(ISP_HW_VER)/multiCamera/SprdDualCamera3Tuning.cpp
endif
endif

ifeq ($(strip $(TARGET_BOARD_RANGEFINDER_SUPPORT)),true)
LOCAL_SRC_FILES+= \
	hal3_$(ISP_HW_VER)/multiCamera/SprdCamera3RangeFinder.cpp
else ifeq ($(strip $(TARGET_BOARD_SPRD_RANGEFINDER_SUPPORT)),true)
LOCAL_SRC_FILES+= \
	hal3_$(ISP_HW_VER)/multiCamera/SprdCamera3RangeFinder.cpp
endif

ifeq ($(strip $(TARGET_BOARD_STEREOVIDEO_SUPPORT)),true)
LOCAL_SRC_FILES+= \
	hal3_$(ISP_HW_VER)/multiCamera/SprdCamera3StereoVideo.cpp
endif

ifeq ($(strip $(TARGET_BOARD_STEREOPREVIEW_SUPPORT)),true)
LOCAL_SRC_FILES+= \
	hal3_$(ISP_HW_VER)/multiCamera/SprdCamera3StereoPreview.cpp
endif

ifeq ($(strip $(TARGET_BOARD_STEREOCAPTURE_SUPPORT)),true)
LOCAL_SRC_FILES+= \
	hal3_$(ISP_HW_VER)/multiCamera/SprdCamera3Capture.cpp
endif
#    hal1.0/src/SprdCameraHardwareInterface.cpp \
#    hal1.0/src/SprdCameraFlash.cpp

ifeq ($(strip $(TARGET_BOARD_BLUR_MODE_SUPPORT)),true)
LOCAL_SRC_FILES+= \
    hal3_$(ISP_HW_VER)/multiCamera/SprdCamera3Blur.cpp
endif

ifeq ($(strip $(TARGET_BOARD_COVERED_SENSOR_SUPPORT)),true)
LOCAL_SRC_FILES+= \
    hal3_$(ISP_HW_VER)/multiCamera/SprdCamera3PageTurn.cpp  \
    hal3_$(ISP_HW_VER)/multiCamera/SprdCamera3SelfShot.cpp
endif

ifeq ($(strip $(TARGET_BOARD_BOKEH_MODE_SUPPORT)),true)
LOCAL_SRC_FILES+= \
    hal3_$(ISP_HW_VER)/multiCamera/SprdCamera3RealBokeh.cpp \
    hal3_$(ISP_HW_VER)/multiCamera/arcsoft/altek/arcsoft_calibration_parser.cpp
else ifeq ($(strip $(TARGET_BOARD_ARCSOFT_BOKEH_MODE_SUPPORT)),true)
LOCAL_SRC_FILES+= \
    hal3_$(ISP_HW_VER)/multiCamera/SprdCamera3RealBokeh.cpp \
    hal3_$(ISP_HW_VER)/multiCamera/arcsoft/altek/arcsoft_calibration_parser.cpp
endif

ifeq ($(strip $(TARGET_BOARD_BOKEH_MODE_SUPPORT)),sbs)
LOCAL_SRC_FILES+= \
    hal3_$(ISP_HW_VER)/multiCamera/SprdCamera3SidebySideCamera.cpp
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_DIR)),3)
LOCAL_SRC_FILES+= \
    hal3_$(ISP_HW_VER)/SprdCamera3AutotestMem.cpp
endif

endif

LOCAL_CFLAGS += -fno-strict-aliasing -D_VSP_ -DJPEG_ENC -D_VSP_LINUX_ -DCHIP_ENDIAN_LITTLE -Wno-unused-parameter -Werror -Wno-error=format

include $(LOCAL_PATH)/SprdCtrl.mk

include $(LOCAL_PATH)/SprdLib.mk

ifeq (1, $(strip $(shell expr $(ANDROID_MAJOR_VER) \>= 8)))
LOCAL_SHARED_LIBRARIES += liblog libsensorndkbridge
LOCAL_CFLAGS += -DCONFIG_SPRD_ANDROID_8
endif

ifeq ($(strip $(CONFIG_HAS_CAMERA_HINTS_VERSION)),801)
LOCAL_SHARED_LIBRARIES += libhidlbase libhidltransport libutils vendor.sprd.hardware.power@2.0_vendor
endif

LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_MODULE := camera.$(TARGET_BOARD_PLATFORM)

LOCAL_MODULE_TAGS := optional

ifeq (1, $(strip $(shell expr $(ANDROID_MAJOR_VER) \>= 8)))
LOCAL_PROPRIETARY_MODULE := true
endif

ifeq ($(strip $(TARGET_BOARD_PLATFORM)),sp9853i)
LOCAL_CFLAGS += -DCONFIG_CAMERA_MAX_PREVSIZE_1080P
endif

ifeq ($(PLATFORM_VERSION),4.4.4)
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
endif

include $(BUILD_SHARED_LIBRARY)
