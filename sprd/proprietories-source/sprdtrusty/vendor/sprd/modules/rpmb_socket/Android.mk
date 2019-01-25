#
#

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

#ifeq ($(BOARD_TEE_CONFIG), trusty)

include $(CLEAR_VARS)

LOCAL_MODULE := rpmbserver
LOCAL_SRC_FILES := rpmb_server.c
LOCAL_INIT_RC := rpmbserver.rc
LOCAL_CLFAGS += -Wall -Werror
LOCAL_SHARED_LIBRARIES := \
        liblog \
        libcutils

LOCAL_MODULE_TAGS := optional
include $(BUILD_EXECUTABLE)
#endif

include $(CLEAR_VARS)


# ==  rpmbcleinttest ==
include $(CLEAR_VARS)

LOCAL_MODULE := rpmbclienttest
LOCAL_SRC_FILES := rpmb_client_test.c
LOCAL_CLFAGS += -Wall -Werror
LOCAL_SHARED_LIBRARIES := \
        liblog \
        librpmbclient \
        libcutils

include $(BUILD_EXECUTABLE)


# ==  Static library ==
include $(CLEAR_VARS)

LOCAL_MODULE := librpmbclient

LOCAL_SRC_FILES := \
        rpmb_client.c

LOCAL_CLFAGS = -fvisibility=hidden -Wall -Werror

LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)

LOCAL_STATIC_LIBRARIES := \
    liblog \
    libcutils

include $(BUILD_STATIC_LIBRARY)


# ==   shared library ==
include $(CLEAR_VARS)

LOCAL_MODULE := librpmbclient

LOCAL_SRC_FILES := \
        rpmb_client.c

LOCAL_CLFAGS = -fvisibility=hidden -Wall -Werror

LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)

LOCAL_SHARED_LIBRARIES := \
    liblog \
    libcutils

include $(BUILD_SHARED_LIBRARY)




