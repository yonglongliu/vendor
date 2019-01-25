
#ifndef _AKMORI_H_
#define _AKMORI_H_

extern "C" {
int AKMInitLib(uint32_t deviceid, uint32_t layout);
int AKMDoCaliApi(int64_t timeStamp, float ucalibmag[ ], int64_t status, float *calibmag, float *magbias, int8_t *lv);
int AKMCaliApiSetAccData(int64_t timeStamp, float acc[ ], int64_t status);
int GetOrientation(float *outputData);
int AKMCaliApiSetOffset(float offset[3]);

}
#endif

