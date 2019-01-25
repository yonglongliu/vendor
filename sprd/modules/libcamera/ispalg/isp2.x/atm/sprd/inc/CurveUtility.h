#ifndef CURVE_UtILITY_H_
#define CURVE_UtILITY_H_

#ifdef WIN32
#include "sci_types.h"
#else
#include <linux/types.h>
#include <sys/types.h>
#include <android/log.h>
#include <sys/system_properties.h>
#endif
#ifdef __cplusplus
extern "C"  {
#endif

typedef enum {
    UTY_STS_OK,
    UTY_STS_ERR
} UTY_STS;

typedef struct {
    int i4Len;
    int i4X[8];
    int i4Y[8];
} GEN_LUT;


unsigned long binomial(unsigned long n, unsigned long k);
float BezierPiece(int n, int *i4X, int *i4Y, int *pOutput,int i4MaxY, int i4MinY);
float Bezier(int n, int i4Degree, int *i4X, int *i4Y, int *pOutput, int i4MaxY, int i4MinY);
int GetIntpByLUT(GEN_LUT *lut, int XVal);

#ifdef __cplusplus
}
#endif
#endif // CURVE_UtILITY_H_

