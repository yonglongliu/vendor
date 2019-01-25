
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_C_INCLUDES += \
	external/tinyalsa/include \
	frameworks/base/include

LOCAL_SHARED_LIBRARIES:= libcutils libutils libtinyalsa libaudioutils liblog

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE := libFMHalSource

LOCAL_SRC_FILES := FMHalSource.cpp RingBuffer.cpp tinyalsa_util.c

ifneq ($(filter $(strip $(PLATFORM_VERSION)),O 8.0.0 8.1.0),)
LOCAL_CFLAGS += -DFLAG_VENDOR_ETC
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_SHARED_LIBRARY)

