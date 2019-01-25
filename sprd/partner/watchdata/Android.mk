#only compiled when TEE configure to watchdata
ifeq ($(strip $(BOARD_TEE_CONFIG)),watchdata)
LOCAL_PATH:= $(call my-dir)

MY_LOCAL_PATH := $(LOCAL_PATH)

SDIR := $(LOCAL_PATH)/../../proprietories-source/sprdtrusty/external/lk/platform/sprd
ifneq ($(shell find $(SDIR) -type d), )
	include $(LOCAL_PATH)/sprd_security_bsp/Android.mk
endif

include $(MY_LOCAL_PATH)/iwhale2/Android.mk

endif

