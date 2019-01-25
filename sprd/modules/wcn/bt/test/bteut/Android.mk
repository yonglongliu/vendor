LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

BTSUITE_PATH := vendor/sprd/modules/wcn/bt/test/btsuite

ifeq ($(PLATFORM_VERSION),4.4.4)
    SPRD_BT_HAL_SRC := src/bt_hal_4_4.c
else
    SPRD_BT_HAL_SRC := src/bt_hal.c
endif

ifeq ($(PLATFORM_VERSION),4.4.4)
LOCAL_CFLAGS += -DOSI_COMPAT_ANROID_4_4_4
endif

ifeq ($(PLATFORM_VERSION),6.0)
LOCAL_CFLAGS += -DOSI_COMPAT_ANROID_6_0
endif


LOCAL_SRC_FILES := \
    src/bt_eut.c \
    src/bt_npi.c \
    src/bt_cus.c \
    src/bt_eng.c \
    $(SPRD_BT_HAL_SRC)

LOCAL_C_INCLUDES:= \
    $(TOP)/vendor/sprd/proprietories-source/engmode \
    $(LOCAL_PATH)/include \
    $(BTSUITE_PATH)/main/include

LOCAL_SHARED_LIBRARIES := \
    liblog \
    libc \
    libcutils \
    libdl

ifneq ($(SPRD_WCNBT_CHISET),)
    LOCAL_C_INCLUDES += $(BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR)
    LOCAL_CFLAGS += -DHAS_SPRD_BUILDCFG -DHAS_BDROID_BUILDCFG
else
    LOCAL_CFLAGS += -DHAS_NO_BDROID_BUILDCFG
endif

LOCAL_MODULE := libbt-sprd_eut
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_RELATIVE_PATH := npidevice
LOCAL_PROPRIETARY_MODULE := true
LOCAL_32_BIT_ONLY := true

include $(BUILD_SHARED_LIBRARY)
