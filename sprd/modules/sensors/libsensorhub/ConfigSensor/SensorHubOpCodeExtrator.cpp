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
/*
 *
 *
 *
 * */
#define SENSORHUB_FIRMWARE_PATH "/data/local/sensorhub/shub_fw_"

void SensorHubOpcodeRegister(const char *name,
                             sensor_info_t *sensorInfo,
                             opcode_unit *sensorOpcode) {
  char fixed_sysfs_path[128];
  int fixed_sysfs_path_len;
  strcpy(fixed_sysfs_path, SENSORHUB_FIRMWARE_PATH);
  fixed_sysfs_path_len = strlen(SENSORHUB_FIRMWARE_PATH);

  strcpy(&fixed_sysfs_path[fixed_sysfs_path_len], name);

  FILE *fdfirm = fopen(fixed_sysfs_path, "wb");

  SH_LOG("strlen = %zu %s\n", strlen(name), fixed_sysfs_path);

  if (NULL == fdfirm) {
    SH_ERR("open %s failed %s\n", fixed_sysfs_path, strerror(errno));
    return;
  } else {
    SH_LOG("open %s success\n", fixed_sysfs_path);
  }

  fwshub_head_t *fw = (fwshub_head_t *)calloc(1, sizeof(fwshub_head_t));

  /* Init head */
  memcpy(fw->sensorName, SHUB_SENSOR_NAME, SHUB_SENSOR_NAME_LENGTH);
  // strcpy(fw->sensorName, name);
  fw->sensorType = 1;

  int idx = 0;
  size_t retval = 0;

  fw->indexOpcode[idx].eCmd = sensorOpcode[idx].eCmd;
  fw->indexOpcode[idx].nShift = 0;
  fw->indexOpcode[idx].nUnits = sensorOpcode[idx].nIIcUnits;

  for (idx = 1; idx < eCmdTotal; idx++) {
    fw->indexOpcode[idx].eCmd = sensorOpcode[idx].eCmd;
    fw->indexOpcode[idx].nShift =
        fw->indexOpcode[idx - 1].nShift + fw->indexOpcode[idx - 1].nUnits;
    fw->indexOpcode[idx].nUnits = sensorOpcode[idx].nIIcUnits;
  }
  /* write head */
  fwrite(fw, sizeof(fwshub_head_t), 1, fdfirm);

  /* write attr */
  fwrite(sensorInfo, sizeof(sensor_info_t), 1, fdfirm);

  /* write body */
  for (idx = 0; idx < eCmdTotal; idx++) {
    retval = fwrite(sensorOpcode[idx].pIIcUnits,
                    sizeof(iic_unit_t),
                    sensorOpcode[idx].nIIcUnits,
                    fdfirm);
  }

  /* write tail */
  free(fw);
  fclose(fdfirm);

  return;
}


void SensorHubOpcodeExtrator(void) {
#ifdef SENSORHUB_WITH_ACCELEROMETER
    SensorHubOpcodeRegisterAccelerometer();
#endif
#ifdef SENSORHUB_WITH_PROXIMITY
    SensorHubOpcodeRegisterProximity();
#endif
#ifdef SENSORHUB_WITH_GYROSCOPE
    SensorHubOpcodeRegisterGyroscope();
#endif
#ifdef SENSORHUB_WITH_LIGHT
    SensorHubOpcodeRegisterLight();
#endif
#ifdef SENSORHUB_WITH_MAGNETIC
    SensorHubOpcodeRegisterMagnetic();
#endif
#ifdef SENSORHUB_WITH_PRESSURE
    SensorHubOpcodeRegisterPressure();
#endif
#ifdef SENSORHUB_WITH_CALIBRATION
    SensorHubCalibrationRegisterAllSensor();
#endif
    return;
}
