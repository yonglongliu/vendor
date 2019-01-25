#ifndef _GC5024_COMMON_GOLDEN_OTP_H_
#define _GC5024_COMMON_GOLDEN_OTP_H_
#include "otp_common.h"
// dnp: 9# 53,70,46; 0.757, 0.657
// d50: 9# 51, 73, 43; 0.589, 0.685
// d65: 9# 43, 73, 50; 0.589, 0.685
// bmp d50: 12.14,30.20,11.44  ;  0.40, 0.37  ; 0.8, 0.74
// bmp dnp: 12.16,36.9,11.27   ;  0.33, 0.31  ; 0.66, 0.62
// bmp d65: 10.42,37.10,13.27  ;  0.28, 0.36  ; 0.56, 0.72

static awb_target_packet_t golden_awb[AWB_MAX_LIGHT] = {
    {
        .R = 0xA6, .G = 0x100, .B = 0x9A,
    },
};
static cmr_u8 golden_lsc[] = {};

#endif
