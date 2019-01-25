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

#include <fcntl.h>
#include <hardware/hardware.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "SprdHWLayer.h"
#include "DpuDevice.h"

namespace android {

DpuDevice::DpuDevice():dev_id(0), intf_id(0), eng_id(0), intf(-1), eng(-1),
	mCapability(NULL) {
	memset(&dev, 0, sizeof(adf_device));
	memset(&engine_data, 0, sizeof(struct adf_overlay_engine_data));
}

DpuDevice::~DpuDevice() {

}
bool checkRGBLayerFormat(SprdHWLayer *l) {
    hwc_layer_1_t *layer = l->getAndroidLayer();
    if (layer == NULL)
    {
        return false;
    }

    const native_handle_t *pNativeHandle = layer->handle;

    if (pNativeHandle == NULL)
    {
        return false;
    }

    switch (ADP_FORMAT(pNativeHandle)) {
	case HAL_PIXEL_FORMAT_RGBA_8888:
	case HAL_PIXEL_FORMAT_RGBX_8888:
	case HAL_PIXEL_FORMAT_RGB_888:
	case HAL_PIXEL_FORMAT_RGB_565:
	case HAL_PIXEL_FORMAT_BGRA_8888:
		return true;
	default:
	    break;
	}
    return false;
}

bool checkAFBCBlockSize(SprdHWLayer *l) {
  hwc_layer_1_t *layer = l->getAndroidLayer();
  if (layer == NULL) {
    return false;
  }

  const native_handle_t *pNativeHandle = layer->handle;

  if (pNativeHandle == NULL) {
    return false;
  }

  if (ADP_COMPRESSED(pNativeHandle)) {
    if (ADP_FORMAT(pNativeHandle) == HAL_PIXEL_FORMAT_RGB_565)
      return false;
    if (((ADP_STRIDE(pNativeHandle) % 16) == 0) &&
        ((ADP_VSTRIDE(pNativeHandle) % 16) == 0)) {
      return true;
    } else {
      ALOGW("Dpu checked invalid handle size under AFBC: pitch: %d, vstride:%d",
            ADP_STRIDE(pNativeHandle), ADP_VSTRIDE(pNativeHandle));
      return false;
    }
  }
  return true;
}

int DpuDevice::prepare(SprdHWLayer **list, int count, bool *support) {
	SprdHWLayer *l;
	bool alpha_issue = false;

	if (count > (int)mCapability->number_hwlayer) {
		ALOGW("Warning: Dispc cannot support %d layer blending(%d)", count, *support);
		return -1;
	}

	for (int i = 0; i < count; i++) {
		l = list[i];

		/*sharkl2 dpu cannot handle alpha plane*/
		if (l->getPlaneAlpha() != 0xff) {
			alpha_issue = true;
			break;
		}
		if (!checkRGBLayerFormat(l))
			continue;
		if (l->getTransform() != 0)
			continue;
		if ((l->getSprdSRCRect()->w != l->getSprdFBRect()->w)
				|| (l->getSprdSRCRect()->h != l->getSprdFBRect()->h))
			continue;
		if (!checkAFBCBlockSize(l))
			continue;
		l->setLayerAccelerator(ACCELERATOR_DISPC);
	}

	if (alpha_issue) {
		for (int i = 0; i < count; i++) {
			l = list[i];
			l->setLayerAccelerator(ACCELERATOR_NON);
		}
	}

	return 0;
}
int DpuDevice::init(bool *support) {
	int dev_id = 0;
	const __u32 fmt8888[] = { DRM_FORMAT_ARGB8888, };
	const size_t n_fmt8888 = sizeof(fmt8888) / sizeof(fmt8888[0]);

	int err = adf_device_open(dev_id, O_RDWR, &dev);
	if (err) {
		ALOGE("DpuDevice::init adf_device_open fail");
		return -1;
	}
	err = adf_find_simple_post_configuration(&dev, fmt8888, n_fmt8888, &intf_id,
			&eng_id);
	if (err) {
		ALOGE("DpuDevice::init simple post configuration err");
		goto ERR0;
	}
	intf = adf_interface_open(&dev, intf_id, O_RDWR);
	if (intf < 0) {
		ALOGE("DpuDevice::init adf_interface_open err");
		goto ERR0;
	}
	eng = adf_overlay_engine_open(&dev, eng_id, O_RDWR);
	if (eng < 0) {
		ALOGE("DpuDevice::init adf_overlay_engine_open err");
		goto ERR0;
	}
	err = adf_get_overlay_engine_data(eng, &engine_data);
	if (err) {
		ALOGE("DpuDevice::init adf_get_overlay_engine_data err");
		goto ERR0;
	}
	mCapability =(struct sprd_adf_overlayengine_capability *)(engine_data.custom_data);
	ALOGD("Got dpu hwlayer number: %d", mCapability->number_hwlayer);
	if (mCapability->number_hwlayer > 2)
		*support = true;
	else
		*support = false;

ERR0:
	adf_device_close(&dev);
	return 0;
}
} // namespace android
