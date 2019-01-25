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

#ifndef CONFIGFEATURE_INCLUDE_SENSORS_SENSORCONFIGFEATURE_H_
#define CONFIGFEATURE_INCLUDE_SENSORS_SENSORCONFIGFEATURE_H_
typedef struct {
    const char *ctlName;
    int         ctlMode;
    const char *drvName;
    int         drvMode;
    sensor_t   *sensorList;
    int         listSize;
}shub_drv_cfg_t, *pshub_drv_cfg_t;

typedef struct {
    int cfgSize;
    pshub_drv_cfg_t cfg;
}shub_cfg_t, *pshub_cfg_t;


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


/*
 * function:
 *      sensorGetListSize
 *
 *   return:
 *      int, size of list
 * */
int shubGetListSize(void);


/*
 * function:
 *      sensorGetList
 *
 *   return:
 *      sensor_t
 * */
const struct sensor_t * shubGetList(void);


/*
 * function:
 *      sensorGetDrvConfig
 *
 *   return:
 *      pshub_cfg_t
 * */
pshub_cfg_t shubGetDrvConfig(void);

#endif  // CONFIGFEATURE_INCLUDE_SENSORS_SENSORCONFIGFEATURE_H_
