LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_32_BIT_ONLY := true

LOCAL_SRC_FILES := gpio.cpp
LOCAL_MODULE := autotestgpio
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/engpc

LOCAL_C_INCLUDES:= \
    $(TOP)/vendor/sprd/proprietories-source/engmode

LOCAL_SHARED_LIBRARIES:= \
    libcutils \
    liblog 

include $(BUILD_SHARED_LIBRARY)
