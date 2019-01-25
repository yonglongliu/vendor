#
# Copyright (C) 2015 The Android Open-Source Project
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

# WARNING: Everything listed here will be built on ALL platforms,
# including x86, the emulator, and the SDK.  Modules must be uniquely
# named (liblights.panda), and must build everywhere, or limit themselves
# to only building on ARM if they include assembly. Individual makefiles
# are responsible for having their own logic, for fine-grained control.

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

ifeq (7.0,$(filter 7.0,$(PLATFORM_VERSION)))
CONFIG_KEYMASTER_VERSION := 1
else
CONFIG_KEYMASTER_VERSION := 2
endif

LOCAL_MODULE := trusty_keymaster_tests
LOCAL_SRC_FILES := \
	android_keymaster_test_utils.cpp \
	android_keymaster_test.cpp \
	gtest_main.cpp \
	attestation_record.cpp \
	../trusty_keymaster_ipc.c \
	../trusty_keymaster$(CONFIG_KEYMASTER_VERSION)_device.cpp

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../  \
	system/keymaster/

LOCAL_CFLAGS = -Wall -Wunused -DKEYMASTER_NAME_TAGS
LOCAL_CFLAGS += -DTRUSTY_KEYMASTER_VERSION=$(CONFIG_KEYMASTER_VERSION)
LOCAL_CLANG := true
LOCAL_CLANG_CFLAGS += -Wno-error=unused-const-variable -Wno-error=unused-private-field
LOCAL_CLANG_CFLAGS += -fno-sanitize-coverage=edge,indirect-calls,8bit-counters,trace-cmp
LOCAL_MODULE_TAGS := tests
LOCAL_SHARED_LIBRARIES := \
	libkeymaster_messages \
	libkeymaster1 \
	libsoftkeymasterdevice \
	libtrusty \
	liblog \
	libcutils \
	libcrypto \
	libhardware

include $(BUILD_NATIVE_TEST)
