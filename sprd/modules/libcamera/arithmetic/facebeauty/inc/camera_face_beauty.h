#ifndef SPRD_FB_H
#define SPRD_FB_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <string.h>
#include "sprdfacebeauty.h"
#include <cutils/properties.h>

struct class_fb {
    int fb_mode;
    int firstFrm;
    char sprdAlgorithm[PROPERTY_VALUE_MAX];
    FB_BEAUTY_OPTION fb_option;
    FB_IMAGE_YUV420SP fb_image;
    FB_FACEINFO fb_face[10];
    FB_BEAUTY_HANDLE hSprdFB;
};

struct face_beauty_levels {
    unsigned char
        blemishLevel; /* Flag for removing blemish; 0 --> OFF; 1 --> ON    */
    unsigned char smoothLevel; /* Smooth skin level. Value range [0, 10] */
    unsigned char skinColor; /* The target skin color: white, rosy, or wheat */
    unsigned char
        skinLevel; /* The level to tune skin color. Value range [0, 10] */
    unsigned char brightLevel; /* Skin brightness level. Value range [0, 10] */
    unsigned char lipColor; /* The target lip color: crimson, pink or fuchsia */
    unsigned char lipLevel; /* Red lips level. Value range [0, 10] */
    unsigned char slimLevel; /* Slim face level. Value range [0, 10] */
    unsigned char largeLevel; /* Enlarge eye level. Value range [0, 10] */
};

void init_fb_handle(struct class_fb *faceBeauty, int workMode, int threadNum);
void deinit_fb_handle(struct class_fb *faceBeauty);
void construct_fb_face(struct class_fb *faceBeauty, int j, int sx, int sy,
                       int ex, int ey, int angle, int pose);
void construct_fb_image(struct class_fb *faceBeauty, int picWidth,
                        int picHeight, unsigned char *addrY,
                        unsigned char *addrU, int format);
void construct_fb_level(struct class_fb *faceBeauty,
                        struct face_beauty_levels beautyLevels);
void do_face_beauty(struct class_fb *faceBeauty, int faceCount);

#ifdef __cplusplus
}
#endif

#endif /* SPRD_FB_H */
