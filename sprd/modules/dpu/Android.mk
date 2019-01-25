LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := dpu.$(TARGET_BOARD_PLATFORM)

LOCAL_PROPRIETARY_MODULE := true

LOCAL_MODULE_RELATIVE_PATH := hw

LOCAL_MODULE_TAGS := optional

LOCAL_SHARED_LIBRARIES := liblog libutils libcutils libhardware libui libsync libdl

LOCAL_STATIC_LIBRARIES += libadf libadfhwc

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../external/kernel-headers \
                    $(LOCAL_PATH)/../libmemion \
                    $(LOCAL_PATH)/../hwcomposer \
                    $(LOCAL_PATH)/../

LOCAL_C_INCLUDES += $(GPU_GRALLOC_INCLUDES)
LOCAL_C_INCLUDES += $(TOP)/vendor/sprd/external/drivers/gpu
LOCAL_C_INCLUDES += $(TOP)/system/core/adf/libadf/include/  \
		    $(TOP)/system/core/adf/libadfhwc/include/

LOCAL_CFLAGS := -DLOG_TAG=\"DPUModule\"
LOCAL_CFLAGS += -DTARGET_GPU_PLATFORM=$(TARGET_GPU_PLATFORM)

LOCAL_SRC_FILES := DpuModule.cpp \
					DpuDevice.cpp


include $(BUILD_SHARED_LIBRARY)
