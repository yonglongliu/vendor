LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:=                     \
        sprd_omx_core.cpp              \

LOCAL_C_INCLUDES += \
        vendor/sprd/modules/libmemion                     \
        $(LOCAL_PATH)/../libstagefrighthw/include         \
        $(LOCAL_PATH)/../libstagefrighthw/include/openmax

LOCAL_SHARED_LIBRARIES :=               \
        libstagefright_foundation       \
        libstagefrighthw                \
        libcutils                       \
        libutils                        \
        libdl                           \
        liblog

ifneq ($(filter $(TARGET_BOARD_PLATFORM), sp9850ka), )
LOCAL_CFLAGS += -DPLATFORM_SHARKL2
endif

LOCAL_MODULE:= libsprd_omx_core

LOCAL_PROPRIETARY_MODULE := true
include $(BUILD_SHARED_LIBRARY)

################################################################################

include $(call all-makefiles-under,$(LOCAL_PATH))
