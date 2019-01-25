#
# The MIT License (MIT)
# Copyright (c) 2008-2015 Travis Geiselbrecht
# Copyright (c) 2016, Spreadtrum Communications.
#
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files
# (the "Software"), to deal in the Software without restriction,
# including without limitation the rights to use, copy, modify, merge,
# publish, distribute, sublicense, and/or sell copies of the Software,
# and to permit persons to whom the Software is furnished to do so,
# subject to the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
# CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

ifeq ($(BOARD_TEE_CONFIG), trusty)

include $(CLEAR_VARS)

ifeq ($(strip $(HAS_GETUID_LIB)),true)
LOCAL_CFLAGS += -DPRODUCTION_READ_UID
endif

ifeq (sp9820e_2h10,$(filter sp9820e_2h10,$(TARGET_BOARD)))
$(warning "target_board: $(TARGET_BOARD), this for nand boot")
LOCAL_CFLAGS += -DCONFIG_NAND_BOOT_2H10
endif

LOCAL_MODULE := libteeproduction
LOCAL_SRC_FILES := trusty_production_ipc.c \
        rpmb_cli.c \
        sharkl2_efuse_for_production.c
LOCAL_CLFAGS += -fvisibility=hidden -Wall -Werror
LOCAL_C_INCLUDES += bionic/libc/kernel/uapi
LOCAL_SHARED_LIBRARIES := \
        libtrusty \
        liblog \
        libcutils \
        librpmbclient \

LOCAL_MODULE_TAGS := optional
include $(BUILD_SHARED_LIBRARY)
endif
