/* Copyright (c) 2018, The Linux Foundataion. All rights reserved.
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
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AsND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#define LOG_TAG "cmr_img_debug"

#include <stdlib.h>
#include <cutils/trace.h>
#include <cutils/properties.h>
#include "cmr_img_debug.h"

#define NUMBER_DEBUG 8

static void overlay_rectangle(unsigned char* pbuf, unsigned int w, unsigned int h, int roi_w, int roi_h)
{
    int off_x = (w - roi_w) / 2;
    int off_y = (h - roi_h) / 2;
    unsigned char* py = NULL;
    int i;

    py = pbuf;
    py += w * off_y + off_x;
    memset(py, 0xff, roi_w);

    py = pbuf;
    py += w * (off_y+roi_h) + off_x;
    memset(py, 0xff, roi_w);

    py = pbuf;
    py += w * off_y + off_x;
    for (i=off_y; i<off_y+roi_h; i++) {
        *py = 255;
        py += w;
    }

    py = pbuf;
    py += w * off_y + off_x + roi_w;
    for (i=off_y; i<off_y+roi_h; i++) {
        *py = 255;
        py += w;
    }
}

cmr_int add_face_postion() {
    CMR_LOGD("add_face_postion");
    return CMR_CAMERA_SUCCESS;
};

cmr_int cmr_img_debug(void *param1, void *param2) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct img_debug *img_debug = param1;
    if (NULL == img_debug) {
        CMR_LOGE("invalid param1:%p", param1);
        ret = -CMR_CAMERA_INVALID_PARAM;
        goto exit;
    }
    CMR_LOGD("img_debug->input.addr_y:0x%x,  img_debug->input.addr_u:0x%x,  "
             "img_debug->output.addr_y:0x%x, img_debug->size.width:%d, "
             "img_debug->size.height:%d,   img_debug->format:%d",
             img_debug->input.addr_y, img_debug->input.addr_u,
             img_debug->output.addr_y, img_debug->size.width,
             img_debug->size.height, img_debug->format);
    char value[PROPERTY_VALUE_MAX];
    property_get("persist.sys.camera.debug.type", value, "0");
    int type = atoi(value);
    int i = 0;
    for (; i < NUMBER_DEBUG; i++) {
        if (type >> i == 0) { // reduce the loop count
            break;
        }
        switch (type & (1 << i)) {
        case 1 << 0: {
            struct face_tag *face_tag = img_debug->params;
            CMR_LOGD("bit 0 is set, face_tag:%p", face_tag);
            if (face_tag != NULL) {
                CMR_LOGD("face_tag->face_num:%d", face_tag->face_num);
                ret = add_face_postion();
            }
            break;
        }
        case 1 << 1: {
            CMR_LOGD("bit 1 is set");
            property_get("debug.sys.cam.roi.set", value, "");
            if (*value != '\0') {
                unsigned int x1 = 1;
                unsigned int x1_base = 1;
                unsigned int y1 = -1;
                unsigned int y1_base = 1;
                unsigned int x2 = -1;
                unsigned int x2_base = 1;
                unsigned int y2 = -1;
                unsigned int y2_base = 1;
                unsigned int w =  img_debug->size.width;
                unsigned int h = img_debug->size.height;
                unsigned char* py = (unsigned char *)(img_debug->input.addr_y);

                int num = sscanf(value, "[%d/%d, %d/%d][%d/%d, %d/%d]", &x1, &x1_base, &y1, &y1_base, &x2, &x2_base, &y2, &y2_base);

                if (num >= 4) {
                    if (x1_base < x1)	x1_base = x1;
                    if (y1_base < y1)	y1_base = y1;
                    if (x1_base == 0)	x1_base = 1;
                    if (y1_base == 0)	y1_base = 1;
                    int roi_w = (w * x1 + x1_base/2) / x1_base;
                    int roi_h = (h * y1 + y1_base/2) / y1_base;

                    overlay_rectangle(py, w, h, roi_w, roi_h);
                }
                if (num == 8) {
                    if (x2_base < x2)	x2_base = x2;
                    if (y2_base < y2)	y2_base = y2;
                    if (x2_base == 0)	x2_base = 1;
                    if (y2_base == 0)	y2_base = 1;
                    int roi_w = (w * x2 + x2_base/2) / x2_base;
                    int roi_h = (h * y2 + y2_base/2) / y2_base;

                    overlay_rectangle(py, w, h, roi_w, roi_h);
                }
            }
            break;
        }
        case 1 << 2: {
            CMR_LOGD("bit 2 is set");
            break;
        }
        case 1 << 3: {
            CMR_LOGD("bit 3 is set");
            break;
        }
        case 1 << 4: {
            CMR_LOGD("bit 4 is set");
            break;
        }
        case 1 << 5: {
            CMR_LOGD("bit 5 is set");
            break;
        }
        case 1 << 6: {
            CMR_LOGD("bit 6 is set");
            break;
        }
        case 1 << 7: {
            CMR_LOGD("bit 7 is set");
            break;
        }
        default: {
            CMR_LOGW("the bit :%d is reserved. debug type:%d", i, type);
        }
        }
    }
exit:
    return ret;
}
