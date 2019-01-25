#
# Copyright (C) 2017 spreadtrum.com
#

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := librpmbproxyinterface

LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/include/rpmbproxy

include $(BUILD_STATIC_LIBRARY)



include $(CLEAR_VARS)

LOCAL_MODULE := libsprdimgversioninterface

LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/include/sprdimgversion

include $(BUILD_STATIC_LIBRARY)
