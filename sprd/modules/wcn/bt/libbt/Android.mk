LOCAL_PATH := $(call my-dir)

ifneq ($(SPRD_WCNBT_CHISET),)

include $(CLEAR_VARS)

BDROID_DIR := $(TOP_DIR)system/bt

LOCAL_SRC_FILES := \
        src/bt_vendor_sprd.c \
        src/hardware.c \
        src/userial_vendor.c \
        src/upio.c \
        src/conf.c \
        src/sitm.c \
        conf/sprd/$(SPRD_WCNBT_CHISET)/src/$(SPRD_WCNBT_CHISET).c \
        src/bt_vendor_sprd_hci.c \
        src/bt_vendor_sprd_a2dp.c

LOCAL_C_INCLUDES += \
        $(LOCAL_PATH)/include \
        $(BDROID_DIR)/ \
        $(BDROID_DIR)/hci/include \
        $(BDROID_DIR)/include \
        $(BDROID_DIR)/stack/btm \
        $(BDROID_DIR)/stack/include \
        $(BDROID_DIR)/gki/ulinux \
        $(LOCAL_PATH)/conf/sprd/$(SPRD_WCNBT_CHISET)/include

ifeq ($(PLATFORM_VERSION),4.4.4)
LOCAL_C_INCLUDES += \
		$(TOP_DIR)external/bluetooth/bluedroid/hci/include \
		$(TOP_DIR)external/bluetooth/bluedroid/stack/include \
		$(TOP_DIR)external/bluetooth/bluedroid/gki/ulinux/ \
		$(TOP_DIR)external/bluetooth/bluedroid/include
bdroid_CFLAGS += -DANDROID_4_4_4
LIBBT_CFLAGS += -DOSI_COMPAT_ANROID_4_4_4
endif

LOCAL_ALGO_SRC_FILES:= \
        src/bt_vendor_sprd_ssp.c \
        src/algorithms/p_256_ecc_pp.c \
        src/algorithms/p_256_curvepara.c \
        src/algorithms/p_256_multprecision.c \
        src/algorithms/lmp_ecc.c \
        src/algorithms/algo_api.c \
        src/algorithms/algo_utils.c

LOCAL_ALGO_C_INCLUDES := $(LOCAL_PATH)/src/algorithms

LOCAL_SRC_FILES += $(LOCAL_ALGO_SRC_FILES)
LOCAL_C_INCLUDES += $(LOCAL_ALGO_C_INCLUDES)


LOCAL_SHARED_LIBRARIES := \
        libcutils \
        liblog


## Special configuration ##
ifeq ($(BOARD_SPRD_WCNBT_MARLIN), true)
    ifneq ($(strip $(WCN_EXTENSION)),true)
        LIBBT_CFLAGS += -DSPRD_WCNBT_MARLIN_15A
    else
        LIBBT_CFLAGS += -DGET_MARLIN_CHIPID
    endif
endif


## Compatible with different osi
ifeq ($(PLATFORM_VERSION),6.0)
    LIBBT_CFLAGS += -DOSI_COMPAT_ANROID_6_0
endif


ifneq ($(BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR),)
  bdroid_C_INCLUDES := $(BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR)
  bdroid_CFLAGS += -DHAS_BDROID_BUILDCFG
else
  bdroid_C_INCLUDES :=
  bdroid_CFLAGS += -DHAS_NO_BDROID_BUILDCFG
endif

LOCAL_C_INCLUDES += $(bdroid_C_INCLUDES)
LOCAL_CFLAGS := $(LIBBT_CFLAGS) $(bdroid_CFLAGS)
LOCAL_MODULE := libbt-vendor
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_OWNER := sprd
LOCAL_PROPRIETARY_MODULE := true

ifeq ($(PLATFORM_VERSION),4.4.4)
include $(LOCAL_PATH)/vnd_buildcfg_4_4.mk
else
include $(LOCAL_PATH)/vnd_buildcfg.mk
endif

include $(BUILD_SHARED_LIBRARY)

endif
