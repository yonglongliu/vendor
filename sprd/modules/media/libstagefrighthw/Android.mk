LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES :=       \
    SprdOMXPlugin.cpp    \
    SprdOMXComponent.cpp \
    SprdSimpleOMXComponent.cpp

LOCAL_C_INCLUDES += $(GPU_GRALLOC_INCLUDES)

LOCAL_C_INCLUDES:= \
    frameworks/native/include/media/hardware \
    vendor/sprd/modules/libmemion            \
    vendor/sprd/external/kernel-headers      \
    $(LOCAL_PATH)/include                    \
    $(LOCAL_PATH)/include/openmax

LOCAL_C_INCLUDES += $(GPU_GRALLOC_INCLUDES)

LOCAL_SHARED_LIBRARIES :=       \
    libmemion                   \
    libutils                    \
    libcutils                   \
    libui                       \
    libdl                       \
    libstagefright_foundation   \
    liblog

LOCAL_MODULE := libstagefrighthw

LOCAL_CFLAGS:= -DLOG_TAG=\"$(TARGET_BOARD_PLATFORM).libstagefright\"
LOCAL_PROPRIETARY_MODULE := true

ifeq ($(USE_MEDIASDK),true)
    LOCAL_CFLAGS += -DUSE_MEDIASDK
endif

ifeq ($(USE_AVC_DECODE_COEXISTENCE),true)
    LOCAL_CFLAGS += -DUSE_AVC_DECODE_COEXISTENCE
endif

ifeq ($(USE_INTEL_MDP),true)
    LOCAL_CFLAGS += -DUSE_INTEL_MDP
endif

ifeq ($(TARGET_BUILD_VARIANT), userdebug)
    LOCAL_CPPFLAGS += -DDUMP_DEBUG
endif

include $(BUILD_SHARED_LIBRARY)

