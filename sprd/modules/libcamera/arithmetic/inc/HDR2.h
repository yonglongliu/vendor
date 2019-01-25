#ifndef MORPHO_HDR
#define MORPHO_HDR

typedef unsigned char BYTE;

#ifdef __cplusplus
extern "C" {
#endif
int HDR_Function(BYTE *Y0, BYTE *Y1, BYTE *Y2, BYTE *output, int height,
                 int width, char *format);
#ifdef __cplusplus
}
#endif

#endif /* ifndef MORPHO_HDR */
