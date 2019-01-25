LOCAL_PATH:= $(call my-dir)
ifeq ($(strip $(BOARD_WLAN_DEVICE)),sc2355)
include $(CLEAR_VARS)

LOCAL_32_BIT_ONLY := true

LOCAL_SRC_FILES := wifi_eut_hdlr.c

LOCAL_MODULE := libwifieut_m3
LOCAL_MODULE_TAGS := optional

LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := npidevice

LOCAL_C_INCLUDES:= \
	$(TOP)/vendor/sprd/proprietories-source/engmode

LOCAL_SHARED_LIBRARIES:= liblog libc libcutils

include $(BUILD_SHARED_LIBRARY)
endif
