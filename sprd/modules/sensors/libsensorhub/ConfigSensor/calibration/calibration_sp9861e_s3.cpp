/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <cutils/log.h>

#include "SensorHubOpCodeExtrator.h"

#define SENSORHUB_FIRMWARE_PATH "/data/local/sensorhub/shub_fw_"

/*Mag sensor soft calibration data*/
static int eMagSoftCaliArray[36] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0,
};

static prox_cali_t eProxCaliArray[] = {
{
    .collect_num = 7,
    .ps_threshold_high = 200,
    .ps_threshold_low = 100,
    .dyna_cali = 500,
    .dyna_cali_add = 10,
    .noise_threshold = 500,
    .noise_high_add = 120,
    .noise_low_add = 70,
    .ps_threshold_high_def = 200,
    .ps_threshold_low_def = 100,
    .dyna_cali_threahold = 20,
    .dyna_cali_reduce = 10,
    .value_threshold = 500,
    .noise_reference = 0xFFFF,
  },
};

static light_cali_t eLightCaliArray[] = {
};

static uint8_t eAccCaliDataArray[] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

static uint8_t eGyroCaliDataArray[] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

/*actual - raw_data*/
static float berometer_offset = 3.28f;
static uint16_t fusion_mode = ACC_GYRO_MAG_FUSION;

void SensorHubCalibrationRegister(const char *name)
{
  char fixed_sysfs_path[128];
  int fixed_sysfs_path_len;
  size_t retval = 0;
  uint8_t calibration_size[7];

  strcpy(fixed_sysfs_path, SENSORHUB_FIRMWARE_PATH);
  fixed_sysfs_path_len = strlen(SENSORHUB_FIRMWARE_PATH);

  strcpy(&fixed_sysfs_path[fixed_sysfs_path_len], name);

  FILE *fdfirm = fopen(fixed_sysfs_path, "wb");

  SH_LOG("strlen = %zu %s\n", strlen(name), fixed_sysfs_path);
  fixed_sysfs_path_len += strlen(name);

  if (fixed_sysfs_path_len > 128) {
    SH_ERR("the file path is too long\n");
    return;
  } else {
    ;
  }

  if (NULL == fdfirm) {
    SH_ERR("open %s failed %s\n", fixed_sysfs_path, strerror(errno));
    return;
  } else {
    SH_LOG("open %s success\n", fixed_sysfs_path);
  }

  /* calculate data size */
  calibration_size[0] = sizeof(eMagSoftCaliArray);
  calibration_size[1] = sizeof(eProxCaliArray);
  calibration_size[2] = sizeof(eLightCaliArray);
  calibration_size[3] = sizeof(eAccCaliDataArray);
  calibration_size[4] = sizeof(eGyroCaliDataArray);
  calibration_size[5] = sizeof(berometer_offset);
  calibration_size[6] = sizeof(fusion_mode);

  /* write head*/
  retval = fwrite(calibration_size, sizeof(calibration_size), 1, fdfirm);
  if(retval == 0) {
    SH_ERR("[SENSORHUB]write calibration_size failed\n");
  } else {
    SH_LOG("[SENSORHUB]write calibration_size retval = %zu\n", retval);
  }

  /* write body */
  /* write M sensor soft cali data*/
  retval = fwrite(eMagSoftCaliArray, sizeof(eMagSoftCaliArray), 1, fdfirm);
  if(retval == 0) {
    SH_ERR("[SENSORHUB]write eMagSoftCaliArray failed\n");
  } else {
    SH_LOG("[SENSORHUB]write eMagSoftCaliArray retval = %zu\n", retval);
  }
  /* write prox sensor cali data*/
  retval = fwrite(eProxCaliArray, sizeof(eProxCaliArray), 1, fdfirm);
  if(retval == 0) {
    SH_ERR("[SENSORHUB]write eProxCaliArray failed\n");
  } else {
    SH_LOG("[SENSORHUB]write eProxCaliArray retval = %zu\n", retval);
  }
  /* write light sensor cali data*/
  retval = fwrite(eLightCaliArray, sizeof(eLightCaliArray), 1, fdfirm);
  if(retval == 0) {
    SH_ERR("[SENSORHUB]write eLightCaliArray failed\n");
  } else {
    SH_LOG("[SENSORHUB]write eLightCaliArray retval = %zu\n", retval);
  }
  /* write acc sensor cali data*/
  retval = fwrite(eAccCaliDataArray, sizeof(eAccCaliDataArray), 1, fdfirm);
  if(retval == 0) {
    SH_ERR("[SENSORHUB]write eAccCaliDataArray failed\n");
  } else {
    SH_LOG("[SENSORHUB]write eAccCaliDataArray retval = %zu\n", retval);
  }
  /* write gyro sensor cali data*/
  retval = fwrite(eGyroCaliDataArray, sizeof(eGyroCaliDataArray), 1, fdfirm);
  if(retval == 0) {
    SH_ERR("[SENSORHUB]write eGyroCaliDataArray failed\n");
  } else {
    SH_LOG("[SENSORHUB]write eGyroCaliDataArray retval = %zu\n", retval);
  }
  /* write pressure sensor cali data*/
  retval = fwrite(&berometer_offset, sizeof(berometer_offset), 1, fdfirm);
  if(retval == 0) {
    SH_ERR("[SENSORHUB]write actual_berometer failed\n");
  } else {
    SH_LOG("[SENSORHUB]write actual_berometer retval = %zu\n", retval);
  }
  /* write fusion mode*/
  retval = fwrite(&fusion_mode, sizeof(fusion_mode), 1, fdfirm);
  if(retval == 0) {
    SH_ERR("[SENSORHUB]write fusion_mode failed\n");
  } else {
    SH_LOG("[SENSORHUB]write fusion_mode retval = %zu\n", retval);
  }

  /* write tail */
  fclose(fdfirm);

  return;
}

void SensorHubCalibrationRegisterAllSensor(void) {
  SensorHubCalibrationRegister("calibration");
  return;
}
