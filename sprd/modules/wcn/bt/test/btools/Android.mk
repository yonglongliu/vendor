LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_32_BIT_ONLY := true

LOCAL_SRC_FILES:=     \
    btools.c

LOCAL_C_INCLUDES :=

LOCAL_MODULE:= btools

LOCAL_LDLIBS += -llog

LOCAL_SHARED_LIBRARIES += libcutils   \
                          libutils    \
                          libhardware \
                          libhardware_legacy

LOCAL_PROPRIETARY_MODULE := true
include $(BUILD_EXECUTABLE)
