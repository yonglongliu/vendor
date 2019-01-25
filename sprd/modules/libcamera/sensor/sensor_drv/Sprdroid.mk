#
# Copyright (C) 2008 The Android Open Source Project
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
CUR_DIR := sensor_drv
LOCAL_SRC_DIR := $(LOCAL_PATH)/$(CUR_DIR)
SUB_DIR := classic

#sensor makefile config
SENSOR_FILE_COMPILER := $(CAMERA_SENSOR_TYPE_BACK)
SENSOR_FILE_COMPILER += $(CAMERA_SENSOR_TYPE_FRONT)
SENSOR_FILE_COMPILER += $(CAMERA_SENSOR_TYPE_BACK_EXT)
SENSOR_FILE_COMPILER += $(CAMERA_SENSOR_TYPE_FRONT_EXT)

#SENSOR_FILE_COMPILER := $(shell echo $(SENSOR_FILE_COMPILER) | tr a-z A-Z)
#SENSOR_FILE_COMPILER += $(shell echo $(SENSOR_FILE_COMPILER) | tr A-Z a-z)
SENSOR_FILE_COMPILER := $(shell echo $(SENSOR_FILE_COMPILER))
#$(warning $(SENSOR_FILE_COMPILER))

sensor_comma:=,
sensor_empty:=
sensor_space:=$(sensor_empty)

#$(warning $(LOCAL_PATH))
#SENSOR_PREFIX_DIR := vendor/sprd/modules/libcamera/sensor

split_sensor:=$(sort $(subst $(sensor_comma),$(sensor_space) ,$(shell echo $(SENSOR_FILE_COMPILER))))
#$(warning $(split_sensor))

sensor_macro:=$(shell echo $(split_sensor) | tr a-z A-Z)
#$(warning $(sensor_macro))
$(foreach item,$(sensor_macro), $(eval LOCAL_CFLAGS += -D$(shell echo $(item))))

define sensor-ic-identifier
$(shell echo $(sensor_macro) | grep -w $(shell echo $(1) | tr a-z A-Z))
endef

define sensor-c-file-search
$(eval file_dir :=$(shell find $(LOCAL_SRC_DIR)/$(SUB_DIR) -name $(1) -type d))
$(eval file :=$(wildcard *.c $(file_dir)/*.c))
$(eval c_src_dir :=$(shell echo $(file_dir)| sed s:^$(LOCAL_PATH)/::g))
$(eval LOCAL_SRC_FILES += $(c_src_dir)/$(notdir $(file)))
endef
#sensor makefile config

LOCAL_C_INCLUDES += $(shell find $(LOCAL_SRC_DIR)/$(SUB_DIR) -maxdepth 2 -type d)

LOCAL_C_INCLUDES += sensor_ic_drv.h

#LOCAL_SRC_FILES += $(shell find $(LOCAL_SRC_DIR)/$(SUB_DIR) -maxdepth 3 -iregex ".*\.\(c\)" | sed s:^$(LOCAL_PATH)/::g )

ifeq ($(strip $(TARGET_BOARD_SBS_MODE_SENSOR)),true)
LOCAL_STATIC_LIBRARIES += libsensor_sbs
LOCAL_C_INCLUDES += $(LOCAL_PATH)/sensor_drv/classic/OmniVision/sbs/sensor_sbs.h
endif

LOCAL_SRC_FILES += \
    sensor_drv/sensor_ic_drv.c


#$(foreach item,$(split_sensor),$(eval $(call sensor-c-file-search,$(shell echo $(item) | tr A-Z a-z))))
$(foreach item,$(split_sensor),$(eval $(call sensor-c-file-search,$(shell echo $(item)))))
