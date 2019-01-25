/*
 * Copyright (C) 2017 spreadtrum.com
 */

#pragma once

#define RPMB_SERVER_NAME "rpmb_cli"
#define RPMB_KEY_LEN 32

enum rpmb_socket_cmd {
    IS_WR_RPMB_KEY = 1,
    WR_RPMB_KEY = 1 << 1,
    RUN_STORAGEPROXY = 1 << 2,
};
