LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_C_INCLUDES  := system/media/audio_utils/include

LOCAL_SRC_FILES   :=  AudioCustom_Mmi.c

LOCAL_SHARED_LIBRARIES := libcutils liblog libutils

LOCAL_MODULE:= libAudioCustomMmi
LOCAL_MODULE_TAGS := optional

ifneq ($(filter $(strip $(PLATFORM_VERSION)),O 8.0.0 8.1.0),)
LOCAL_CFLAGS += -DFLAG_VENDOR_ETC
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_SHARED_LIBRARY)

