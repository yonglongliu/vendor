/*
 * Copyright (C) 2010 The Android Open Source Project
 * Copyright (C) 2012-2015, The Linux Foundation. All rights reserved.
 *
 * Not a Contribution, Apache license notifications and license are retained
 * for attribution purposes only.
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

#include <cutils/log.h>
#include <errno.h>
#include <fcntl.h>
#include <hardware/hardware.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/cdefs.h>
#include <sys/types.h>
#include <unistd.h>
#include "SprdHWLayer.h"
#include "DpuDevice.h"

#define DPU_HEADER_VERSION 1

#define DPU_DEVICE_API_VERSION_1_0 \
  HARDWARE_DEVICE_API_VERSION_2(1, 0, DPU_HEADER_VERSION)

#define GET_DPU_RETURN_X_IF_NULL(X, dev)          \
  DpuDevice *dpu = static_cast<DpuDevice *>(dev); \
  do {                                            \
    if (!dpu) {                                   \
      ALOGE("invalid dpu device.");               \
      return X;                                   \
    }                                             \
  } while (0)

#define GET_DPU_RETURN_ERROR_IF_NULL(dev) GET_DPU_RETURN_X_IF_NULL(-EINVAL, dev)
#define GET_DPU_RETURN_VOID_IF_NULL(dev) GET_DPU_RETURN_X_IF_NULL()

typedef struct dpu_module { struct hw_module_t common; } dpu_module_t;

namespace android {

static int dpu_device_prepare(dpu_device_t *dev, SprdHWLayer **list, int cnt,
                              bool *support) {
	GET_DPU_RETURN_ERROR_IF_NULL(dev);
  return dpu->prepare(list, cnt, support);
}

static int dpu_device_init(dpu_device_t *dev, bool *support) {
	GET_DPU_RETURN_ERROR_IF_NULL(dev);
  return dpu->init(support);
}
static int dpu_device_close(struct hw_device_t *dev) {
  ALOGI("dpu_device_close: close dpu device");

  dpu_device_t *dpu = (dpu_device_t *)dev;

  DpuDevice *dpuDevice = static_cast<DpuDevice *>(dpu);
  delete dpuDevice;

  return 0;
}

static int dpu_device_open(const struct hw_module_t *module, const char *name,
                           struct hw_device_t **device) {
  if (strcmp(name, DPU_HARDWARE_MODULE_ID)) return -EINVAL;

  ALOGI("dpu_device_open: open dpu device module");

  DpuDevice *dpu = new DpuDevice();

  // setup dpu methods
  dpu->dpu_device_t::common.tag = HARDWARE_DEVICE_TAG;
  dpu->dpu_device_t::common.version = DPU_DEVICE_API_VERSION_1_0;
  dpu->dpu_device_t::common.module = const_cast<hw_module_t *>(module);
  dpu->dpu_device_t::common.close = dpu_device_close;
  dpu->dpu_device_t::prepare = dpu_device_prepare;
  dpu->dpu_device_t::init = dpu_device_init;

  *device = &dpu->dpu_device_t::common;

  return 0;
}

}  // namespace android

static hw_module_methods_t dpu_module_methods = {
		.open = dpu_device_open
};

dpu_module_t HAL_MODULE_INFO_SYM = {
  .common = {
    .tag = HARDWARE_MODULE_TAG,
    .version_major = 1,
    .version_minor = 0,
    .id = DPU_HARDWARE_MODULE_ID,
    .name = "Spreadtrum Display Processor Unit(DPU)",
    .author = "Spreadtrum Communications, Inc",
    .methods = &dpu_module_methods,
    .dso = 0,
    .reserved = {0},
  }
};
