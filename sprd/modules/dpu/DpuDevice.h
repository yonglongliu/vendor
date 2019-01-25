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

#ifndef DpuDevice_H_
#define DpuDevice_H_

#include "SprdHWLayer.h"
#include "SprdUtil.h"
#include <adf/adf.h>
#include <sprd_adf.h>


#define DPU_HARDWARE_MODULE_ID "dpu"

namespace android {

typedef struct dpu_device {
	struct hw_device_t common;

	int (*prepare)(struct dpu_device *device, SprdHWLayer **list, int count,
			bool *support);
	int (*init)(struct dpu_device *device, bool *support);
} dpu_device_t;

class DpuDevice: public dpu_device_t {
public:
	DpuDevice();
	~DpuDevice();

	int prepare(SprdHWLayer **list, int count, bool *support);
	int init(bool *support);

private:
	adf_id_t dev_id;
	adf_id_t intf_id;
	adf_id_t eng_id;
	adf_device dev;
	int intf;
	int eng;
	struct adf_overlay_engine_data engine_data;
	struct sprd_adf_overlayengine_capability *mCapability;
};

}  // namespace android
#endif  // DpuDevice_H_
