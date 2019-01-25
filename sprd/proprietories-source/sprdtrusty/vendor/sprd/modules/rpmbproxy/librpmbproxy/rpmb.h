/*
 * Copyright (C) 2017 spreadtrum.com
 *
 */

#pragma once

#include <stdint.h>

#define MMC_READ_MULTIPLE_BLOCK 18
#define MMC_WRITE_MULTIPLE_BLOCK 25
#define MMC_RELIABLE_WRITE_FLAG (1 << 31)

#define MMC_RSP_PRESENT (1 << 0)
#define MMC_RSP_CRC (1 << 2)
#define MMC_RSP_OPCODE (1 << 4)
#define MMC_CMD_ADTC (1 << 5)
#define MMC_RSP_SPI_S1 (1 << 7)
#define MMC_RSP_R1 (MMC_RSP_PRESENT | MMC_RSP_CRC | MMC_RSP_OPCODE)
#define MMC_RSP_SPI_R1 (MMC_RSP_SPI_S1)

#define MMC_WRITE_FLAG_R 0
#define MMC_WRITE_FLAG_W 1
#define MMC_WRITE_FLAG_RELW (MMC_WRITE_FLAG_W | MMC_RELIABLE_WRITE_FLAG)

#define MMC_BLOCK_SIZE 512

#define MMC_RPMB_DATA_SIZE 256



struct rpmb_nonce {
    uint8_t     byte[16];
};

struct rpmb_u16 {
    uint8_t     byte[2];
};

struct rpmb_u32 {
    uint8_t     byte[4];
};

struct rpmb_key {
    uint8_t     byte[32];
};


struct rpmb_packet {
    uint8_t              pad[196];
    struct rpmb_key      key_mac;
    uint8_t              data[MMC_RPMB_DATA_SIZE];
    struct rpmb_nonce    nonce;
    struct rpmb_u32      write_counter;
    struct rpmb_u16      address;
    struct rpmb_u16      block_count;
    struct rpmb_u16      result;
    struct rpmb_u16      req_resp;
};

enum rpmb_request {
    RPMB_REQ_PROGRAM_KEY                = 0x0001,
    RPMB_REQ_GET_COUNTER                = 0x0002,
    RPMB_REQ_DATA_WRITE                 = 0x0003,
    RPMB_REQ_DATA_READ                  = 0x0004,
    RPMB_REQ_RESULT_READ                = 0x0005,
};

enum rpmb_response {
    RPMB_RESP_PROGRAM_KEY               = 0x0100,
    RPMB_RESP_GET_COUNTER               = 0x0200,
    RPMB_RESP_DATA_WRITE                = 0x0300,
    RPMB_RESP_DATA_READ                 = 0x0400,
};

enum rpmb_result {
    RPMB_RES_OK                         = 0x0000,
    RPMB_RES_GENERAL_FAILURE            = 0x0001,
    RPMB_RES_AUTH_FAILURE               = 0x0002,
    RPMB_RES_COUNT_FAILURE              = 0x0003,
    RPMB_RES_ADDR_FAILURE               = 0x0004,
    RPMB_RES_WRITE_FAILURE              = 0x0005,
    RPMB_RES_READ_FAILURE               = 0x0006,
    RPMB_RES_NO_AUTH_KEY                = 0x0007,

    RPMB_RES_WRITE_COUNTER_EXPIRED      = 0x0080,
};
