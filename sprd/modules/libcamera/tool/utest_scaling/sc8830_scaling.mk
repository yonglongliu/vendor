
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)


LOCAL_MODULE:= utest_scaling_$(TARGET_BOARD_PLATFORM)
LOCAL_MODULE_TAGS:= debug
LOCAL_MODULE_PATH:= $(LOCAL_PATH)
LOCAL_SHARED_LIBRARIES := liblog libEGL libbinder libutils libmemion

LOCAL_SRC_FILES:= utest_scaling.cpp
LOCAL_C_INCLUDES := $(TOP)/kernel/include/video
LOCAL_C_INCLUDES += $(TOP)/kernel/include/uapi/video
LOCAL_C_INCLUDES += $(TOP)/vendor/sprd/external/kernel-headers
LOCAL_C_INCLUDES += $(TOP)/vendor/sprd/modules/libmemion

#$(error $(LOCAL_C_INCLUDES))
include $(BUILD_EXECUTABLE)
