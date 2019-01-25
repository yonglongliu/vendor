# CopyRight sprd

ifeq ($(strip $(BOARD_FINGERPRINT_CONFIG)),microarray)

LOCAL_PATH:= $(call my-dir)
include $(call all-makefiles-under,$(LOCAL_PATH))

endif


