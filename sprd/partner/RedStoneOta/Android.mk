
#****************************************************
ifeq ($(strip $(REDSTONE_FOTA_SUPPORT)), true)
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := rsota
LOCAL_MODULE_TAGS := optional
ifeq ($(strip $(REDSTONE_FOTA_APK_ICON)), yes)
LOCAL_SRC_FILES := rsotaui_v6.1_icon.apk
else

LOCAL_SRC_FILES := rsotaui_v6.1_without_icon.apk

endif
LOCAL_MODULE_CLASS := APPS
LOCAL_MODULE_SUFFIX := $(COMMON_ANDROID_PACKAGES_SUFFIX)
LOCAL_CERTIFICATE:=platform
#LOCAL_MODULE_PATH:=$(LOCAL_PATH)
include $(BUILD_PREBUILT)
endif
#****************************************************

