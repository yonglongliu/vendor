######################################
# function to find all *.cpp files under a directory
$(warning *** NFC - S.LSI ***)

#define all-cpp-files-under
#$(patsubst ./%,%, \
#    $(shell cd $(LOCAL_PATH) ; \
#    find $(1) -name "*.cpp" -and -not -name ".*") \
#    )
#endef

LOCAL_PATH:= $(call my-dir)
NFA := src/nfa
NFC := src/nfc
HAL := src/hal
UDRV := src/udrv
HALIMPL := halimpl/samsung
D_CFLAGS := -DANDROID -DANDROID_USE_LOGCAT=TRUE -DDEBUG -D_DEBUG -O0 -g \
    -DBUILDCFG=1 -DNFC_CONTROLLER_ID=1 -DNFA_EE_MAX_AID_CFG_LEN=230 \
    -DNFC_SEC_NOT_OPEN_INCLUDED=TRUE

######################################
# Build static library libnfc-nci-sec.a for DTA.
# This source code shall not be released to customer,
# so they are put in this static lib.
ifeq ($(BOARD_USES_SAMSUNG_NFC_DTA), true)
LOCAL_CFLAGS_32 += -DSEC_NFC_DTA_SUPPORT
include $(LOCAL_PATH)/libnfc-nci-sec-prebuilt.mk
endif

######################################
# Build shared library system/lib/libnfc-sec.so for stack code.

include $(CLEAR_VARS)
# LOCAL_PRELINK_MODULE := false
LOCAL_ARM_MODE := arm
LOCAL_MODULE := libnfc-sec
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := libhardware_legacy libcutils liblog libdl libhardware

########################################
# SAMSUNG DTA Configuration
ifeq ($(BOARD_USES_SAMSUNG_NFC_DTA), true)
ifeq ($(BOARD_USES_SAMSUNG_NFC_DTA_64), true)
LOCAL_WHOLE_STATIC_LIBRARIES_64 := libnfc-nci-sec-dta-64
else
LOCAL_WHOLE_STATIC_LIBRARIES_32 := libnfc-nci-sec-dta-32
endif
endif

LOCAL_CFLAGS_64 += $(D_CFLAGS)
LOCAL_CFLAGS_32 += $(D_CFLAGS)
# LOCAL_C_INCLUDES := external/stlport/stlport bionic/ bionic/libstdc++/include \

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/src/include \
    $(LOCAL_PATH)/src/gki/ulinux \
    $(LOCAL_PATH)/src/gki/common \
    $(LOCAL_PATH)/$(NFA)/include \
    $(LOCAL_PATH)/$(NFA)/int \
    $(LOCAL_PATH)/$(NFC)/include \
    $(LOCAL_PATH)/$(NFC)/int \
    $(LOCAL_PATH)/src/hal/include \
    $(LOCAL_PATH)/src/hal/int \
    $(LOCAL_PATH)/$(HALIMPL)/include

########################################
# SAMSUNG DTA Configuration
ifeq ($(BOARD_USES_SAMSUNG_NFC_DTA), true)
LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/$(NFA)/include/sec
endif

LOCAL_SRC_FILES := \
    $(call all-c-files-under, $(NFA)/ce $(NFA)/dm $(NFA)/ee) \
    $(call all-c-files-under, $(NFA)/hci $(NFA)/int $(NFA)/p2p $(NFA)/rw $(NFA)/sys) \
    $(call all-c-files-under, $(NFC)/int $(NFC)/llcp $(NFC)/nci $(NFC)/ndef $(NFC)/nfc $(NFC)/tags) \
    $(call all-c-files-under, src/adaptation) \
    $(call all-cpp-files-under, src/adaptation) \
    $(call all-c-files-under, src/gki) \
    src/nfca_version.c

include $(BUILD_SHARED_LIBRARY)


######################################
# Build shared library system/lib/hw/nfc_nci.*.so for Hardware Abstraction Layer.
# Android's generic HAL (libhardware.so) dynamically loads this shared library.

include $(CLEAR_VARS)
LOCAL_MODULE := nfc_nci.sec

LOCAL_INIT_RC := sec.rc

LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_SRC_FILES := \
    $(call all-c-files-under, $(HALIMPL)) \
    $(call all-cpp-files-under, $(HALIMPL))

LOCAL_SHARED_LIBRARIES := liblog libcutils libhardware_legacy libcrypto libssl
LOCAL_MODULE_TAGS := optional

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/$(HALIMPL)/osi \
    $(LOCAL_PATH)/$(HALIMPL)/include

LOCAL_CFLAGS := -DANDROID \
    -DBUILDCFG=1 -DNFC_SEC_NOT_OPEN_INCLUDED=TRUE -DNFC_HAL_TARGET=TRUE -DNFC_RW_ONLY=TRUE -DNFC_HAL_USE_FIRMPIN -DNFC_HAL_DO_NOT_UPDATE_BL5000

include $(BUILD_SHARED_LIBRARY)

######################################
include $(call all-makefiles-under,$(LOCAL_PATH))
