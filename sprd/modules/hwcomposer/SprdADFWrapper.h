/*
 * Copyright (C) 2010 The Android Open Source Project
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
 *  SprdADFWrapper:: surpport Android ADF.
 *  It pass display data from HWC to Android ADF
 *  Author: zhongjun.chen@spreadtrum.com
*/

#ifndef _SPRD_ADF_WRAPPER_H
#define _SPRD_ADF_WRAPPER_H

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <hardware/hwcomposer.h>

#include <adfhwc/adfhwc.h>
#include <adf/adf.h>
#include "sprd_adf.h"

/* for our custom validate ioctl */
#include <video/adf.h>

#include "SprdDisplayCore.h"
#include "SprdHWLayer.h"
#include "SprdDisplayDevice.h"

using namespace android;

#ifndef ALIGN
#define ALIGN(value, base) (((value) + ((base)-1)) & ~((base)-1))
#endif

class SprdADFWrapper : public SprdDisplayCore {
 public:
  SprdADFWrapper();
  ~SprdADFWrapper();

  /*
   *  Init func: open ADF device, and get some configs.
   * */
  virtual bool Init();

  virtual int AddFlushData(int DisplayType, SprdHWLayer **list, int LayerCount);

  virtual int PostDisplay(DisplayTrack *tracker);

  virtual int QueryDisplayInfo(uint32_t *DisplayNum);

  virtual int GetConfigs(int DisplayType, uint32_t *Configs,
                         size_t *NumConfigs);

  virtual int GetConfigAttributes(int DisplayType, uint32_t Config,
                                  const uint32_t *attributes, int32_t *values);

  virtual int EventControl(int DisplayType, int event, bool enabled);

  virtual int Blank(int DisplayType, bool enabled);

  virtual int Dump(char *buffer);

 private:
  typedef struct {
    /* File descriptor for ioctls on this overlay engine */
    int Fd;

    /* Overlay engine data (supported pixel formats) */
    struct adf_overlay_engine_data Data;
  } ADF_overlay_engine;

  typedef struct {
    /* File descriptor for ioctls on this interface */
    int Fd;

    /* List of overlay engine IDs */
    adf_id_t *EngineIDs;

    /* List of overlay engines */
    ADF_overlay_engine *Engines;

    /* Number of overlay engines */
    ssize_t numEngines;

    /* Interface data. Some data may change (power/hotplug state, current mode)
     * so should not be used without refreshing this */
    struct adf_interface_data Data;
  } ADF_interface;

  typedef struct {
    int LayerCount;
    SprdHWLayer **LayerList;
    int32_t DisplayType;
    bool Active;
    adf_id_t OverlayEngineId;
    adf_id_t InterfaceId;
    ADF_interface *Interface;
    void *user;
  } FlushContext;

  int32_t mNumInterfaces;
  FlushContext mFlushContext[DEFAULT_DISPLAY_TYPE_NUM];
  adf_id_t *mInterfaceIDs; /* List of interface IDs  */
  struct adf_device *mDevice;
  struct adf_hwc_helper *mHelper;
  ADF_interface *mInterfaces; /* List of interface  */
  int mDebugFlag;

  int createADFInterface();
  void freeADFInterface();
  void deInit();

  int attachOverlayEngineToInterface(FlushContext *ctx,
                                     adf_id_t *overlayEngineId);
  void detachOverlayEngineFromInterfaces(
      struct adf_buffer_config **bufferConfig);

  int implementBufferConfig(SprdHWLayer *l, adf_id_t overlayEngineId,
                            struct adf_buffer_config *bufferConfig);

  void implementCustomConfig(FlushContext *ctx, SprdHWLayer *l,
                             struct sprd_adf_hwlayer_custom_data *adfLayer,
                             int32_t index);

  ADF_interface *getInterface(int displayType);
  adf_id_t getInterfaceId(int displayType);

  static void DestroyADFSignalHandler(int Signal, siginfo_t *SigInfo,
                                      void *user);

  void invalidateFlushContext();

  inline FlushContext *getFlushContext(int displayType) {
    if (displayType >= 0) {
      return &(mFlushContext[displayType]);
    } else {
      return NULL;
    }
  }

  inline int convertToADFBlendMode(int androidBlend) { return androidBlend; }

  inline int convertToADFTransform(int androidTransform) {
    return androidTransform;
  }
};

#endif  // #ifndef _SPRD_ADF_WRAPPER_H
