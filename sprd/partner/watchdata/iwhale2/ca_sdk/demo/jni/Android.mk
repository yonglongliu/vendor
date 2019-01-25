LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

#LOCAL_MODULE_TAGS := debug eng


LOCAL_MODULE := wdca_debug_test 

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/src/\
	$(LOCAL_PATH)/src/include


LOCAL_SRC_FILES := src/main.c	\

LOCAL_CFLAGS += -DCA_CLIENT_DEBUG
LOCAL_CFLAGS += -DWITH_GP_TESTS
LOCAL_CFLAGS +=  -pie -fPIE -fno-stack-protector
LOCAL_CFLAGS += -D_FORTIFY_SOURCE=0 
LOCAL_LDFLAGS += -pie -fPIE

LOCAL_LDLIBS += -llog

LOCAL_LDLIBS += $(LOCAL_PATH)/lib64/$(APP_ABI)/libteec.so\
		$(LOCAL_PATH)/lib64/$(APP_ABI)/libc.so \
		$(LOCAL_PATH)/lib64/$(APP_ABI)/libm.so \
		$(LOCAL_PATH)/lib64/$(APP_ABI)/libstdc++.so \
		$(LOCAL_PATH)/lib64/$(APP_ABI)/libdl.so \
		$(LOCAL_PATH)/lib64/$(APP_ABI)/libcrypto.so\
		$(LOCAL_PATH)/lib64/$(APP_ABI)/libc++.so \
		$(LOCAL_PATH)/lib64/$(APP_ABI)/libcutils.so

include $(BUILD_EXECUTABLE)
