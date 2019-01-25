#
# Copyright (C) 2017 spreadtrum.com
#

LOCAL_PATH:= $(call my-dir)


include $(CLEAR_VARS)

LOCAL_MODULE := libsprdimgversion

LOCAL_SRC_FILES := sprdimgversion.c

LOCAL_STATIC_LIBRARIES := \
    librpmbproxyinterface \
	libsprdimgversioninterface \

LOCAL_SHARED_LIBRARIES := \
	liblog \
	librpmbproxy \

include $(BUILD_SHARED_LIBRARY)

