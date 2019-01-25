$(warning *** NFC - S.LSI ***)
VOB_COMPONENTS := vendor/sprd/partner/samsung/external/libnfc-nci/src
NFA := $(VOB_COMPONENTS)/nfa
NFC := $(VOB_COMPONENTS)/nfc

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
# LOCAL_PRELINK_MODULE := false
include $(call all-makefiles-under,$(LOCAL_PATH))
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk

ifneq ($(NCI_VERSION),)
LOCAL_CFLAGS += -DNCI_VERSION=$(NCI_VERSION) -O0 -g
endif

LOCAL_CFLAGS += -Wall -Wextra
LOCAL_CFLAGS += -DNFC_SEC_NOT_OPEN_INCLUDED=TRUE

########################################
# SAMSUNG DTA Configuration
ifeq ($(BOARD_USES_SAMSUNG_NFC_DTA), true)
        LOCAL_CFLAGS_64 := -DSEC_NFC_DTA_SUPPORT
endif

define all-cpp-files-under
$(patsubst ./%,%, \
  $(shell cd $(LOCAL_PATH) ; \
          find $(1) -name "*.cpp" -and -not -name ".*") \
 )
endef

LOCAL_SRC_FILES += $(call all-cpp-files-under, .) $(call all-c-files-under, .)

LOCAL_C_INCLUDES += \
    external/libxml2/include \
    frameworks/native/include \
    libcore/include \
    $(NFA)/include \
    $(NFA)/int \
    $(NFA)/include/sec \
    $(NFC)/include \
    $(NFC)/int \
    $(VOB_COMPONENTS)/hal/include \
    $(VOB_COMPONENTS)/hal/int \
    $(VOB_COMPONENTS)/include \
    $(VOB_COMPONENTS)/gki/ulinux \
    $(VOB_COMPONENTS)/gki/common

########################################
# SAMSUNG DTA Configuration
ifeq ($(BOARD_USES_SAMSUNG_NFC_DTA), true)
LOCAL_C_INCLUDES += \
    $(NFA)/include/sec
endif

LOCAL_SHARED_LIBRARIES := \
    libicuuc \
    libnativehelper \
    libcutils \
    libutils \
    liblog \
    libnfc-sec \

LOCAL_STATIC_LIBRARIES := libxml2

LOCAL_MODULE := libnfc_sec_jni

include $(BUILD_SHARED_LIBRARY)

