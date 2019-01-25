/* Copyright (c) 2015, The Linux Foundataion. All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are
 met:
     * Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
     * Redistributions in binary form must reproduce the above
       copyright notice, this list of conditions and the following
*       disclaimer in the documentation and/or other materials provided
*       with the distribution.
*     * Neither the name of The Linux Foundation nor the names of its
*       contributors may be used to endorse or promote products derived
*       from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
* ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF *
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
* BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
* OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
* IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/

#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <linux/media.h>
#include <sys/types.h>
#include <sys/stat.h>

//#include <media/msmb_camera.h>
//#include <media/msm_cam_sensor.h>
#include <utils/Log.h>

#include "SprdCamera3Flash.h"

#include "cmr_common.h"
#define LOGE CMR_LOGE
#define LOGV CMR_LOGV

#define STRING_LENGTH 15

volatile uint32_t gCamHal3LogLevel = 1;

namespace sprdcamera {
SprdCamera3Flash *SprdCamera3Flash::_instance = 0;
/*===========================================================================
* FUNCTION    : getInstance
* DESCRIPTION : Get and create the SprdCameraFlash singleton.
* PARAMETERS  : None
* RETURN      : Instance of SprdCameraFlash class
*==========================================================================*/
SprdCamera3Flash *SprdCamera3Flash::getInstance() {
    if (!_instance) {
        _instance = new SprdCamera3Flash();
    }
    return _instance;
}
/*===========================================================================
* FUNCTION    : SprdCameraFlash
* DESCRIPTION : default constructor of SprdCameraFlash
* PARAMETERS  : None
* RETURN      : None
*==========================================================================*/
SprdCamera3Flash::SprdCamera3Flash() : m_callbacks(NULL) {
    LOGV("%s : In", __func__);
    memset(&m_flashOn, 0, sizeof(m_flashOn));
    memset(&m_cameraOpen, 0, sizeof(m_cameraOpen));
}
/*===========================================================================
* FUNCTION    : ~SprdCameraFlash
* DESCRIPTION : deconstructor of SprdCameraFlash
* PARAMETERS  : None
* RETURN      : None
*==========================================================================*/
SprdCamera3Flash::~SprdCamera3Flash() {
    // Destructor
}
/*===========================================================================
* FUNCTION    : registerCallbacks
* DESCRIPTION : reference to callbacks to framework
* PARAMETERS  :
*		callbacks : Framework's function pointers to update flash
* RETURN      : None
*==========================================================================*/
int32_t SprdCamera3Flash::registerCallbacks(
    const camera_module_callbacks_t *callbacks) {
    LOGV("%s : In", __func__);
    int32_t retVal = 0;
    if (!_instance)
        _instance = new SprdCamera3Flash();
    _instance->m_callbacks = callbacks;

    return retVal;
}
/*===========================================================================
* FUNCTION    : setFlashMode
* DESCRIPTION : Turn on or off the flash associated with a given handle.
* PARAMETERS  :
*   camera_id : Camera id of the flash
*   mode      : Whether to turn flash on (true) or off (false)
* RETURN      : 0 -- success
*==========================================================================*/
int32_t SprdCamera3Flash::setFlashMode(const int camera_id, const bool mode) {
    int32_t retVal = 0;
    int32_t bytes = 0;
    char buffer[16];
    uint8_t default_light_level = 5;
    const char *const flashInterface =
        "/sys/devices/virtual/misc/sprd_flash/test";
    ssize_t wr_ret;
    LOGV("open flash driver interface");
    int fd = open(flashInterface, O_WRONLY);
    /* open sysfs file parition */
    if (-1 == fd) {
        LOGE("Failed to open: flash_light_interface, %s", flashInterface);
        return -EINVAL;
    }
#ifdef CAMERA_TORCH_LIGHT_LEVEL
    default_light_level = CAMERA_TORCH_LIGHT_LEVEL;
#endif
    m_flashOn[0] = mode ? SPRD_FLASH_STATUS_ON : SPRD_FLASH_STATUS_OFF;
    if (mode) {
        bytes = snprintf(buffer, sizeof(buffer), "0x%x",
                         (default_light_level << 8) | 0x0016);
        wr_ret = write(fd, buffer, bytes);
    }
    bytes = snprintf(buffer, sizeof(buffer), "0x%x", 0x11 ^ mode);
    wr_ret = write(fd, buffer, bytes);

    if (-1 == wr_ret) {
        LOGE("WRITE FAILED \n");
        retVal = -EINVAL;
    }

    close(fd);
    LOGV("Close file");
    return retVal;
}
/*===========================================================================
* FUNCTION    : set_torch_mode
* DESCRIPTION : turn on or off the torch mode of the flash unit.
* PARAMETERS  :
*   camera_id : camera ID
*   on        : Indicates whether to turn the flash on or off
* RETURN      : 0  -- success
*             none-zero failure code
*==========================================================================*/
int32_t SprdCamera3Flash::set_torch_mode(const char *cameraIdStr, bool on) {
    LOGV("%s : cameraId:%s", __func__, cameraIdStr);
    int retVal = 0;
    char *cmd_str;
    if (on) {
        setFlashMode((int)(*cameraIdStr), on);
        m_callbacks->torch_mode_status_change(m_callbacks, cameraIdStr,
                                              TORCH_MODE_STATUS_AVAILABLE_ON);
    } else {
        setFlashMode((int)(*cameraIdStr), on);
        m_callbacks->torch_mode_status_change(m_callbacks, cameraIdStr,
                                              TORCH_MODE_STATUS_AVAILABLE_OFF);
    }
    return retVal;
}

/*===========================================================================
* FUNCTION    : reserveFlashForCamera
* DESCRIPTION : Give control of the flash to the camera, and notify
*              framework that the flash has become unavailable.
* PARAMETERS  :
*   camera_id : Camera id of the flash.
* RETURN      : 0 -- success
*             non-zero -- No camera present at camera_id or not inited. OR
*             No callback available for torch_mode_status_change.
*==========================================================================*/
int32_t SprdCamera3Flash::reserveFlashForCamera(const int cameraId) {
    int retVal = 0;
    LOGV("%s : cameraId = %d", __func__, cameraId);
    if (cameraId < 0 || cameraId >= SPRD_CAMERA_MAX_NUM_SENSORS) {
        LOGE("%s: Invalid camera id: %d", __func__, cameraId);
        retVal = -EINVAL;
        goto exit;
    }
    if (m_callbacks == NULL || m_callbacks->torch_mode_status_change == NULL) {
        LOGE("%s: Callback is not defined!", __func__);
        retVal = -EINVAL;
        goto exit;
    }
    char cameraIdStr[STRING_LENGTH];
    snprintf(cameraIdStr, STRING_LENGTH, "%d", cameraId);
    if (m_cameraOpen[cameraId]) {
        LOGV("FLash already reserved for camera ");
    } else {
        if (!cameraId && m_flashOn[0]) {
            set_torch_mode(cameraIdStr, SPRD_FLASH_STATUS_OFF);
        }
        m_cameraOpen[cameraId] = true;
        m_callbacks->torch_mode_status_change(m_callbacks, cameraIdStr,
                                              TORCH_MODE_STATUS_NOT_AVAILABLE);
    }
exit:
    return retVal;
}
/*===========================================================================
* FUNCTION    : releaseFlashFromCamera
* DESCRIPTION : Release control of the flash from the camera, and notify
*     framework that the flash has become available.
* PARAMETERS  :
    camera_id : Camera id of the flash.
* RETURN      : 0 -- success
*       non-zero --  No camera present at camera_id or not inited.
*       No callback available for torch_mode_status_change.
*==========================================================================*/
int32_t SprdCamera3Flash::releaseFlashFromCamera(const int cameraId) {
    int retVal = 0;
    LOGV("%s : cameraId = %d", __func__, cameraId);

    if (cameraId < 0 || cameraId >= SPRD_CAMERA_MAX_NUM_SENSORS) {
        LOGE("%s: Invalid camera id: %d", __func__, cameraId);
        retVal = -EINVAL;
        goto exit;
    }
    if (m_callbacks == NULL) {
        LOGE("%s: Callback is not defined!", __func__);
        retVal = -EINVAL;
        goto exit;
    }
    char cameraIdStr[STRING_LENGTH];
    snprintf(cameraIdStr, STRING_LENGTH, "%d", cameraId);
    if (!m_cameraOpen[cameraId]) {
        LOGV(" Flash unit is already released");
    } else {
        m_cameraOpen[cameraId] = false;
#ifndef ANDROID_VERSION_KK
        m_callbacks->torch_mode_status_change(m_callbacks, cameraIdStr,
                                              TORCH_MODE_STATUS_AVAILABLE_OFF);
#endif
    }
exit:
    return retVal;
}

int32_t SprdCamera3Flash::reserveFlashForCameraForKK(const int cameraId) {
    int32_t retVal = 0;
    int32_t bytes = 0;
    char buffer[16];
    const char *const flashInterface =
        "/sys/devices/virtual/misc/sprd_flash/test";
    ssize_t wr_ret;
    LOGV("open flash driver interface");

    if (m_cameraOpen[cameraId]) {
        LOGV("FLash already reserved for camera ");
    } else {
        m_cameraOpen[cameraId] = true;
        if (cameraId == 0) {
            int fd = open(flashInterface, O_WRONLY);
            /* open sysfs file parition */
            if (-1 == fd) {
                LOGE("Failed to open: flash_light_interface, %s", flashInterface);
                return -EINVAL;
            }
            bytes = snprintf(buffer, sizeof(buffer), "0x%x", 0x11);
            wr_ret = write(fd, buffer, bytes);

            if (-1 == wr_ret) {
                LOGE("WRITE FAILED \n");
                retVal = -EINVAL;
            }

            close(fd);
            LOGV("Close file");
        }
    }
    return retVal;

}

int32_t SprdCamera3Flash::setTorchMode(const char *cameraIdStr, bool on) {
    int retVal = 0;
    if (_instance)
        retVal = _instance->set_torch_mode(cameraIdStr, on);
    return retVal;
}
int32_t SprdCamera3Flash::reserveFlash(const int cameraId) {
    int retVal = 0;
    if (_instance) {
#ifndef ANDROID_VERSION_KK
        retVal = _instance->reserveFlashForCamera(cameraId);
#else
        retVal = _instance->reserveFlashForCameraForKK(cameraId);
#endif
    }
    return retVal;
}
int32_t SprdCamera3Flash::releaseFlash(const int cameraId) {
    int retVal = 0;
    if (_instance)
        retVal = _instance->releaseFlashFromCamera(cameraId);
    return retVal;
}

}; // namespace sprdcamera
