LOCAL_PATH := $(call my-dir)

$(warning shell echo "it should $(TARGET_BUILD_VARIANT)")


#get the lte src path 
ifeq ($(TARGET_ARCH), $(filter $(TARGET_ARCH), x86 x86_64))
    GNSS_LTE_SO_PATH := lte/x86
else
   GNSS_LTE_SO_PATH := lte/arm
endif

include $(CLEAR_VARS)
LOCAL_MODULE := liblte
LOCAL_MODULE_CLASS := SHARED_LIBRARIES

LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := liblte.so
LOCAL_SRC_FILES_32 :=  $(GNSS_LTE_SO_PATH)/32bit/liblte.so

ifeq ($(TARGET_ARCH), $(filter $(TARGET_ARCH), arm64 x86_64))
LOCAL_MODULE_STEM_64 := liblte.so
LOCAL_SRC_FILES_64 :=  $(GNSS_LTE_SO_PATH)/64bit/liblte.so
endif
LOCAL_MODULE_TAGS := optional
#LOCAL_PROPRIETARY_MODULE := true


include $(BUILD_PREBUILT)


GNSS_LCS_FLAG := TRUE

include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
	gps_lib/src/agps.c \
	gps_lib/src/agps_server.c \
	gps_lib/src/gps.c \
	gps_lib/src/gps_comm.c \
	gps_lib/src/gps_hardware.c \
	gps_lib/src/gps_daemon.c \
	gps_lib/src/gnss_libgps_api.c \
	common/src/common.c \
	common/src/gps_api.c \
	common/src/agps_interface.c \
	common/src/cp_agent.c \
   	common/src/lcs_agps.c \
	gps_lib/src/SpreadOrbit.c \
	gps_lib/src/eph.c \
	gps_lib/src/nmeaenc.c\
	gps_lib/src/gnssdaemon_client.c\
        gps_lib/src/gnss_log.c



LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/common/inc \
	$(LOCAL_PATH)/gps_lib/inc \
	hardware/libhardware_legacy \
	hardware/libhardware_legacy/include/hardware_legacy \
	hardware/libhardware/include/hardware \
	external/openssl/include

LOCAL_SHARED_LIBRARIES := \
	libc \
	libutils \
	libcutils \
	liblog \
	libpower \
	libm   \
        liblte \
	libdl

ifeq ($(strip $(SPRD_GNSS_SHARKLE_PIKL2)),true)
LOCAL_CFLAGS += -DGNSS_INTEGRATED
endif

LOCAL_CFLAGS +=  -DGNSS_LCS
LOCAL_CFLAGS +=  -Wall -Wno-missing-field-initializers  -Wunreachable-code -Wpointer-arith -Wshadow
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_MODULE := gps.default
LOCAL_MODULE_TAGS := optional
#LOCAL_PROPRIETARY_MODULE := true
$(warning shell echo "it build end libgps")
include $(BUILD_SHARED_LIBRARY)
