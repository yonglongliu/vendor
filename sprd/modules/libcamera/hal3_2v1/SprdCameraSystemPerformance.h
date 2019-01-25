/* Copyright (c) 2017, The Linux Foundataion. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
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
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#ifndef SPRDCAMERA3SYSTEMPERFORMACEH_HEADER
#define SPRDCAMERA3SYSTEMPERFORMACEH_HEADER

#include <stdlib.h>
#include <dlfcn.h>
#include <utils/Log.h>
#include <utils/Errors.h>
#include <utils/List.h>
#include <utils/Mutex.h>
#include <cutils/properties.h>
#include <sys/mman.h>
#include <cutils/sockets.h>
#include <sys/socket.h>
#include "SprdCamera3HALHeader.h"

#define ANDROID_VERSION_O (801)
#define ANDROID_VERSION_N (701)

#if (CONFIG_HAS_CAMERA_HINTS_VERSION == ANDROID_VERSION_N)
#include <binder/BinderService.h>
#include <binder/IInterface.h>
#include <powermanager/IPowerManager.h>
#include <powermanager/PowerManager.h>
#include <hardware/power.h>
#elif(CONFIG_HAS_CAMERA_HINTS_VERSION == ANDROID_VERSION_O)
#include <hardware/power.h>
#include <vendor/sprd/hardware/power/2.0/IPower.h>
#include <vendor/sprd/hardware/power/2.0/types.h>
using ::vendor::sprd::hardware::power::V2_0::IPower;
using ::android::hidl::base::V1_0::IBase;
using ::vendor::sprd::hardware::power::V2_0::PowerHint;
#endif

/*CONFIG_HAS_CAMERA_HINTS_VERSION
* 701: 7.0 isharkl2
* 801: 8.0 isharkl2
*/

namespace sprdcamera {

typedef enum CURRENT_POWER_HINT {
    CAM_POWER_NORMAL,
    CAM_POWER_PERFORMACE_ON,
    CAM_POWER_LOWPOWER_ON
} power_hint_state_type_t;

typedef enum DFS_POLICY {
    CAM_EXIT,
    CAM_LOW,
    CAM_NORMAL,
    CAM_VERYHIGH,
} dfs_policy_t;
typedef enum CAMERA_PERFORMACE_SCENE {
    CAM_OPEN_S,
    CAM_OPEN_E_LEVEL_H,     // DFS:veryhigh
    CAM_OPEN_E_LEVEL_N,     // DFS:normal
    CAM_OPEN_E_LEVEL_L,     // DFS:low
    CAM_PREVIEW_S_LEVEL_H,  // powerhint:performance
    CAM_PREVIEW_S_LEVEL_N,  // powerhint:normal
    CAM_PREVIEW_S_LEVEL_L,  // powerhint:low
    CAM_CAPTURE_S_LEVEL_HH, // powerhint:performance  DFS:veryhigh
    CAM_CAPTURE_S_LEVEL_HN, // powerhint:performance  DFS:normal
    CAM_CAPTURE_S_LEVEL_NH, // powerhint:normal  DFS:veryhigh
    CAM_CAPTURE_S_LEVEL_NN, // powerhint:normal  DFS:normal
    CAM_CAPTURE_E_LEVEL_NH, // powerhint:normal  DFS:veryhigh
    CAM_CAPTURE_E_LEVEL_NN, // powerhint:normal  DFS:normal
    CAM_CAPTURE_E_LEVEL_NL, // powerhint:normal  DFS:low
    CAM_CAPTURE_E_LEVEL_LN, // powerhint:low DFS:normal
    CAM_CAPTURE_E_LEVEL_LL, // powerhint:low  DFS:low
    CAM_CAPTURE_E_LEVEL_LH, // powerhint:low  DFS:veryhigh
    CAM_FLUSH_S,
    CAM_FLUSH_E,
    CAM_EXIT_S,
    CAM_EXIT_E,
} sys_performance_camera_scene;

class SprdCameraSystemPerformance {
  public:
    static void getSysPerformance(SprdCameraSystemPerformance **pmCamSysPer);
    static void freeSysPerformance(SprdCameraSystemPerformance **pgCamSysPer);
    void setCamPreformaceScene(sys_performance_camera_scene camera_scene,
                               int camera_id);

  private:
    SprdCameraSystemPerformance();
    ~SprdCameraSystemPerformance();
    void setPowerHint(power_hint_state_type_t powerhint_id);
    int changeDfsPolicy(dfs_policy_t dfs_policy);
    void initPowerHint();
    void deinitPowerHint();

    int setDfsPolicy(int dfs_policy);
    int releaseDfsPolicy(int dfs_policy);
    static int mCameraSessionActive;
    int mCameraDfsPolicyCur;
    int mCurrentPowerHint;
    bool mPowermanageInited;
    sys_performance_camera_scene mCurSence[2]; // 1.main camera  2.sub camera.

#if (CONFIG_HAS_CAMERA_HINTS_VERSION == ANDROID_VERSION_O)
    void acquirePowerHint(sp<IPower> powermanager, PowerHint id);
    void releasePowerHint(sp<IPower> powermanager, PowerHint id);

    sp<IPower> mPowerManager;
    sp<IPower> mPowerManagerLowPower;
    sp<IBase> mPrfmLock;
    sp<IBase> mPrfmLockLowPower;
#elif(CONFIG_HAS_CAMERA_HINTS_VERSION == ANDROID_VERSION_N)
    void thermalEnabled(bool flag);

    void enablePowerHintExt(sp<IPowerManager> powermanager,
                            sp<IBinder> prfmlock, int powerhint_id);

    void disablePowerHintExt(sp<IPowerManager> powermanager,
                             sp<IBinder> prfmlock);

    sp<IPowerManager> mPowerManager;
    sp<IPowerManager> mPowerManagerLowPower;
    sp<IBinder> mPrfmLock;
    sp<IBinder> mPrfmLockLowPower;
#endif
    Mutex mLock;
    static Mutex sLock;
};
}
#endif /* SPRDCAMERAMU*/
