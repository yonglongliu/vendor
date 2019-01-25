LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    SPRDHEVCDecoder.cpp \

LOCAL_C_INCLUDES := \
    frameworks/av/media/libstagefright/include                    \
    frameworks/native/include/media/hardware                      \
    frameworks/native/include/ui                                  \
    frameworks/native/include/utils                               \
    frameworks/native/include/media/hardware                      \
    $(LOCAL_PATH)/../../../../libstagefrighthw/include         \
    $(LOCAL_PATH)/../../../../libstagefrighthw/include/openmax \
    $(LOCAL_PATH)/../../../../../libmemion                     \
    $(LOCAL_PATH)/../../../../../../external/kernel-headers

LOCAL_C_INCLUDES += $(TOP)/vendor/sprd/external/drivers/gpu

LOCAL_CFLAGS := -DOSCL_EXPORT_REF= -DOSCL_IMPORT_REF=

LOCAL_CFLAGS += -DTARGET_GPU_PLATFORM=$(TARGET_GPU_PLATFORM)

LOCAL_ARM_MODE := arm

LOCAL_SHARED_LIBRARIES := \
    libstagefright             \
    libstagefright_omx         \
    libstagefright_foundation  \
    libstagefrighthw           \
    libmedia                   \
    libutils                   \
    libui                      \
    libmemion                  \
    libdl                      \
    liblog

LOCAL_MODULE := libstagefright_sprd_h265dec
LOCAL_MODULE_TAGS := optional
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_SHARED_LIBRARY)

