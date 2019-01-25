/*
 * Copyright (C) 2017 spreadtrum.com
 *
 */

#pragma once

#include <stdint.h>


#define RPMB_DEV_PATH         "/dev/block/mmcblk0rpmb"
#define TRUSTY_DEVICE_NAME    "/dev/trusty-ipc-dev0"

/*
 * rpmbproxy port names
 */
#define RPMBPROXY_PORT     "com.spreadtrum.rpmbproxy"

enum rpmbproxy_cmd {
        RPMBPROXY_REQ_SHIFT = 1,
        RPMBPROXY_RESP_BIT  = 1,

        RPMBPROXY_RESP_MSG_ERR   = RPMBPROXY_RESP_BIT,

        RPMBPROXY_MAC    = 1 << RPMBPROXY_REQ_SHIFT,
};

/**
 * enum rpmbproxy_res - result codes for rpmb proxy
 * @RPMBPROXY_NO_ERROR:           all OK
 * @RPMBPROXY_ERR_GENERIC:        unknown error. Can occur when there's an internal server
 *                              error, e.g. the server runs out of memory or is in a bad state.
 * @RPMBPROXY_ERR_NOT_VALID:      input not valid.
 * @RPMBPROXY_ERR_UNIMPLEMENTED:  the command passed in is not recognized
 */
enum rpmbproxy_res {
        RPMBPROXY_NO_ERROR          = 0,
        RPMBPROXY_ERR_GENERIC       = 1,
        RPMBPROXY_ERR_NOT_VALID     = 2,
        RPMBPROXY_ERR_UNIMPLEMENTED = 3,
};


/**
 * struct storage_rpmb_send_req - request format for RPMBPROXY_MAC
 * @rpmb_packet_num:           number of rpmb_packet
 * @__reserved:                 unused, must be set to 0
 * @payload:                    start of rpmb_packet.
 *
 * Only used in proxy<->server interface.
 */
struct rpmbproxy_mac_req {
        uint32_t rpmb_packet_num;
        uint32_t __reserved;
        uint8_t  payload[0];
};

/**
 * struct rpmbproxy_mac_resp: response type for RPMBPROXY_MAC
 * @data: the MAC.
 */
struct rpmbproxy_mac_resp {
        uint8_t data[0];
};

/**
 * struct rpmbproxy_msg - generic req/resp format for all rpmbproxy commands
 * @cmd:        one of enum rpmbproxy_cmd
 * @op_id:      client chosen operation identifier
 * @size:       total size of the message including this header
 * @result:     one of enum rpmbproxy_err
 * @payload:    beginning of command specific message format
 */
struct rpmbproxy_msg {
        uint32_t cmd;
        uint32_t op_id;
        uint32_t size;
        int32_t  result;
        uint8_t  payload[0];
};
