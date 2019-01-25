LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_SRC_FILES:= \
	src/test_main.cpp \
	src/test_control.cpp \
	src/test_process.cpp \
	src/test_alsa_utils.cpp \
	src/test_xml_utils.cpp

LOCAL_32_BIT_ONLY := true
LOCAL_SHARED_LIBRARIES := \
	libcutils \
	liblog \
	libutils\
	libtinyxml\
	libtinyalsa \
	libmedia \
	libhardware_legacy

ifneq ($(filter $(strip $(PLATFORM_VERSION)),O 8.0.0 8.1.0),)
LOCAL_SHARED_LIBRARIES += libaudioclient
LOCAL_CFLAGS += -DFLAG_VENDOR_ETC
LOCAL_PROPRIETARY_MODULE := true
endif

LOCAL_MODULE = audio_hardware_test

LOCAL_C_INCLUDES +=  \
	$(LOCAL_PATH)/inc/ \
	external/tinyxml \
	external/tinyalsa/include \
	system/media/audio_utils/include \
	system/media/audio_effects/include\

include $(BUILD_EXECUTABLE)
