/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "SPRD_OMX_CORE"
#include <utils/Log.h>

#include "sprd_omx_core.h"
#include "SprdOMXComponent.h"

#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/AString.h>
#include <media/stagefright/foundation/ABase.h>
#include <pthread.h>
#include <dlfcn.h>

using namespace android;

// created components array
static OMXCore* g_OMX_Core = NULL;
static bool g_bInitialized = false;
// mfx OMX IL Core thread safety
static pthread_mutex_t g_OMXCoreLock = PTHREAD_MUTEX_INITIALIZER;


/*------------------------------------------------------------------------------*/

#define SPRD_OMX_FREE(_ptr) \
    { if (_ptr) { free(_ptr); (_ptr) = NULL; } }

#define SPRD_OMX_CORE_LOCK()  int res_lock = pthread_mutex_lock(&g_OMXCoreLock); \
    if (res_lock) return OMX_ErrorUndefined;

#define SPRD_OMX_CORE_UNLOCK()  int res_unlock = pthread_mutex_unlock(&g_OMXCoreLock); \
    if (res_unlock) return OMX_ErrorUndefined;

#define SPRD_OMX_LOG(msg)       ALOGV("%s: %s"        ,__FUNCTION__, msg)
#define SPRD_OMX_LOG_D(msg,value) ALOGV("%s: %s = %d"   ,__FUNCTION__, msg, value)
#define SPRD_OMX_LOG_S(msg,value) ALOGV("%s: %s = %s"   ,__FUNCTION__, msg, value)

static const struct {
    const char *mName;
    const char *mLibNameSuffix;
    const char *mRole;

} kComponents[] = {
    { "OMX.sprd.h263.decoder", "sprd_mpeg4dec", "video_decoder.h263" },
    { "OMX.sprd.mpeg4.decoder", "sprd_mpeg4dec", "video_decoder.mpeg4" },
    { "OMX.sprd.h264.decoder", "sprd_h264dec", "video_decoder.avc" },
    { "OMX.sprd.mp3.decoder", "sprd_mp3dec", "audio_decoder.mp3" },
    { "OMX.sprd.mp3l1.decoder", "sprd_mp3dec", "audio_decoder.mp1" },
    { "OMX.sprd.mp3l2.decoder", "sprd_mp3dec", "audio_decoder.mp2" },
    { "OMX.sprd.mp3.encoder", "sprd_mp3enc", "audio_encoder.mp3" },
    { "OMX.sprd.h264.encoder", "sprd_h264enc", "video_encoder.avc" },
    { "OMX.google.mjpg.decoder", "soft_mjpgdec", "video_decoder.mjpg" },
    { "OMX.google.imaadpcm.decoder", "soft_imaadpcmdec", "audio_decoder.imaadpcm" },
#ifdef PLATFORM_SHARKL2
    { "OMX.sprd.vpx.decoder", "sprd_vpxdec", "video_decoder.vpx" },
    { "OMX.sprd.mpeg4.encoder", "sprd_mpeg4enc", "video_encoder.mpeg4" },
    { "OMX.sprd.h263.encoder", "sprd_mpeg4enc", "video_encoder.h263" },
#endif
};

static const size_t kNumComponents =
    sizeof(kComponents) / sizeof(kComponents[0]);


/*------------------------------------------------------------------------------*/

OMX_API OMX_ERRORTYPE OMX_APIENTRY OMX_Init(void)
{
    SPRD_OMX_LOG("+");
    SPRD_OMX_CORE_LOCK();
    OMX_ERRORTYPE omx_res = OMX_ErrorNone;

    SPRD_OMX_LOG_D("g_bInitialized",g_bInitialized);

    if (!g_bInitialized)
    {
        g_OMX_Core = new OMXCore();
        g_bInitialized = true;
    }
    g_OMX_Core->m_OMXCoreRefCount++;

    SPRD_OMX_CORE_UNLOCK();
    SPRD_OMX_LOG("-");
    return OMX_ErrorNone;
}

/*------------------------------------------------------------------------------*/

OMX_API OMX_ERRORTYPE OMX_APIENTRY OMX_Deinit(void)
{
    SPRD_OMX_CORE_LOCK();

    SPRD_OMX_LOG_D("g_bInitialized", g_bInitialized);
    if (g_bInitialized)
    {
        --g_OMX_Core->m_OMXCoreRefCount;
        SPRD_OMX_LOG_D("g_OMXCoreRefCount", g_OMX_Core->m_OMXCoreRefCount);

        if(0 == g_OMX_Core->m_OMXCoreRefCount)
        {
            delete g_OMX_Core;
            g_bInitialized = false;
        }
    }

    SPRD_OMX_LOG_D("g_bInitialized", g_bInitialized);
    SPRD_OMX_CORE_UNLOCK();
    return OMX_ErrorNone;
}

OMX_API OMX_ERRORTYPE OMX_APIENTRY OMX_GetHandle(
    OMX_OUT OMX_HANDLETYPE* pHandle,
    OMX_IN  OMX_STRING cComponentName,
    OMX_IN  OMX_PTR pAppData,
    OMX_IN  OMX_CALLBACKTYPE* pCallBacks) {
    SPRD_OMX_LOG("+");
    SPRD_OMX_CORE_LOCK();
    SPRD_OMX_LOG_S("makeComponentInstance", cComponentName);

    for (size_t i = 0; i < kNumComponents; ++i) {
        if (strcmp(cComponentName, kComponents[i].mName)) {
            continue;
        }

        AString libName = "libstagefright_";
        libName.append(kComponents[i].mLibNameSuffix);
        libName.append(".so");

        void *libHandle = dlopen(libName.c_str(), RTLD_NOW);

        if (libHandle == NULL) {
            ALOGE("unable to dlopen %s: %s", libName.c_str(), dlerror());
            SPRD_OMX_CORE_UNLOCK();
            return OMX_ErrorComponentNotFound;
        }

        typedef SprdOMXComponent *(*CreateSprdOMXComponentFunc)(
                const char *, const OMX_CALLBACKTYPE *,
                OMX_PTR, OMX_COMPONENTTYPE **);

        CreateSprdOMXComponentFunc createSprdOMXComponent =
            (CreateSprdOMXComponentFunc)dlsym(
                    libHandle,
                    "_Z22createSprdOMXComponentPKcPK16OMX_CALLBACKTYPE"
                    "PvPP17OMX_COMPONENTTYPE");

        if (createSprdOMXComponent == NULL) {
            dlclose(libHandle);
            libHandle = NULL;
            SPRD_OMX_LOG("createSoftOMXComponent == NULL");
            SPRD_OMX_LOG("-");
            SPRD_OMX_CORE_UNLOCK();
            return OMX_ErrorComponentNotFound;
        }

        sp<SprdOMXComponent> codec =
            (*createSprdOMXComponent)(cComponentName, pCallBacks, pAppData, reinterpret_cast<OMX_COMPONENTTYPE **>(pHandle));

        if (codec == NULL) {
            dlclose(libHandle);
            libHandle = NULL;
            SPRD_OMX_LOG("createSoftOMXComponent == NULL");
            SPRD_OMX_LOG("-");
            SPRD_OMX_CORE_UNLOCK();
            return OMX_ErrorInsufficientResources;
        }

        OMX_ERRORTYPE err = codec->initCheck();
        if (err != OMX_ErrorNone) {
            SPRD_OMX_LOG("codec->initCheck() returned an error");
            dlclose(libHandle);
            libHandle = NULL;

            SPRD_OMX_LOG("-");
            SPRD_OMX_CORE_UNLOCK();
            return err;
        }
        SPRD_OMX_LOG("realloc");

        codec->incStrong(g_OMX_Core);
        codec->setLibHandle(libHandle);
        ALOGI("Created OMXPlugin : %s", kComponents[i].mName);
        SPRD_OMX_LOG("-");
        SPRD_OMX_CORE_UNLOCK();
        return OMX_ErrorNone;
    }
    SPRD_OMX_CORE_UNLOCK();
    return OMX_ErrorInvalidComponentName;
}

OMX_API OMX_ERRORTYPE OMX_APIENTRY OMX_FreeHandle(
    OMX_IN  OMX_HANDLETYPE hComponent)
 {
    SPRD_OMX_LOG("+");
    SPRD_OMX_CORE_LOCK();
    SprdOMXComponent* omx_component =
       (SprdOMXComponent*)(((OMX_COMPONENTTYPE *)hComponent)->pComponentPrivate);

    omx_component->prepareForDestruction();
    void *libHandle = omx_component->libHandle();
    CHECK_EQ(omx_component->getStrongCount(), 1);

    omx_component->decStrong(g_OMX_Core);
    omx_component = NULL;

    dlclose(libHandle);

    SPRD_OMX_LOG("-");
    SPRD_OMX_CORE_UNLOCK();
    return OMX_ErrorNone;
}

OMX_API OMX_ERRORTYPE OMX_APIENTRY OMX_ComponentNameEnum(
    OMX_OUT OMX_STRING cComponentName,
    OMX_IN  OMX_U32 nNameLength,
    OMX_IN  OMX_U32 nIndex) {
    if (nIndex >= kNumComponents) {
        return OMX_ErrorNoMore;
    }

    strcpy(cComponentName, kComponents[nIndex].mName);

    return OMX_ErrorNone;
}

OMX_API OMX_ERRORTYPE OMX_GetRolesOfComponent(
    OMX_IN      OMX_STRING compName,
    OMX_INOUT   OMX_U32 *pNumRoles,
    OMX_OUT     OMX_U8 **roles) {
    SPRD_OMX_LOG("+");
    if (!pNumRoles) {
        return OMX_ErrorBadParameter;
    }
    else {
        for (size_t i = 0; i < kNumComponents; ++i) {
            if (strcmp(compName, kComponents[i].mName)) {
                continue;
            }

            if (roles) strcpy((char*)roles[0],kComponents[i].mRole);
            (*pNumRoles) = 1;
            SPRD_OMX_LOG("-");
            return OMX_ErrorNone;
        }
    }
    SPRD_OMX_LOG("-");
    return OMX_ErrorInvalidComponentName;
}

OMX_API OMX_ERRORTYPE OMX_GetComponentsOfRole(
    OMX_IN      OMX_STRING role,
    OMX_INOUT   OMX_U32 *pNumComps,
    OMX_INOUT   OMX_U8  **compNames)
{
    SPRD_OMX_LOG("+");
    OMX_ERRORTYPE omx_res = OMX_ErrorNone;
    size_t component_index = 0, role_index = 0;

    // errors checking
    if (!role)
    {
        omx_res = OMX_ErrorBadParameter;
    }

    if ((OMX_ErrorNone == omx_res) && !pNumComps)
    {
        omx_res = OMX_ErrorBadParameter;
    }

    // getting components os requested role
    if ((OMX_ErrorNone == omx_res) && g_bInitialized)
    {
        // searching for the component in the registry
        if (OMX_ErrorNone == omx_res)
        {
            OMX_U32 num_comps = *pNumComps;

            *pNumComps = 0;
            for (component_index = 0; component_index < kNumComponents; ++component_index)
            {
                if (!strcmp(role, kComponents[component_index].mRole))
                {
                    if (compNames)
                    {
                        if (num_comps <= *pNumComps)
                            omx_res = OMX_ErrorBadParameter;
                        else
                            strcpy((char*)compNames[*pNumComps], kComponents[component_index].mName);
                    }
                    ++(*pNumComps);
                    break;
                }
                if (OMX_ErrorNone != omx_res) break;
            }
        }
    }

    SPRD_OMX_LOG("-");
    return omx_res;
}
