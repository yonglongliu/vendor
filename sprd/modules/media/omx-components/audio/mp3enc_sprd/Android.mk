LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
        SprdMP3Encoder.cpp \

LOCAL_CFLAGS := -fno-strict-aliasing -DOPT_DCT_FIXED31 -DFPM_64BIT -D__ASO__
LOCAL_LDFLAGS += -Wl,--no-warn-shared-textrel

#LOCAL_STATIC_LIBRARIES :=
LOCAL_SHARED_LIBRARIES := \
          libstagefright libstagefright_omx libstagefright_foundation libstagefrighthw libutils libui libbinder libdl libcutils liblog

LOCAL_C_INCLUDES := \
        frameworks/av/media/libstagefright/include \
        $(TOP)/vendor/sprd/modules/media/libstagefrighthw/include \
        frameworks/native/include/media/openmax \
        $(TOP)/vendor/sprd/modules/libmemion \
        $(LOCAL_PATH)/src \
        $(LOCAL_PATH)/../common/include \
        $(LOCAL_PATH)/../common


LOCAL_MODULE :=libstagefright_sprd_mp3enc
LOCAL_MODULE_TAGS := optional
LOCAL_32_BIT_ONLY := true
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_SHARED_LIBRARY)
