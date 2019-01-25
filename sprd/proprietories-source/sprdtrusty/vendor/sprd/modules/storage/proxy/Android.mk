#
# Copyright (C) 2016 The Android Open-Source Project
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
#

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := sprdstorageproxyd


LOCAL_C_INCLUDES += bionic/libc/kernel/uapi
#LOCAL_C_INCLUDES += $(TOP)/kernel/include/uapi

ifeq (7.0,$(filter 7.0,$(PLATFORM_VERSION)))
LOCAL_INIT_RC := storageproxyd.rc
else ifeq ($(PLATFORM_VERSION),8.0.0)
LOCAL_PROPRIETARY_MODULE := true
#LOCAL_MODULE_PATH :=$(TARGET_OUT_VENDOR_EXECUTABLES)
LOCAL_INIT_RC := storageproxyd_androido.rc
else
#LOCAL_PROPRIETARY_MODULE := true
#LOCAL_MODULE_PATH :=$(TARGET_OUT_VENDOR_EXECUTABLES)
LOCAL_INIT_RC := storageproxyd_androidone.rc
endif

LOCAL_SRC_FILES := \
	ipc.c \
	rpmb.c \
	storage.c \
	proxy.c

LOCAL_CFLAGS = -Wall -Werror -std=gnu11

LOCAL_SHARED_LIBRARIES := \
	liblog \

LOCAL_STATIC_LIBRARIES := \
	libsprdtrustystorageinterface \
	libtrusty

include $(BUILD_EXECUTABLE)
