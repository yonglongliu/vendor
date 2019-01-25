#ifndef _SP8407_GOLDEN_OTP_H_
#define _SP8407_GOLDEN_OTP_H_
#include "otp_common.h"

static awb_target_packet_t golden_awb[AWB_MAX_LIGHT] = {
    {
        .R = 0x189, .G = 0x276, .B = 0x1A5,
    },
};

static cmr_u16 golden_lsc[] = {
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
};

#endif
