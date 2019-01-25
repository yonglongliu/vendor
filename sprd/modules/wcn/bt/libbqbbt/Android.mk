LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
BDROID_DIR := $(TOP_DIR)system/bt

LOCAL_SRC_FILES := \
        src/bqb.c \
        src/bqb_vendor.c

LOCAL_C_INCLUDES += \
        $(LOCAL_PATH)/include \
        $(BDROID_DIR)/hci/include \
        $(BDROID_DIR)/stack/include \
        $(BDROID_DIR)/include \
        $(BDROID_DIR)/btcore/include \
        $(BDROID_DIR)/gki/common \
        $(BDROID_DIR)/gki/ulinux \
        $(BDROID_DIR)/osi/include \
        $(BDROID_DIR)/stack/include \
        $(BDROID_DIR)/utils/include \
        $(BDROID_DIR)/

ifeq ($(PLATFORM_VERSION),4.4.4)
LOCAL_C_INCLUDES += \
		$(TOP_DIR)external/bluetooth/bluedroid/hci/include \
		$(TOP_DIR)external/bluetooth/bluedroid/stack/include \
		$(TOP_DIR)external/bluetooth/bluedroid/gki/ulinux/ \
		$(TOP_DIR)external/bluetooth/bluedroid/include
LOCAL_CFLAGS += -DANDROID_4_4_4
endif

LOCAL_SHARED_LIBRARIES := \
        libcutils \
        liblog \
        libdl

LOCAL_CFLAGS += -fstack-protector
LOCAL_MODULE := libbqbbt
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_OWNER := sprd
LOCAL_MULTILIB := 32
include $(BUILD_SHARED_LIBRARY)
