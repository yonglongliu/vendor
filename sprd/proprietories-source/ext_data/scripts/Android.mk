include $(CLEAR_VARS)
LOCAL_SRC_FILES := scripts/tiny_firewall.sh

LOCAL_MODULE := tiny_firewall.sh
LOCAL_INIT_RC := scripts/tiny_firewall.rc

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_PATH := $(TARGET_OUT_EXECUTABLES)

include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := scripts/ip_monitor.sh

LOCAL_MODULE := ip_monitor.sh
LOCAL_INIT_RC := scripts/ip_monitor.rc

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_PATH := $(TARGET_OUT_EXECUTABLES)

include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := scripts/data_rps.sh

LOCAL_MODULE := data_rps.sh
LOCAL_INIT_RC := scripts/data_rps.rc

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_PATH := $(TARGET_OUT_EXECUTABLES)

include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := scripts/netbox.sh

LOCAL_MODULE := netbox.sh
LOCAL_INIT_RC := scripts/netbox.rc

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_PATH := $(TARGET_OUT_EXECUTABLES)

include $(BUILD_PREBUILT)
