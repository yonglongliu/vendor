#
# Copyright (C) 2016 The Android Open Source Project
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
LOCAL_MODULE := sprd-secure-storage-unit-test
LOCAL_CFLAGS += -g -Wall -Werror -std=gnu++11 -Wno-missing-field-initializers
LOCAL_STATIC_LIBRARIES := \
	libsprdtrustystorageinterface \
	libsprdtrustystorage \
	libtrusty \
	liblog
LOCAL_SRC_FILES := main.cpp
include $(BUILD_NATIVE_TEST)



include $(CLEAR_VARS)

LOCAL_MODULE := rpmb_test
LOCAL_C_INCLUDES += bionic/libc/kernel/uapi
LOCAL_SRC_FILES := rpmb_test.c


LOCAL_SHARED_LIBRARIES := \
     liblog \
     libcrypto

LOCAL_STATIC_LIBRARIES :=

include $(BUILD_EXECUTABLE)





include $(CLEAR_VARS)
LOCAL_MODULE := storage_test
LOCAL_CFLAGS += -g -Wall -Werror -std=gnu++11 -Wno-missing-field-initializers
LOCAL_STATIC_LIBRARIES := \
	libsprdtrustystorageinterface \
	libsprdtrustystorage \
	libtrusty \
	liblog
LOCAL_SRC_FILES := storage_test.cpp
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := storage_unittest
LOCAL_CFLAGS += -g -Wall -Werror -std=gnu++11 -Wno-missing-field-initializers
LOCAL_STATIC_LIBRARIES := \
	libsprdtrustystorageinterface \
	libsprdtrustystorage \
	libtrusty \
	liblog
LOCAL_SRC_FILES := storage_unittest.cpp
include $(BUILD_EXECUTABLE)
