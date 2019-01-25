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

#ifndef _SPRD_DEPTH_CONFIGURABLE_PARA_H_
#define _SPRD_DEPTH_CONFIGURABLE_PARA_H_

struct sprd_depth_configurable_para {
    unsigned char SensorDirection; // sensor摆放位置: 0--> Horizontal 1 -->
                                   // Vertical，与模组相关
    unsigned char DepthScaleMin; // Min Value when we Scale Depth Result
    unsigned char DepthScaleMax; // Max Value when we Scale Depth Result
    unsigned char
        CalibInfiniteZeroPt; // The Calibration Zero Point is Infinite or Not
    int SearhRange;      //搜索范围
    int MinDExtendRatio; // Min Disparity Search Value Adjust Ratio
    /* 以下四个参数用于设定测距范围和阈值*/
    int inDistance;
    int inRatio;
    int outDistance;
    int outRatio;
};

static struct sprd_depth_configurable_para sprd_depth_config_para = {
    .SensorDirection = 1,
    .DepthScaleMin = 10,
    .DepthScaleMax = 200,
    .CalibInfiniteZeroPt = 1,
    .SearhRange = 32,
    .MinDExtendRatio = 50,
    .inDistance = 1500,
    .inRatio = 3,
    .outDistance = 700,
    .outRatio = 60,
};
#endif
