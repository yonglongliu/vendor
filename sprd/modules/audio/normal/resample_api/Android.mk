LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_C_INCLUDES += vendor/sprd/modules/resampler\
                    vendor/sprd/modules/audio/normal/DumpData \
                    system/media/audio_utils/include

LOCAL_SHARED_LIBRARIES := liblog libc libcutils liblog libutils libaudioutils

ifeq ($(strip $(AUDIOSERVER_MULTILIB)),)
LOCAL_32_BIT_ONLY := true
else
LOCAL_MULTILIB := $(AUDIOSERVER_MULTILIB)
endif
LOCAL_LDFLAGS += -Wl,--no-warn-shared-textrel
LOCAL_STATIC_LIBRARIES := libaudioresamplersprd

ifeq ($(strip $(AUDIO_RECORD_NR)),true)
LOCAL_CFLAGS += -DUSE_RESAMPLER_44_1_K
endif

LOCAL_SRC_FILES += sprd_resample_48kto44k.c

LOCAL_MODULE := libresample48kto44k

LOCAL_MODULE_TAGS := optional

ifneq ($(filter $(strip $(PLATFORM_VERSION)),O 8.0.0 8.1.0),)
LOCAL_CFLAGS += -DFLAG_VENDOR_ETC
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_SHARED_LIBRARY)

