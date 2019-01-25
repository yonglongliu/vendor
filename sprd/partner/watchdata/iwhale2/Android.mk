#only compiled when TEE configure to watchdata
ifeq ($(strip $(BOARD_TEE_CONFIG)),watchdata)
LOCAL_PATH:= $(call my-dir)
include $(call all-makefiles-under,$(LOCAL_PATH))
endif

