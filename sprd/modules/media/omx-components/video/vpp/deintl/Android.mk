LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    component/SPRDDeinterlace.cpp

ifeq ($(strip $(TARGET_VSP_PLATFORM)),whale)
LOCAL_SRC_FILES += core/vpp_deint_api.c            \
                   core/vpp_drv_interface.c

LOCAL_C_INCLUDES := $(LOCAL_PATH)/core
else ifeq ($(strip $(TARGET_VSP_PLATFORM)),iwhale2)
LOCAL_SRC_FILES += core/vpp_deint_api.c            \
                   core/vpp_drv_interface.c

LOCAL_C_INCLUDES := $(LOCAL_PATH)/core
else
LOCAL_SRC_FILES += core/vsp/vsp_deint_api.c        \
                   core/vsp/osal_log.c             \
                   core/vsp/vsp_drv.c

LOCAL_C_INCLUDES := $(LOCAL_PATH)/core/vsp
endif

LOCAL_C_INCLUDES += $(TOP)/vendor/sprd/external/drivers/gpu \
    $(LOCAL_PATH)/../../../../libstagefrighthw/include         \
    $(LOCAL_PATH)/../../../../libstagefrighthw/include/openmax \
    $(LOCAL_PATH)/../../../../../libmemion                     \
    $(LOCAL_PATH)/../../../../../../external/kernel-headers    \
    $(LOCAL_PATH)/../../avc/dec/hw                             \
    $(LOCAL_PATH)/component                                    \
    $(LOCAL_PATH)/core                                         \
    frameworks/native/include/ui

LOCAL_C_INCLUDES += $(TOP)/vendor/sprd/external/drivers/gpu

LOCAL_CFLAGS := -DOSCL_EXPORT_REF= -DOSCL_IMPORT_REF=

LOCAL_CFLAGS += -DTARGET_GPU_PLATFORM=$(TARGET_GPU_PLATFORM)

LOCAL_ARM_MODE := arm

LOCAL_SHARED_LIBRARIES := \
    libstagefrighthw           \
    libutils                   \
    libui                      \
    libmemion                  \
    liblog                     \
    libmedia

LOCAL_MODULE := libstagefright_sprd_deintl
LOCAL_MODULE_TAGS := optional
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_SHARED_LIBRARY)

