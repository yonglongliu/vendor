LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES:= sprd_debugger.cpp

LOCAL_MODULE:= sprd_debugger

LOCAL_CPPFLAGS := -Wall

LOCAL_MODULE_TAGS:= optional

LOCAL_SHARED_LIBRARIES := libcutils liblog

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= systemDebuggerd.cpp

LOCAL_MODULE:= systemDebuggerd

LOCAL_CFLAGS := -Wall
LOCAL_MODULE_TAGS:= optional

LOCAL_SHARED_LIBRARIES := liblog libbase libcutils

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := .sprd_debugger.conf
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)
ifneq ($(TARGET_BUILD_VARIANT),user)
LOCAL_SRC_FILES := userdebug/$(LOCAL_MODULE)
else
LOCAL_SRC_FILES := user/$(LOCAL_MODULE)
endif
include $(BUILD_PREBUILT)

CUSTOM_MODULES += sprd_debugger
CUSTOM_MODULES += systemDebuggerd
CUSTOM_MODULES += .sprd_debugger.conf
