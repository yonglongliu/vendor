LOCAL_PATH := $(call my-dir)

TARGET_ARCH := arm

#---------------- avatar ------------------------------
########################################################
include $(CLEAR_VARS)
LOCAL_MODULE := libavatar
ifeq ($(TARGET_ARCH), arm)
LOCAL_SRC_FILES_32 := arm/lib/libavatar.so
LOCAL_MULTILIB :=  32
else ifeq ($(TARGET_ARCH), arm64)
LOCAL_SRC_FILES_32 := arm64/lib/libavatar.so
LOCAL_SRC_FILES_64 := arm64/lib64/libavatar.so
LOCAL_MULTILIB := both
else ifeq ($(TARGET_ARCH), x86_64)
LOCAL_SRC_FILES_32 := x86_64/lib/libavatar.so
LOCAL_SRC_FILES_64 := x86_64/lib64/libavatar.so
LOCAL_MULTILIB := both
endif
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
include $(BUILD_PREBUILT)
########################################################

#---------------- grape --------------------------------
########################################################
include $(CLEAR_VARS)
LOCAL_MODULE := libzmf
ifeq ($(TARGET_ARCH), arm)
LOCAL_SRC_FILES := arm/lib/libzmf.so
LOCAL_MULTILIB :=  32
else ifeq ($(TARGET_ARCH), arm64)
LOCAL_SRC_FILES_32 := arm64/lib/libzmf.so
LOCAL_SRC_FILES_64 := arm64/lib64/libzmf.so
LOCAL_MULTILIB := both
else ifeq ($(TARGET_ARCH), x86_64)
LOCAL_SRC_FILES_32 := x86_64/lib/libzmf.so
LOCAL_SRC_FILES_64 := x86_64/lib64/libzmf.so
LOCAL_MULTILIB := both
endif
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
include $(BUILD_PREBUILT)
########################################################

include $(CLEAR_VARS)
LOCAL_MODULE := libCamdrv23
ifeq ($(TARGET_ARCH), arm)
LOCAL_SRC_FILES_32 := arm/lib/libCamdrv23.so
LOCAL_MULTILIB :=  32
else ifeq ($(TARGET_ARCH), arm64)
LOCAL_SRC_FILES_32 := arm64/lib/libCamdrv23.so
LOCAL_SRC_FILES_64 := arm64/lib64/libCamdrv23.so
LOCAL_MULTILIB := both
else ifeq ($(TARGET_ARCH), x86_64)
LOCAL_SRC_FILES_32 := x86_64/lib/libCamdrv23.so
LOCAL_SRC_FILES_64 := x86_64/lib64/libCamdrv23.so
LOCAL_MULTILIB := both
endif
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
include $(BUILD_PREBUILT)
########################################################

#---------------- lemon --------------------------------
########################################################
include $(CLEAR_VARS)
LOCAL_MODULE := liblemon
ifeq ($(TARGET_ARCH), arm)
LOCAL_SRC_FILES_32 := arm/lib/liblemon.so
LOCAL_MULTILIB :=  32
else ifeq ($(TARGET_ARCH), arm64)
LOCAL_SRC_FILES_32 := arm64/lib/liblemon.so
LOCAL_SRC_FILES_64 := arm64/lib64/liblemon.so
LOCAL_MULTILIB := both
else ifeq ($(TARGET_ARCH), x86_64)
LOCAL_SRC_FILES_32 := x86_64/lib/liblemon.so
LOCAL_SRC_FILES_64 := x86_64/lib64/liblemon.so
LOCAL_MULTILIB := both
endif
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
include $(BUILD_PREBUILT)
########################################################

#---------------- watermelon, melon -----------------------
########################################################
include $(CLEAR_VARS)
LOCAL_MODULE := libmme_jrtc
ifeq ($(TARGET_ARCH), arm)
LOCAL_SRC_FILES_32 := arm/lib/libmme_jrtc.so
LOCAL_MULTILIB :=  32
else ifeq ($(TARGET_ARCH), arm64)
LOCAL_SRC_FILES_32 := arm64/lib/libmme_jrtc.so
LOCAL_SRC_FILES_64 := arm64/lib64/libmme_jrtc.so
LOCAL_MULTILIB := both
else ifeq ($(TARGET_ARCH), x86_64)
LOCAL_SRC_FILES_32 := x86_64/lib/libmme_jrtc.so
LOCAL_SRC_FILES_64 := x86_64/lib64/libmme_jrtc.so
LOCAL_MULTILIB := both
endif
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
include $(BUILD_PREBUILT)
########################################################

#---------------- ju_ipsec_server ----------------------
########################################################
include $(CLEAR_VARS)
LOCAL_MODULE := ju_ipsec_server
ifeq ($(TARGET_ARCH), arm)
LOCAL_SRC_FILES := arm/bin/ju_ipsec_server
else ifeq ($(TARGET_ARCH), arm64)
LOCAL_SRC_FILES := arm64/bin/ju_ipsec_server
else ifeq ($(TARGET_ARCH), x86_64)
LOCAL_SRC_FILES := x86_64/bin/ju_ipsec_server
endif
LOCAL_MODULE_CLASS := EXECUTABLES
include $(BUILD_PREBUILT)
########################################################

##---------------- ims_bridged --------------------------
#########################################################
#include $(CLEAR_VARS)
#LOCAL_MODULE := ims_bridged
#ifeq ($(TARGET_ARCH), arm)
#LOCAL_SRC_FILES := arm/bin/ims_bridged
#else ifeq ($(TARGET_ARCH), arm64)
#LOCAL_SRC_FILES := arm64/bin/ims_bridged
#else ifeq ($(TARGET_ARCH), x86_64)
#LOCAL_SRC_FILES := x86_64/bin/ims_bridged
#endif
#LOCAL_MODULE_CLASS := EXECUTABLES
#include $(BUILD_PREBUILT)
########################################################

#include $(CLEAR_VARS)
#LOCAL_MODULE := libimsbrd
#ifeq ($(TARGET_ARCH), arm)
#LOCAL_SRC_FILES_32 := arm/lib/libimsbrd.so
#LOCAL_MULTILIB :=  32
#else ifeq ($(TARGET_ARCH), arm64)
#LOCAL_SRC_FILES_32 := arm64/lib/libimsbrd.so
#LOCAL_SRC_FILES_64 := arm64/lib64/libimsbrd.so
#LOCAL_MULTILIB := both
#else ifeq ($(TARGET_ARCH), x86_64)
#LOCAL_SRC_FILES_32 := x86_64/lib/libimsbrd.so
#LOCAL_SRC_FILES_64 := x86_64/lib64/libimsbrd.so
#LOCAL_MULTILIB := both
#endif
#LOCAL_MODULE_TAGS := optional
#LOCAL_MODULE_SUFFIX := .so
#LOCAL_MODULE_CLASS := SHARED_LIBRARIES
#include $(BUILD_PREBUILT)
########################################################

#---------------- vowifisrvd ---------------------------
########################################################
include $(CLEAR_VARS)
LOCAL_MODULE := vowifisrvd 
ifeq ($(TARGET_ARCH), arm)
LOCAL_SRC_FILES := arm/bin/vowifisrvd
else ifeq ($(TARGET_ARCH), arm64)
LOCAL_SRC_FILES := arm64/bin/vowifisrvd
else ifeq ($(TARGET_ARCH), x86_64)
LOCAL_SRC_FILES := x86_64/bin/vowifisrvd
endif
LOCAL_MODULE_CLASS := EXECUTABLES
include $(BUILD_PREBUILT)
########################################################

include $(CLEAR_VARS)
LOCAL_MODULE := libvowifiservice
ifeq ($(TARGET_ARCH), arm)
LOCAL_SRC_FILES_32 := arm/lib/libvowifiservice.so
LOCAL_MULTILIB :=  32
else ifeq ($(TARGET_ARCH), arm64)
LOCAL_SRC_FILES_32 := arm64/lib/libvowifiservice.so
LOCAL_SRC_FILES_64 := arm64/lib64/libvowifiservice.so
LOCAL_MULTILIB := both
else ifeq ($(TARGET_ARCH), x86_64)
LOCAL_SRC_FILES_32 := x86_64/lib/libvowifiservice.so
LOCAL_SRC_FILES_64 := x86_64/lib64/libvowifiservice.so
LOCAL_MULTILIB := both
endif
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
include $(BUILD_PREBUILT)
########################################################

include $(CLEAR_VARS)
LOCAL_MODULE := libvowifiwrapper
ifeq ($(TARGET_ARCH), arm)
LOCAL_SRC_FILES_32 := arm/lib/libvowifiwrapper.so
LOCAL_MULTILIB :=  32
else ifeq ($(TARGET_ARCH), arm64)
LOCAL_SRC_FILES_32 := arm64/lib/libvowifiwrapper.so
LOCAL_SRC_FILES_64 := arm64/lib64/libvowifiwrapper.so
LOCAL_MULTILIB := both
else ifeq ($(TARGET_ARCH), x86_64)
LOCAL_SRC_FILES_32 := x86_64/lib/libvowifiwrapper.so
LOCAL_SRC_FILES_64 := x86_64/lib64/libvowifiwrapper.so
LOCAL_MULTILIB := both
endif
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
include $(BUILD_PREBUILT)
########################################################

########################################################
include $(call all-makefiles-under,$(LOCAL_PATH))
