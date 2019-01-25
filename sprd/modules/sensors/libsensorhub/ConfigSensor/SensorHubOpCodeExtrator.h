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

#include <stdlib.h>
#include <sensors/SensorLocalLogdef.h>

#ifndef CONFIGSENSOR_SENSORHUBOPCODEEXTRATOR_H_
#define CONFIGSENSOR_SENSORHUBOPCODEEXTRATOR_H_

typedef enum {
  READ_OPERATION = 1,
  WRITE_OPERATION,
  CHECK_OPERATION,
  DELAY_OPERATION,
  GPIO_CONTROL,
} reg_operation_e;

typedef enum {
  AKM = 1,
  ST,
  VTC,
} mag_vendor;

typedef enum {
  ACC_FUSION = 0x01,
  ACC_GYRO_MAG_FUSION = 0x02,
  ACC_MAG_FUSION = 0x04,
  ACC_GYRO_FUSION = 0x08,
  VENDOR_ORIENTATION = 0x10,
  VENDOR_GRAVITY = 0x20,
  VENDOR_LINEAR_ACC = 0x40,
  VENDOR_ROTATION_VECTOR = 0x80,
} fusion_switch;

#pragma pack(1)
typedef struct {
  uint8_t Interface;
  uint16_t Interface_Freq;
  uint8_t ISR_Mode;
  uint8_t GPIO_ISR_Num;
  uint8_t Slave_Addr;
  uint8_t Chip_Id;
  float Resolution;
  uint16_t Range;
  uint8_t Postion;
  uint8_t Vendor;
} sensor_info_t;
#pragma pack()
/*
 *
 * */
typedef struct iic_unit {
  /* read/write/msleep */
  uint8_t operate; /* 0xFF means delay */
  uint8_t addr;
  uint8_t val;
  uint8_t mask;
} iic_unit_t;

typedef enum {
  CMD_HWINIT,
  CMD_ENABLE,
  CMD_DISABLE,
  CMD_GET_BYPASS,
  CMD_GET_FIFO,
  CMD_GET_STATUS,
  CMD_SET_STATUS,
  CMD_SET_SELFTEST,
  CMD_SET_OFFSET,
  CMD_SET_RATE,
  CMD_SET_MODE,
  CMD_SET_SETTING,
  CMD_CHECK_ID,
  CMD_SCANSALVE,
#if 0
  eCmdOpen,
  eCmdClose,
  eCmdGetRawData,
  eCmdSetDelay,
  eCmdSetRange,
  eCmdEnable,
  eCmdDisable,


  eCmdInit,
  eCmdReadId, /* Init Cmd serial maybe before cmd read id, if sensor id depand on initial */
  eCmdReset,
  eCmdEnable,
  eCmdDisable,
  eCmdSetDelay,
  eCmdBatch,
  eCmdFlush,
#endif
  eCmdTotal,
} opcode_t;

#pragma pack(1)

typedef struct prox_cali_tag {
  uint8_t collect_num;
  uint16_t ps_threshold_high;
  uint16_t ps_threshold_low;
  uint16_t dyna_cali;
  uint16_t dyna_cali_add;
  uint16_t noise_threshold;
  uint16_t noise_high_add;
  uint16_t noise_low_add;
  uint16_t ps_threshold_high_def;
  uint16_t ps_threshold_low_def;
  uint16_t dyna_cali_threahold;
  uint16_t dyna_cali_reduce;
  uint16_t value_threshold;
  uint16_t noise_reference;
} prox_cali_t;

typedef struct light_cali_tag {
  uint16_t start;
  uint16_t end;
  float a;
  float b;
} light_cali_t;

#pragma pack()


typedef struct opcode_unit {
  opcode_t eCmd;
  uint32_t nIIcUnits;
  iic_unit_t *pIIcUnits;
} opcode_unit_t;

/*
 * struct head of SensorHub firmware
 * */

// #pragma pack(8)

typedef struct fwshub_unit {
  opcode_t eCmd;
  uint32_t nShift;
  uint32_t nUnits;
} fwshub_unit_t;

#define SHUB_SENSOR_NAME "sprd-shub"
#define SHUB_SENSOR_NAME_LENGTH 10

typedef struct fwshub_head {
  char sensorName[SHUB_SENSOR_NAME_LENGTH];
  int sensorType;
  fwshub_unit_t indexOpcode[eCmdTotal];
} fwshub_head_t;

typedef struct fwiic_attr {
  uint8_t salveWriteAddr;
  uint8_t frequency;
  uint8_t IIC_Port;
} fwiic_attr_t;

typedef struct fwworkmode {
  uint8_t readMode;
  uint8_t numberIrq;
  uint8_t triggle;
} fwworkmode_t;

/*
 * Function declaration
 * */
extern void SensorHubOpcodeRegister(const char *name, sensor_info_t *sensorInfo,
                                    opcode_unit *sensorOpcode);
extern void SensorHubOpcodeRegisterAccelerometer(void);
extern void SensorHubOpcodeRegisterProximity(void);
extern void SensorHubOpcodeRegisterGyroscope(void);
extern void SensorHubOpcodeRegisterLight(void);
extern void SensorHubOpcodeRegisterMagnetic(void);
extern void SensorHubOpcodeRegisterPressure(void);
extern void SensorHubCalibrationRegisterAllSensor(void);

#endif  // CONFIGSENSOR_SENSORHUBOPCODEEXTRATOR_H_
