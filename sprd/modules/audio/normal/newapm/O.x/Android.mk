ifneq ($(filter $(strip $(PLATFORM_VERSION)),O 8.0.0 8.1.0),)

ifneq ($(USE_LEGACY_AUDIO_POLICY), 1)
ifeq ($(USE_CUSTOM_AUDIO_POLICY), 1)
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := AudioPolicyManagerSPRD.cpp

LOCAL_C_INCLUDES := \
    $(TOPDIR)frameworks/av/services/audiopolicy \
    $(TOPDIR)frameworks/av/services/audiopolicy/managerdefault \
    $(TOPDIR)frameworks/av/services/audiopolicy/engine/interface \
    $(TOPDIR)frameworks/av/services/audiopolicy/common/managerdefinitions/include \
    $(TOPDIR)frameworks/av/services/audiopolicy/common/include \
    $(TOPDIR)frameworks/av/services/audiopolicy/common/managerdefinitions/include \
    $(TOPDIR)system/core/base/include




LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libutils \
    liblog \
    libsoundtrigger \
    libaudiopolicymanagerdefault

LOCAL_STATIC_LIBRARIES := \
    libmedia_helper

LOCAL_MODULE := libaudiopolicymanager


include $(BUILD_SHARED_LIBRARY)

endif
endif

endif
