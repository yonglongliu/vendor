ifeq ($(BOARD_TEE_CONFIG), trusty)
LOCAL_PATH:= $(call my-dir)

include $(call all-makefiles-under,$(LOCAL_PATH))
endif
