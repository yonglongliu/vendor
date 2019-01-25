#ifndef SPRD_HDR
#define SPRD_HDR

#define BYTE unsigned char

typedef enum hdr_stop_flag {
    HDR_NORMAL = 0,
    HDR_STOP,
} hdr_stop_flag_t;

#ifdef __cplusplus
extern "C" {
#endif
void sprd_hdr_pool_init();
int HDR_Function(BYTE *Y0, BYTE *Y1, BYTE *Y2, BYTE *output, int height,
                 int width, char *format);
void sprd_hdr_set_stop_flag(hdr_stop_flag_t stop_flag);
int sprd_hdr_pool_destroy();
#ifdef __cplusplus
}
#endif

#endif /* ifndef SPRD_HDR */
