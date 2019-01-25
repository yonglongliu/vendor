#ifndef __SPRDBOKEH_LIBRARY_HEADER_
#define __SPRDBOKEH_LIBRARY_HEADER_

#ifdef __cplusplus
extern "C" {
#endif

int sprd_bokeh_Init(void **handle, int a_dInImgW, int a_dInImgH, char *param);

int sprd_bokeh_Close(void *handle);

int sprd_bokeh_VersionInfo_Get(void *a_pOutBuf, int a_dInBufMaxSize);

int sprd_bokeh_ReFocusPreProcess(void *handle, void *a_pInBokehBufYCC420NV21,
                                 void *a_pInDisparityBuf16);

int sprd_bokeh_ReFocusGen(void *handle, void *a_pOutBlurYCC420NV21,
                          int a_dInBlurStrength, int a_dInPositionX,
                          int a_dInPositionY);

#ifdef __cplusplus
}
#endif

#endif
