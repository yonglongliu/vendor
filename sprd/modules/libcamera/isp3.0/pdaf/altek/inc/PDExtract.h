#ifndef __PDData_H__
#define __PDData_H__
#include "alPDAF.h"

#define ERR_SUCCESS               0
#define ERR_NULL_POINTER          1
#define ERR_OUT_OF_BOUNDARY       2
#define ERR_BUFFER_SIZE           3
#define ERR_NULL_INPUT_ROI        4
#define ERR_NULL_PD_DATA_WIDTH    5
#define ERR_NULL_PD_DATA_HEIGHT   6
#define ERR_ZERO_PD_DATA_PITCH    7
#define ERR_ZERO_PD_DATA_DENSITY  8
#define ERR_ZERO_RAW_WIDTH        9
#define ERR_ZERO_RAW_HEIGHT       10

struct altek_pos_info {
    unsigned short pd_pos_x;
    unsigned short pd_pos_y;
};
struct altek_pdaf_info {
    unsigned short pd_offset_x;
    unsigned short pd_offset_y;
    unsigned short pd_pitch_x;
    unsigned short pd_pitch_y;
    unsigned short pd_density_x;
    unsigned short pd_density_y;
    unsigned short pd_block_num_x;
    unsigned short pd_block_num_y;
    unsigned short pd_pos_size;
    unsigned short pd_raw_crop_en;
    struct altek_pos_info *pd_pos_r;
    struct altek_pos_info *pd_pos_l;
};
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
extern int alPDExtract_VersionInfo_Get(void *VersionBuffer, int BufferSize);
extern int alPDExtract_GetSize(struct altek_pdaf_info *PDSensorInfo,
                               alPD_RECT *InputROI,
                               unsigned short RawFileWidth,
                               unsigned short RawFileHeight,
                               unsigned short *PDDataWidth,
                               unsigned short *PDDataHeight);
extern int alPDExtract_Run(struct altek_pdaf_info *PDSensorInfo,
                                   unsigned char *RawFile,
                                   alPD_RECT *InputROI,
                                   unsigned short RawFileWidth,
                                   unsigned short RawFileHeight,
                                   unsigned short *PDOutLeft,
                                   unsigned short *PDOutRight,
                                   unsigned short *PDDataWidth,
                                   unsigned short *PDDataHeight);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __PDData_H__ */
