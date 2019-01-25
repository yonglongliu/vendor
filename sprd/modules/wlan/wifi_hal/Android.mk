# Copyright (C) 2011 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

LOCAL_PATH := $(call my-dir)

# Make the HAL library
# ============================================================
include $(CLEAR_VARS)

LOCAL_CFLAGS := -Wno-unused-parameter
LOCAL_CFLAGS += -DDEBUG=1

# gscan.cpp: address of array 'cached_results[i].results' will always evaluate to 'true'
LOCAL_CLANG_CFLAGS := -Wno-pointer-bool-conversion

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH) \
	external/libnl/include \
	$(call include-path-for, libhardware_legacy)/hardware_legacy \
	external/wpa_supplicant_8/src/drivers \
	$(TARGET_OUT_HEADERS)/libwpa_client \
	$(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include \
	$(TARGET_OUT_HEADERS)/cld80211-lib

LOCAL_SRC_FILES := \
	wifi_hal.cpp \
	common.cpp \
	cpp_bindings.cpp \
	llstats.cpp \
	gscan.cpp \
	gscan_event_handler.cpp \
	rtt.cpp \
	ifaceeventhandler.cpp \
	tdls.cpp \
	nan.cpp \
	nan_ind.cpp \
	nan_req.cpp \
	nan_rsp.cpp \
	wificonfig.cpp \
	wifilogger.cpp \
	wifilogger_diag.cpp \
	ring_buffer.cpp \
	rb_wrapper.cpp \
	rssi_monitor.cpp \
	roam.cpp

LOCAL_MODULE := libwifi-hal-sprd
LOCAL_PROPRIETARY_MODULE := true
LOCAL_CLANG := true
LOCAL_SHARED_LIBRARIES += libnetutils liblog libwpa_client libcld80211

ifneq ($(wildcard external/libnl),)
LOCAL_SHARED_LIBRARIES += libnl
LOCAL_C_INCLUDES += external/libnl/include
else
LOCAL_SHARED_LIBRARIES += libnl_2
LOCAL_C_INCLUDES += external/libnl-headers
endif

include $(BUILD_STATIC_LIBRARY)

# Make the HAL library
# ============================================================

include $(CLEAR_VARS)

LOCAL_CFLAGS := -Wno-unused-parameter
LOCAL_CFLAGS += -DDEBUG=1

LOCAL_STATIC_LIBRARIES += libwifi-hal-sprd

#LOCAL_CFLAGS += -std=c99

LOCAL_C_INCLUDES += \
    external/libnl/include \
    $(call include-path-for, libhardware_legacy)/hardware_legacy \
    external/wpa_supplicant_8/src/drivers

LOCAL_SHARED_LIBRARIES += \
        libcutils \
        liblog \
        libutils \
        libhardware \
        libhardware_legacy \
        libnl \
        libdl

LOCAL_SRC_FILES := \
    ./test/wifi_hal_cli.cpp

LOCAL_SHARED_LIBRARIES += libnetutils liblog libcld80211

LOCAL_MODULE := wifi-hal-cli
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_EXECUTABLE)
