#ifneq ( $(TARGET_BUILD_VARIANT),user)
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
#include $(BUILD_SHARED_LIBRARY)

ifeq ($(TARGET_BUILD_VARIANT),userdebug)
   LOCAL_CFLAGS += -DOPEN_MODEM_LOG
   LOCAL_CFLAGS += -DOPEN_REBOOT_TEST
endif

LOCAL_CFLAGS += -DBUILD_ENG
LOCAL_SRC_FILES:= HTTPServer.cpp HTTPRequest.cpp HTTPResponse.cpp HTTPServerMain.cpp ATProcesser.cpp ./xmllib/xmlparse.cpp ./xmllib/tinyxmlparser.cpp ./xmllib/tinyxmlerror.cpp ./xmllib/tinyxml.cpp  ./xmllib/tinystr.cpp
#eng_appclient_lib.c

LOCAL_C_INCLUDES    += HTTP.h
LOCAL_C_INCLUDES    += ./xmllib/xmlparse.h
LOCAL_C_INCLUDES    += ./xmllib/tinyxml.h
LOCAL_C_INCLUDES    += ./xmllib/tinystr.h

#engclient.h HTTP.h engopt.h engapi.h
#LOCAL_C_INCLUDES    +=  device/sprd/common/apps/engineeringmodel/engcs
#zsw LOCAL_C_INCLUDES    +=  vendor/sprd/platform/packages/libs/libatchannel/
LOCAL_C_INCLUDES    +=   vendor/sprd/modules/libatci/
LOCAL_C_INCLUDES    +=  system/extras/ubifs_utils/include/mtd/
LOCAL_MODULE := engmoded
LOCAL_STATIC_LIBRARIES := libcutils
#LOCAL_SHARED_LIBRARIES := libstlport libatchannel liblog
#zsw LOCAL_SHARED_LIBRARIES := libatchannel liblog
LOCAL_SHARED_LIBRARIES := libutils libatci liblog librecovery libdl
#libengclient
LOCAL_MODULE_TAGS := optional
#include external/stlport/libstlport.mk
include $(BUILD_EXECUTABLE)
#endif
