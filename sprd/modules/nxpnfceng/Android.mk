LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

ifeq ($(NFC_VENDOR),samsung)
LOCAL_CFLAGS += -DSAMSUNG_NFC=TRUE
else ifeq ($(NFC_VENDOR),nxp)
LOCAL_CFLAGS += -DNXP_NFC=TRUE
endif

LOCAL_SRC_FILES := saveNfcCplc.c

LOCAL_MODULE := saveNfcCplc
LOCAL_MODULE_PATH := $(TARGET_OUT_EXECUTABLES)
LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)


################################################
#LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := sprd_get_nfc_cplc.c

LOCAL_MODULE := libnfccplc
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_RELATIVE_PATH := engpc

LOCAL_C_INCLUDES:= \
	$(TOP)/vendor/sprd/proprietories-source/engmode \

LOCAL_SHARED_LIBRARIES := libc libcutils

include $(BUILD_SHARED_LIBRARY)
