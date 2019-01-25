
#ifndef SPRDCAMERAFACEBEAUTYBASE_H_HEADER
#define SPRDCAMERAFACEBEAUTYBASE_H_HEADER

#include <stdlib.h>
#include <utils/Log.h>
#include "../SprdCamera3Setting.h"
#include "camera_face_beauty.h"

namespace sprdcamera {

class SprdCamera3FaceBeautyBase {

#ifdef CONFIG_FACE_BEAUTY
  public:
    SprdCamera3FaceBeautyBase() {
        memset(&face_beauty, 0, sizeof(face_beauty));
        memset(&beautyLevels, 0, sizeof(beautyLevels));
    }
    virtual ~SprdCamera3FaceBeautyBase() {
        // deinit_fb_handle(&face_beauty);
    }
    virtual void doFaceMakeup2(struct camera_frame_type *frame,
                               face_beauty_levels levels, FACE_Tag *face_info,
                               int work_mode) {
        int sx, sy, ex, ey, angle, pose;

        beautyLevels.blemishLevel = levels.blemishLevel;
        beautyLevels.smoothLevel = levels.smoothLevel;
        beautyLevels.skinColor = levels.skinColor;
        beautyLevels.skinLevel = levels.skinLevel;
        beautyLevels.brightLevel = levels.brightLevel;
        beautyLevels.lipColor = levels.lipColor;
        beautyLevels.lipLevel = levels.lipLevel;
        beautyLevels.slimLevel = levels.slimLevel;
        beautyLevels.largeLevel = levels.largeLevel;
        if (face_info->face_num > 0) {
            for (int i = 0 ; i < face_info->face_num; i++) {
                  sx = face_info->face[i].rect[0];
                  sy = face_info->face[i].rect[1];
                  ex = face_info->face[i].rect[2];
                  ey = face_info->face[i].rect[3];
                  angle = face_info->angle[i];
                  pose = face_info->pose[i];
                  construct_fb_face(&face_beauty, i, sx, sy, ex, ey,angle,pose);
             }
        }
        init_fb_handle(&face_beauty, work_mode, 2);
        construct_fb_image(&face_beauty, frame->width, frame->height,
                               (unsigned char *)(frame->y_vir_addr),
                               (unsigned char *)(frame->y_vir_addr +
                                                 frame->width * frame->height), 0);
        construct_fb_level(&face_beauty, beautyLevels);
        do_face_beauty(&face_beauty, face_info->face_num);
        deinit_fb_handle(&face_beauty);
    }
virtual bool isFaceBeautyOn(face_beauty_levels levels) {
        return (levels.blemishLevel ||levels.brightLevel ||levels.largeLevel ||levels.lipColor ||levels.lipLevel ||levels.skinColor || levels.skinLevel ||levels.slimLevel ||levels.smoothLevel);
}
  private:
    struct class_fb face_beauty;
    struct face_beauty_levels beautyLevels;

#endif
};

} // namespace sprdcamera
#endif
