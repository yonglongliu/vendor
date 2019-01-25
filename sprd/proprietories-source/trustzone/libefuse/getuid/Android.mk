LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

#LOCAL_32_BIT_ONLY := true

LOCAL_SRC_FILES := sprd_get_uid.c

LOCAL_MODULE := libgetuid
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_RELATIVE_PATH := engpc

LOCAL_C_INCLUDES:= \
	$(TOP)/vendor/sprd/proprietories-source/engmode \
	$(LOCAL_PATH)/../

LOCAL_SHARED_LIBRARIES:= liblog libc libcutils

include $(BUILD_SHARED_LIBRARY)

