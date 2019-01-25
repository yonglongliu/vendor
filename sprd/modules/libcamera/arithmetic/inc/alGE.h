#ifndef _ALGE_H_
#define _ALGE_H_

#include "mtype.h"
#define alGE_ERR_CODE int
#define alGE_ERR_SUCCESS 0x00

typedef struct {
    short int m_wLeft;
    short int m_wTop;
    short int m_wWidth;
    short int m_wHeight;
} alGE_RECT;

typedef struct {
    short int m_wX;
    short int m_wY;
} alGE_2DPOINT;

#define ALTEK_IMG_FORMAT_YCC420NV21 (0x00f01)
#define ALTEK_IMG_FORMAT_YCC444SEP (0x00f02)
#define ALTEK_IMG_FORMAT_YOnly (0x00f03)

typedef struct IMAGE_INFO {
    unsigned int m_udFormat;

    unsigned int m_udStride_0;
    unsigned int m_udStride_1;
    unsigned int m_udStride_2;

    void *m_pBuf_0;
    void *m_pBuf_1;
    void *m_pBuf_2;
} ALTEK_IMAGE_INFO;

#endif // _ALGE_H_
