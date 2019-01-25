ifeq ($(BOARD_NFC_VENDOR),)
LOCAL_PATH:= $(call my-dir)

include $(call all-makefiles-under,$(LOCAL_PATH))
endif
