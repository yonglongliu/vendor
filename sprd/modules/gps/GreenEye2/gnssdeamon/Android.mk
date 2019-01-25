LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := gnssdaemon.c 
		  
LOCAL_STATIC_LIBRARIES := libcutils


LOCAL_MODULE := gpsd
LOCAL_MODULE_TAGS := optional

LOCAL_SHARED_LIBRARIES := liblog
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_EXECUTABLE)


