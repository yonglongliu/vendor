LOCAL_PATH := $(call my-dir)

ifeq ($(strip $(TARGET_ARCH)), arm)
include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS :=lib/arm/32/libsmart_amp.a
include $(BUILD_MULTI_PREBUILT)
endif

ifeq ($(strip $(TARGET_ARCH)), arm64)
include $(CLEAR_VARS)
LOCAL_MODULE := libsmart_amp
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
ifeq ($(strip $(AUDIOSERVER_MULTILIB)),)
LOCAL_MULTILIB := 32
LOCAL_MODULE_STEM_32 := libsmart_amp.a
LOCAL_SRC_FILES_32 := lib/arm/32/libsmart_amp.a
else
LOCAL_MULTILIB := $(AUDIOSERVER_MULTILIB)
LOCAL_MODULE_STEM_64 := libsmart_amp.a
LOCAL_SRC_FILES_64 := lib/arm/64/libsmart_amp.a
endif
LOCAL_MODULE_TAGS := optional
include $(BUILD_PREBUILT)
endif

ifeq ($(strip $(TARGET_ARCH)), x86_64)
include $(CLEAR_VARS)
LOCAL_MODULE := libsmart_amp
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MULTILIB := 32
LOCAL_MODULE_STEM_32 := libsmart_amp.a
LOCAL_SRC_FILES_32 := lib/x86/libsmart_amp.a
LOCAL_MODULE_TAGS := optional
include $(BUILD_PREBUILT)
endif



include $(CLEAR_VARS)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/inc \
                    external/tinyalsa/include
LOCAL_SHARED_LIBRARIES := liblog libc libcutils liblog libutils libtinyalsa
LOCAL_STATIC_LIBRARIES += libsmart_amp
LOCAL_SRC_FILES += src/smart_amp.c
ifeq ($(strip $(AUDIOSERVER_MULTILIB)),)
LOCAL_MULTILIB := 32
else
LOCAL_MULTILIB := $(AUDIOSERVER_MULTILIB)
LOCAL_CFLAGS += -DAUDIO_SERVER_64_BIT
endif
LOCAL_CFLAGS += -DDEBUG_SMART_AMP -fPIC
LOCAL_MODULE := libsmartamp
LOCAL_MODULE_TAGS := optional
ifneq ($(filter $(strip $(PLATFORM_VERSION)),O 8.0.0 8.1.0),)
LOCAL_CFLAGS += -DFLAG_VENDOR_ETC
LOCAL_PROPRIETARY_MODULE := true
endif
include $(BUILD_SHARED_LIBRARY)

