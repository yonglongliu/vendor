/* Copyright (c) 2012-2013, The Linux Foundataion. All rights reserved.
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

#ifndef _ARCSOFT_CONFIGURABLE_PARA_H_
#define _ARCSOFT_CONFIGURABLE_PARA_H_

#include "arcsoft_dualcam_image_refocus.h"

struct arcsoft_configurable_para {
    MInt32 lFocusMode;
    /* blur intensity ratio between capture and preview */
    MFloat blurCapPrevRatio;
    MFloat fMaxFOV;
    MInt32 a_dInCamLayout;
    MFloat leftDis[11];
    MFloat rightDis[11];
};

static struct arcsoft_configurable_para arcsoft_config_para = {
    .lFocusMode = ARC_DCIR_NORMAL_MODE,
    .blurCapPrevRatio = 0.6f,
#ifdef CONFIG_ALTEK_ZTE_CALI
    .fMaxFOV = 83.0f,
#else
    .fMaxFOV = 85.0f,
#endif
#ifdef CONFIG_ALTEK_ZTE_CALI
    .a_dInCamLayout = 3,
#else
    .a_dInCamLayout = 0,
#endif
#ifndef CONFIG_ALTEK_ZTE_CALI
    .leftDis = {0.00, 0.09, 0.57, 1.12, 1.40, 1.52, 1.51, 1.45, 1.33, 1.55,
                1.51},
    .rightDis = {0.00, 0.09, 0.57, 1.12, 1.40, 1.52, 1.51, 1.45, 1.33, 1.55,
                 1.51},
#else
    .leftDis = {0},
    .rightDis = {0},
#endif
};
#endif
