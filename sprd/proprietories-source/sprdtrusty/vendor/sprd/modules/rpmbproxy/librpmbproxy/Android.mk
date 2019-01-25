#
# Copyright (C) 2017 spreadtrum.com
#

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := librpmbproxy

LOCAL_SRC_FILES := \
        rpmbproxy.c \
        rpmb_ops.c

LOCAL_CLFAGS = -fvisibility=hidden -Wall -Werror

LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/include
LOCAL_C_INCLUDES += bionic/libc/kernel/uapi

LOCAL_STATIC_LIBRARIES := \
    libtrusty             \
    librpmbproxyinterface \


LOCAL_SHARED_LIBRARIES := \
    liblog \
    libcrypto

include $(BUILD_SHARED_LIBRARY)
#include $(BUILD_STATIC_LIBRARY)
