LOCAL_PATH:= $(call my-dir)

ifeq ($(BOARD_NFC_VENDOR),samsung)
########################################
# SEC Configuration
########################################
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
        $(call all-java-files-under, src)

LOCAL_SRC_FILES += \
        $(call all-java-files-under, sec)

LOCAL_PACKAGE_NAME := NfcSec
LOCAL_CERTIFICATE := platform

LOCAL_JNI_SHARED_LIBRARIES := libnfc_sec_jni

LOCAL_PROGUARD_ENABLED := disabled

include $(BUILD_PACKAGE)

include $(call all-makefiles-under,$(LOCAL_PATH))
endif
