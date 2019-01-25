LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

ifeq ($(SHARKLJ1_BBAT_BIT64), true)
LOCAL_32_BIT_ONLY := false
else
LOCAL_32_BIT_ONLY := true
endif

#SHARKL2 BBAT
ifeq ($(SHARKL2_BBAT_GPIO), true)
LOCAL_CFLAGS += -DSHARKL2_BBAT_FINGER_SENSOR
endif

ifeq ($(iSHARKL2_BBAT_GPIO), true)
LOCAL_CFLAGS += -DSHARKL2_BBAT_FINGER_SENSOR
endif

ifeq ($(IWHALE2_BBAT_GPIO), true)
LOCAL_CFLAGS += -DSHARKL2_BBAT_FINGER_SENSOR
endif


#WHALE2 BBAT
ifeq ($(WHALE2_BBAT_GPIO), true)
LOCAL_CFLAGS += -DWHALE2_BBAT_FINGER_SENSOR
endif

LOCAL_SRC_FILES := finger_sensor.cpp \
			nfc_test.cpp

LOCAL_MODULE := libnfctest
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_RELATIVE_PATH := autotest

LOCAL_C_INCLUDES:= \
     $(TOP)/vendor/sprd/proprietories-source/autotest

LOCAL_SHARED_LIBRARIES:= \
    libcutils  \
	    libhardware  \
    libhardware_legacy 

include $(BUILD_SHARED_LIBRARY)