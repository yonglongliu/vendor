LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

# resolve compile error in .h file

BTSUITE_PATH := vendor/sprd/modules/wcn/bt/test/btsuite

ifeq ($(PLATFORM_VERSION),4.4.4)
    SPRD_BT_HAL_SRC := bt_hal_4_4.c
else
    SPRD_BT_HAL_SRC := bt_hal.c
endif

LOCAL_SRC_FILES := \
    bt_eng.c \
    bt_engpc_sprd.c \
    $(SPRD_BT_HAL_SRC)

LOCAL_C_INCLUDES := \
    $(BTSUITE_PATH)/main/include

ifneq ($(SPRD_WCNBT_CHISET),)
    LOCAL_C_INCLUDES += $(BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR)
    LOCAL_CFLAGS += -DHAS_SPRD_BUILDCFG -DHAS_BDROID_BUILDCFG
else
    LOCAL_CFLAGS += -DHAS_NO_BDROID_BUILDCFG
endif

LOCAL_MODULE := libengbt
LOCAL_MODULE_OWNER := sprd
LOCAL_PROPRIETARY_MODULE := true


LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libutils \
    libhardware \
    libhardware_legacy \
    libdl \
    libbt-sprd_suite

LOCAL_REQUIRED_MODULES := libbt-sprd_suite

LOCAL_LDLIBS := -llog

include $(BUILD_SHARED_LIBRARY)
