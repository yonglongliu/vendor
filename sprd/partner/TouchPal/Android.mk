LOCAL_PATH:= $(call my-dir)

# TouchPal input method.
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := TouchPalGlobal
LOCAL_MODULE_STEM := TouchPal.apk
LOCAL_MODULE_CLASS := APPS
LOCAL_CERTIFICATE := PRESIGNED
LOCAL_MODULE_PATH := $(TARGET_OUT)/vital-app
LOCAL_SRC_FILES := TouchPal_5.8.4.4.20170405143955_Global_OEM.aligned.apk
include $(BUILD_PREBUILT)

include $(call all-makefiles-under,$(LOCAL_PATH))
