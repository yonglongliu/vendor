# Copyright 2005 The Android Open Source Project
#
# Android.mk for copying busybox binary
#

LOCAL_PATH:= $(call my-dir)


include $(CLEAR_VARS)

LOCAL_MODULE := busybox
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_PATH := $(PRODUCT_OUT)/root/sbin
LOCAL_MODULE_TAGS := eng debug

ifeq ($(TARGET_ARCH),arm64)
LOCAL_SRC_FILES := busybox_arm64
else ifeq ($(TARGET_ARCH),arm)
LOCAL_SRC_FILES := busybox_arm32
else ifeq ($(TARGET_ARCH),x86_64)
LOCAL_SRC_FILES := busybox_x86_64
endif

include $(BUILD_PREBUILT)
