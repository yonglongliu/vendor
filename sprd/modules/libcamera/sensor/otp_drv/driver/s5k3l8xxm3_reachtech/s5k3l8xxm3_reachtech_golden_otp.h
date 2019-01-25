#ifndef _s5k3l8xxm3_reachtech_GOLDEN_OTP_H_
#define _s5k3l8xxm3_reachtech_GOLDEN_OTP_H_

#include "otp_common.h"

static awb_target_packet_t golden_awb[AWB_MAX_LIGHT] = {
    {
        .R = 0x140,
        .G = 0x27c,
        .B = 0x154,
        .rg_ratio = 0x102,
        .bg_ratio = 0x112,
        .GrGb_ratio = 0x200,
    },
};

static cmr_u8 golden_lsc[] = {};

#endif
