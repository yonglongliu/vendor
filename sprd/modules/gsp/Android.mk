LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := gsp.$(TARGET_BOARD_PLATFORM)

LOCAL_PROPRIETARY_MODULE := true

LOCAL_MODULE_RELATIVE_PATH := hw

LOCAL_MODULE_TAGS := optional

LOCAL_SHARED_LIBRARIES := liblog libutils libcutils libhardware libui libsync libdl

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../external/kernel-headers \
                    $(LOCAL_PATH)/../libmemion \
                    $(LOCAL_PATH)/../hwcomposer \
                    $(LOCAL_PATH)/../

LOCAL_C_INCLUDES += $(GPU_GRALLOC_INCLUDES)
LOCAL_C_INCLUDES += $(TOP)/vendor/sprd/external/drivers/gpu

ifeq ($(strip $(TARGET_GPU_PLATFORM)),midgard)
LOCAL_CPPFLAGS += -DGRALLOC_MIDGARD
else ifeq ($(strip $(TARGET_GPU_PLATFORM)),utgard)
LOCAL_CPPFLAGS += -DGRALLOC_UTGARD
else ifeq ($(strip $(TARGET_GPU_PLATFORM)),soft)
LOCAL_CPPFLAGS += -DGRALLOC_MIDGARD
endif

LOCAL_CFLAGS := -DLOG_TAG=\"GSP\"
LOCAL_CFLAGS += -DTARGET_GPU_PLATFORM=$(TARGET_GPU_PLATFORM)

LOCAL_SRC_FILES := GspPlane.cpp \
                   GspDevice.cpp \
                   GspModule.cpp

ifeq ($(strip $(TARGET_GPU_PLATFORM)),rogue)
LOCAL_CPPFLAGS += -DGRALLOC_ROGUE
LOCAL_SRC_FILES += GspR4P0Plane/GspR4P0Plane.cpp \
                   GspR4P0Plane/GspR4P0PlaneArray.cpp \
                   GspR4P0Plane/GspR4P0PlaneDst.cpp \
                   GspR4P0Plane/GspR4P0PlaneImage.cpp \
                   GspR4P0Plane/GspR4P0PlaneOsd.cpp
else
LOCAL_SRC_FILES += GspR1P1Plane/GspR1P1Plane.cpp \
                   GspR1P1Plane/GspR1P1PlaneArray.cpp \
                   GspR1P1Plane/GspR1P1PlaneDst.cpp \
                   GspR1P1Plane/GspR1P1PlaneImage.cpp \
                   GspR1P1Plane/GspR1P1PlaneOsd.cpp \
                   GspR3P0Plane/GspR3P0Plane.cpp \
                   GspR3P0Plane/GspR3P0PlaneArray.cpp \
                   GspR3P0Plane/GspR3P0PlaneDst.cpp \
                   GspR3P0Plane/GspR3P0PlaneImage.cpp \
                   GspR3P0Plane/GspR3P0PlaneOsd.cpp \
                   GspLiteR1P0Plane/GspLiteR1P0Plane.cpp \
                   GspLiteR1P0Plane/GspLiteR1P0PlaneArray.cpp \
                   GspLiteR1P0Plane/GspLiteR1P0PlaneDst.cpp \
                   GspLiteR1P0Plane/GspLiteR1P0PlaneImage.cpp \
                   GspLiteR1P0Plane/GspLiteR1P0PlaneOsd.cpp \
                   GspR5P0Plane/GspR5P0Plane.cpp \
                   GspR5P0Plane/GspR5P0PlaneArray.cpp \
                   GspR5P0Plane/GspR5P0PlaneDst.cpp \
                   GspR5P0Plane/GspR5P0PlaneImage.cpp \
                   GspR5P0Plane/GspR5P0PlaneOsd.cpp \
                   GspLiteR2P0Plane/GspLiteR2P0Plane.cpp \
                   GspLiteR2P0Plane/GspLiteR2P0PlaneArray.cpp \
                   GspLiteR2P0Plane/GspLiteR2P0PlaneDst.cpp \
                   GspLiteR2P0Plane/GspLiteR2P0PlaneImage.cpp \
                   GspLiteR2P0Plane/GspLiteR2P0PlaneOsd.cpp
endif

include $(BUILD_SHARED_LIBRARY)
