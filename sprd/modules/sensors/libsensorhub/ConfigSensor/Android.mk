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


LOCAL_PATH := $(call my-dir)
ifeq ($(USE_SPRD_SENSOR_HUB),true)

include $(CLEAR_VARS)
LOCAL_SHARED_LIBRARIES := liblog libcutils libdl libutils
include $(LOCAL_PATH)/SensorCfg.mk
LOCAL_C_INCLUDES += \
		$(LOCAL_PATH)/../include/	\
		$(LOCAL_PATH)/include/
LOCAL_MODULE := libsensorsdrvcfg
LOCAL_MODULE_TAGS := optional
LOCAL_PROPRIETARY_MODULE := true
#LOCAL_CFLAGS := -DLOG_TAG=\"SensorHubDrvConfig\"
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := sensors.firmware
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := FAKE
LOCAL_MODULE_SUFFIX := -timestamp
include $(BUILD_SYSTEM)/base_rules.mk
$(LOCAL_BUILT_MODULE): FIRMWARE_ACC_FILE := /data/local/sensorhub/shub_fw_accelerometer
$(LOCAL_BUILT_MODULE): SYMLINK_ACC := $(TARGET_OUT_VENDOR)/firmware/shub_fw_accelerometer
$(LOCAL_BUILT_MODULE): FIRMWARE_GYR_FILE := /data/local/sensorhub/shub_fw_gyroscope
$(LOCAL_BUILT_MODULE): SYMLINK_GYR := $(TARGET_OUT_VENDOR)/firmware/shub_fw_gyroscope
$(LOCAL_BUILT_MODULE): FIRMWARE_LIG_FILE := /data/local/sensorhub/shub_fw_light
$(LOCAL_BUILT_MODULE): SYMLINK_LIG := $(TARGET_OUT_VENDOR)/firmware/shub_fw_light
$(LOCAL_BUILT_MODULE): FIRMWARE_MAG_FILE := /data/local/sensorhub/shub_fw_magnetic
$(LOCAL_BUILT_MODULE): SYMLINK_MAG := $(TARGET_OUT_VENDOR)/firmware/shub_fw_magnetic
$(LOCAL_BUILT_MODULE): FIRMWARE_PRO_FILE := /data/local/sensorhub/shub_fw_proximity
$(LOCAL_BUILT_MODULE): SYMLINK_PRO := $(TARGET_OUT_VENDOR)/firmware/shub_fw_proximity
$(LOCAL_BUILT_MODULE): FIRMWARE_PRE_FILE := /data/local/sensorhub/shub_fw_pressure
$(LOCAL_BUILT_MODULE): SYMLINK_PRE := $(TARGET_OUT_VENDOR)/firmware/shub_fw_pressure

$(LOCAL_BUILT_MODULE): FIRMWARE_CALIBRATION_FILE := /data/local/sensorhub/shub_fw_calibration
$(LOCAL_BUILT_MODULE): SYMLINK_CALIBRATION := $(TARGET_OUT_VENDOR)/firmware/shub_fw_calibration

$(LOCAL_BUILT_MODULE): $(LOCAL_PATH)/Android.mk
$(LOCAL_BUILT_MODULE):
	$(hide) mkdir -p $(dir $@)
	$(hide) mkdir -p $(dir $(SYMLINK_ACC))
	$(hide) rm -rf $@
ifneq ($(strip $(SENSOR_HUB_ACCELEROMETER)), $(filter $(SENSOR_HUB_ACCELEROMETER), null))
	$(hide) echo "Symlink: $(SYMLINK_ACC) -> $(FIRMWARE_ACC_FILE)"
	$(hide) rm -rf $(SYMLINK_ACC)
	$(hide) ln -sf $(FIRMWARE_ACC_FILE) $(SYMLINK_ACC)
endif
ifneq ($(strip $(SENSOR_HUB_GYROSCOPE)), $(filter $(SENSOR_HUB_GYROSCOPE), null))
	$(hide) echo "Symlink: $(SYMLINK_GYR) -> $(FIRMWARE_GYR_FILE)"
	$(hide) rm -rf $(SYMLINK_GYR)
	$(hide) ln -sf $(FIRMWARE_GYR_FILE) $(SYMLINK_GYR)
endif
ifneq ($(strip $(SENSOR_HUB_LIGHT)), $(filter $(SENSOR_HUB_LIGHT), null))
	$(hide) echo "Symlink: $(SYMLINK_LIG) -> $(FIRMWARE_LIG_FILE)"
	$(hide) rm -rf $(SYMLINK_LIG)
	$(hide) ln -sf $(FIRMWARE_LIG_FILE) $(SYMLINK_LIG)
endif
ifneq ($(strip $(SENSOR_HUB_MAGNETIC)), $(filter $(SENSOR_HUB_MAGNETIC), null))
	$(hide) echo "Symlink: $(SYMLINK_MAG) -> $(FIRMWARE_MAG_FILE)"
	$(hide) rm -rf $(SYMLINK_MAG)
	$(hide) ln -sf $(FIRMWARE_MAG_FILE) $(SYMLINK_MAG)
endif
ifneq ($(strip $(SENSOR_HUB_PROXIMITY)), $(filter $(SENSOR_HUB_PROXIMITY), null))
	$(hide) echo "Symlink: $(SYMLINK_PRO) -> $(FIRMWARE_PRO_FILE)"
	$(hide) rm -rf $(SYMLINK_PRO)
	$(hide) ln -sf $(FIRMWARE_PRO_FILE) $(SYMLINK_PRO)
endif
ifneq ($(strip $(SENSOR_HUB_PRESSURE)), $(filter $(SENSOR_HUB_PRESSURE), null))
	$(hide) echo "Symlink: $(SYMLINK_PRE) -> $(FIRMWARE_PRE_FILE)"
	$(hide) rm -rf $(SYMLINK_PRE)
	$(hide) ln -sf $(FIRMWARE_PRE_FILE) $(SYMLINK_PRE)
endif
ifneq ($(strip $(SENSOR_HUB_CALIBRATION)), $(filter $(SENSOR_HUB_CALIBRATION), null))
	$(hide) echo "Symlink: $(SYMLINK_CALIBRATION) -> $(FIRMWARE_CALIBRATION_FILE)"
	$(hide) rm -rf $(SYMLINK_CALIBRATION)
	$(hide) ln -sf $(FIRMWARE_CALIBRATION_FILE) $(SYMLINK_CALIBRATION)
endif
	$(hide) touch $@
endif # USE_SPRD_SENSOR_HUB
