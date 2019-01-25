LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

include $(LOCAL_PATH)/Camera.mk
#include $(LOCAL_PATH)/Camera_Utest.mk
#include $(LOCAL_PATH)/Utest_jpeg.mk

# include $(call all-subdir-makefiles)
include $(call first-makefiles-under,$(LOCAL_PATH))
