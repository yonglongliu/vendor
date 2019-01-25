/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <cutils/log.h>
#include <sys/time.h>


#include <openssl/mem.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>

#include <linux/major.h>
#include "../proxy/ioctl.h"




#define RPMB_DEBUG 1


#define MMC_READ_MULTIPLE_BLOCK  18
#define MMC_WRITE_MULTIPLE_BLOCK 25
#define MMC_RELIABLE_WRITE_FLAG (1 << 31)

#define MMC_RSP_PRESENT         (1 << 0)
#define MMC_RSP_CRC             (1 << 2)
#define MMC_RSP_OPCODE          (1 << 4)
#define MMC_CMD_ADTC            (1 << 5)
#define MMC_RSP_SPI_S1          (1 << 7)
#define MMC_RSP_R1              (MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)
#define MMC_RSP_SPI_R1          (MMC_RSP_SPI_S1)

#define MMC_WRITE_FLAG_R 0
#define MMC_WRITE_FLAG_W 1
#define MMC_WRITE_FLAG_RELW (MMC_WRITE_FLAG_W | MMC_RELIABLE_WRITE_FLAG)

#define MMC_BLOCK_SIZE 512
#define RPMB_DATA_SIZE 256
//"/sys/devices/sdio_emmc/mmc_host/mmc0/mmc0:0001/raw_rpmb_size_mult"

#define RPMB_SIZE 4 * 1024 * 128



#define RPMB_KEY_MAC_SIZE 32


#define RPMB_DEV_PATH         "/dev/block/mmcblk0rpmb"
static uint8_t rpmb_key_byte[] =  {
    0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xa,0xb,0xc,0xd,0xe,0xf,
    0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xa,0xb,0xc,0xd,0xe,0xf};

static int rpmb_fd = -1;


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
    uint8_t              data[256];
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

struct rpmb_state {
    struct rpmb_key     key;
    void                *mmc_handle;
    uint32_t            write_counter;
};

static struct rpmb_u16 rpmb_u16(uint16_t val)
{
    struct rpmb_u16 ret = {{
        val >> 8,
        val >> 0,
    }};
    return ret;
}

static struct rpmb_u32 rpmb_u32(uint32_t val)
{
    struct rpmb_u32 ret = {{
        val >> 24,
        val >> 16,
        val >> 8,
        val >> 0,
    }};
    return ret;
}

static uint16_t rpmb_get_u16(struct rpmb_u16 u16)
{
    size_t i;
    uint16_t val;

    val = 0;
    for (i = 0; i < sizeof(u16.byte); i++)
        val = val << 8 | u16.byte[i];

    return val;
}

static uint32_t rpmb_get_u32(struct rpmb_u32 u32)
{
    size_t i;
    uint32_t val;

    val = 0;
    for (i = 0; i < sizeof(u32.byte); i++)
        val = val << 8 | u32.byte[i];

    return val;
}

#if RPMB_DEBUG
#define rpmb_dprintf(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)
//#define rpmb_dprintf ALOGI
#else
#define rpmb_dprintf(fmt, ...) do { } while (0)
#endif
#define dprintf(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)


static void rpmb_dprint_buf(const char *prefix, const uint8_t *buf, size_t size)
{
#if RPMB_DEBUG
    size_t i, j;

    rpmb_dprintf("%s", prefix);
    for (i = 0; i < size; i++) {
        if (i && i % 32 == 0) {
            rpmb_dprintf("\n");
            j = strlen(prefix);
            while (j--)
                rpmb_dprintf(" ");
        }
        rpmb_dprintf(" %02x", buf[i]);
    }
    rpmb_dprintf("\n");
#endif
}

static void rpmb_dprint_u16(const char *prefix, const struct rpmb_u16 u16)
{
    rpmb_dprint_buf(prefix, u16.byte, sizeof(u16.byte));
}

static void rpmb_dprint_u32(const char *prefix, const struct rpmb_u32 u32)
{
    rpmb_dprint_buf(prefix, u32.byte, sizeof(u32.byte));
}

static void rpmb_dprint_key(const char *prefix, const struct rpmb_key key,
                            const char *expected_prefix,
                            const struct rpmb_key expected_key)
{
#if RPMB_DEBUG
    rpmb_dprint_buf(prefix, key.byte, sizeof(key.byte));
    if (CRYPTO_memcmp(key.byte, expected_key.byte, sizeof(key.byte)))
        rpmb_dprint_buf(expected_prefix, expected_key.byte,
                        sizeof(expected_key.byte));
#endif
}

static struct rpmb_nonce rpmb_nonce_init(void)
{
    struct rpmb_nonce rpmb_nonce;

    RAND_bytes(rpmb_nonce.byte, sizeof(rpmb_nonce.byte));

    return rpmb_nonce;
}

static int rpmb_mac(struct rpmb_key key,
                    struct rpmb_packet *packet, size_t packet_count,
                    struct rpmb_key *mac)
{
    size_t i;
    int hmac_ret;
    unsigned int md_len;
    HMAC_CTX hmac_ctx;

    HMAC_CTX_init(&hmac_ctx);
    hmac_ret = HMAC_Init_ex(&hmac_ctx, &key, sizeof(key), EVP_sha256(), NULL);
    if (!hmac_ret) {
        rpmb_dprintf("HMAC_Init_ex failed\n");
        goto err;
    }
    for (i = 0; i < packet_count; i++) {
        //STATIC_ASSERT(sizeof(*packet) - offsetof(typeof(*packet), data) == 284);
        hmac_ret = HMAC_Update(&hmac_ctx, packet[i].data, 284);
        if (!hmac_ret) {
            rpmb_dprintf("HMAC_Update failed\n");
            goto err;
        }
    }
    hmac_ret = HMAC_Final(&hmac_ctx, mac->byte, &md_len);
    if (md_len != sizeof(mac->byte)) {
        rpmb_dprintf("bad md_len %d != %zd\n", md_len, sizeof(mac->byte));
        exit(1);
    }
    if (!hmac_ret) {
        rpmb_dprintf("HMAC_Final failed\n");
        goto err;
    }

err:
    HMAC_CTX_cleanup(&hmac_ctx);
    return hmac_ret ? 0 : -1;
}

static int rpmb_check_response(const char *cmd_str, enum rpmb_response response_type,
                               struct rpmb_packet *res, int res_count,
                               struct rpmb_key *mac, struct rpmb_nonce *nonce,
                               uint16_t *addrp)
{
    int i;
    for (i = 0; i < res_count; i++) {
        if (rpmb_get_u16(res[i].req_resp) != response_type) {
            rpmb_dprintf("%s: Bad response type, 0x%x, expected 0x%x\n",
                    cmd_str, rpmb_get_u16(res[i].req_resp), response_type);
            return -1;
        }

        if (rpmb_get_u16(res[i].result) != RPMB_RES_OK) {
            if (rpmb_get_u16(res[i].result) == RPMB_RES_ADDR_FAILURE) {
                rpmb_dprintf("%s: Addr failure, %u\n",
                        cmd_str, rpmb_get_u16(res[i].address));
                return -ENOENT;
            }
            rpmb_dprintf("%s: Bad result, 0x%x\n", cmd_str, rpmb_get_u16(res[i].result));
            return -1;
        }

        if (i == res_count - 1 && mac && CRYPTO_memcmp(res[i].key_mac.byte, mac->byte, sizeof(mac->byte))) {
            rpmb_dprintf("%s: Bad MAC\n", cmd_str);
            return -1;
        }

        if (nonce && CRYPTO_memcmp(res[i].nonce.byte, nonce->byte, sizeof(nonce->byte))) {
            rpmb_dprintf("%s: Bad nonce\n", cmd_str);
            return -1;
        }

        if (addrp && *addrp != rpmb_get_u16(res[i].address)) {
            rpmb_dprintf("%s: Bad addr, got %u, expected %u\n",
                    cmd_str, rpmb_get_u16(res[i].address), *addrp);
            return -1;
        }
    }

    return 0;
}


int rpmb_send_dummy(void *handle_,
              void *reliable_write_buf, size_t reliable_write_size,
              void *write_buf, size_t write_size,
              void *read_buf, size_t read_size, bool sync)
{

    rpmb_dprintf("%s: handle %p, rel_write size %zu, write size %zu, read size %zu\n",
        __func__, handle_, reliable_write_size, write_size, read_size);


    int rc;
    struct mmc_ioc_cmd cmds[3];
    struct mmc_ioc_cmd *cmd;
    struct timeval start, end;

    memset(cmds, 0, sizeof(cmds));
    if (reliable_write_size) {
        if ((reliable_write_size % MMC_BLOCK_SIZE) != 0) {
            rpmb_dprintf("invalid reliable write size %u\n", reliable_write_size);
            goto err_response;
        }

        cmd = &cmds[0];
        cmd->write_flag = MMC_WRITE_FLAG_RELW;
        cmd->opcode = MMC_WRITE_MULTIPLE_BLOCK;
        cmd->flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;
        //cmd->flags = MMC_RSP_R1;
        cmd->blksz = MMC_BLOCK_SIZE;
        cmd->data_timeout_ns = 1000000000;
        cmd->is_acmd = 0;
        cmd->blocks = reliable_write_size / MMC_BLOCK_SIZE;
        mmc_ioc_cmd_set_data((*cmd), reliable_write_buf);
#ifdef RPMB_DEBUG
        rpmb_dprintf("opcode: 0x%x, write_flag: 0x%x, reliable_write_buf[511] = 0x%02x\n",
            cmd->opcode, cmd->write_flag, ((uint8_t *)reliable_write_buf)[511]);
#endif

        gettimeofday(&start, NULL);

        rc = ioctl(rpmb_fd, MMC_IOC_CMD, cmd);
        gettimeofday(&end, NULL);
        dprintf("ioctl(%d) reliable_write_size %d, use %ld us\n", rpmb_fd, reliable_write_size,
            1000000 * ( end.tv_sec - start.tv_sec ) + (end.tv_usec - start.tv_usec));

        if (rc < 0) {
            rpmb_dprintf("%s: mmc ioctl failed: %d, %s\n", __func__, rc, strerror(errno));
            goto err_response;
        }

    }

    if (write_size) {
        if ((write_size % MMC_BLOCK_SIZE) != 0) {
            rpmb_dprintf("invalid write size %u\n", write_size);
            goto err_response;
        }
        cmd = &cmds[1];
        cmd->write_flag = MMC_WRITE_FLAG_W;
        cmd->opcode = MMC_WRITE_MULTIPLE_BLOCK;
        cmd->flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;
        //cmd->flags = MMC_RSP_R1;
        cmd->blksz = MMC_BLOCK_SIZE;
        cmd->data_timeout_ns = 1000000000;
        cmd->is_acmd = 0;
        cmd->blocks = write_size / MMC_BLOCK_SIZE;
        mmc_ioc_cmd_set_data((*cmd), write_buf);
#ifdef RPMB_DEBUG
        rpmb_dprintf("opcode: 0x%x, write_flag: 0x%x, write_buf[511] = 0x%02x\n",
            cmd->opcode, cmd->write_flag, ((uint8_t *)write_buf)[511]);
#endif

        gettimeofday(&start, NULL);

        rc = ioctl(rpmb_fd, MMC_IOC_CMD, cmd);

        gettimeofday(&end, NULL);
        dprintf("ioctl(%d) write_size %d, use %ld us\n", rpmb_fd, write_size,
            1000000 * ( end.tv_sec - start.tv_sec ) + (end.tv_usec - start.tv_usec));

        if (rc < 0) {
            rpmb_dprintf("%s: mmc ioctl failed: %d, %s\n", __func__, rc, strerror(errno));
            goto err_response;
        }

    }

    if (read_size) {
        if (read_size % MMC_BLOCK_SIZE != 0 ) {
            rpmb_dprintf("%s: invalid read size %u\n", __func__, read_size);
            goto err_response;
        }
        cmd = &cmds[2];
        cmd->write_flag = MMC_WRITE_FLAG_R;
        cmd->opcode = MMC_READ_MULTIPLE_BLOCK;
        cmd->flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC,
        cmd->blksz = MMC_BLOCK_SIZE;
        cmd->data_timeout_ns = 1000000000;
        cmd->is_acmd = 0;
        cmd->blocks = read_size / MMC_BLOCK_SIZE;
        mmc_ioc_cmd_set_data((*cmd), read_buf);
#ifdef RPMB_DEBUG
        rpmb_dprintf("opcode: 0x%x, write_flag: 0x%x, read_buf[511] = 0x%02x\n",
            cmd->opcode, cmd->write_flag, ((uint8_t *)read_buf)[511]);
#endif
        gettimeofday(&start, NULL);
        rc = ioctl(rpmb_fd, MMC_IOC_CMD, cmd);

        gettimeofday(&end, NULL);
        dprintf("ioctl(%d) read_size %d, use %ld us\n", rpmb_fd, write_size,
            1000000 * ( end.tv_sec - start.tv_sec ) + (end.tv_usec - start.tv_usec));


        if (rc < 0) {
            rpmb_dprintf("%s: mmc ioctl failed: %d, %s\n", __func__, rc, strerror(errno));
            goto err_response;
        }
    }

    return rc;

err_response:
    return -1;
}

static int rpmb_read_counter(struct rpmb_state *state, uint32_t *write_counter)
{
    int ret;
    struct rpmb_key mac;
    struct rpmb_nonce nonce = rpmb_nonce_init();
    struct rpmb_packet cmd = {
        .nonce = nonce,
        .req_resp = rpmb_u16(RPMB_REQ_GET_COUNTER),
    };
    struct rpmb_packet res;

    //cmd->blocks = 1,the block count must be set to 1.
    ret = rpmb_send_dummy(state->mmc_handle, NULL, 0, &cmd, sizeof(cmd), &res, sizeof(res), false);
    if (ret < 0) {
        rpmb_dprintf("rpmb_read_counter failed\n");
        return ret;
    }

    ret = rpmb_mac(state->key, &res, 1, &mac);
    if (ret < 0) {
        rpmb_dprintf("rpmb_read_counter check mac failed\n");
        return ret;
    }

    rpmb_dprintf("rpmb: read counter response:\n");
    rpmb_dprint_key("  key/mac       ", res.key_mac, "   expected mac ", mac);
    rpmb_dprint_buf("  nonce         ", res.nonce.byte, sizeof(res.nonce.byte));
    rpmb_dprint_u32("  write_counter ", res.write_counter);
    rpmb_dprint_u16("  result        ", res.result);
    rpmb_dprint_u16("  req/resp      ", res.req_resp);

    if (write_counter)
        *write_counter = rpmb_get_u32(res.write_counter);

    ret = rpmb_check_response("read counter", RPMB_RESP_GET_COUNTER,
                              &res, 1, &mac, &nonce, NULL);

    return ret;
}


int rpmb_read(struct rpmb_state *state, void *buf, uint16_t addr, uint16_t count)
{
    int i;
    int ret;
    struct rpmb_key mac;
    struct rpmb_nonce nonce = rpmb_nonce_init();
    struct rpmb_packet cmd = {
        .nonce = nonce,
        .address = rpmb_u16(addr),
        .req_resp = rpmb_u16(RPMB_REQ_DATA_READ),
    };
    struct rpmb_packet res[count];
    uint8_t *bufp;
    struct timeval start, end;

    gettimeofday(&start, NULL);

    if (!state)
        return -EINVAL;

    memset(res, 0x0, sizeof(res));
    ret = rpmb_send_dummy(state->mmc_handle, NULL, 0, &cmd, sizeof(cmd), res, sizeof(res), false);
    if (ret < 0)
        return ret;

    ret = rpmb_mac(state->key, res, count, &mac);
    if (ret < 0)
        return ret;

    rpmb_dprintf("rpmb: read data, addr %d, count %d, response:\n", addr, count);
    for (i = 0; i < count; i++) {
        rpmb_dprintf("  block %d\n", i);
        if (i == count - 1)
            rpmb_dprint_key("    key/mac       ", res[i].key_mac, "     expected mac ", mac);
            rpmb_dprint_buf("    data          ", res[i].data, sizeof(res[i].data));
            rpmb_dprint_buf("    nonce         ", res[i].nonce.byte, sizeof(res[i].nonce.byte));
            rpmb_dprint_u16("    address       ", res[i].address);
            rpmb_dprint_u16("    block_count   ", res[i].block_count);
            rpmb_dprint_u16("    result        ", res[i].result);
            rpmb_dprint_u16("    req/resp      ", res[i].req_resp);
    }

    ret = rpmb_check_response("read data", RPMB_RESP_DATA_READ,
                              res, count, &mac, &nonce, &addr);
    if (ret < 0)
        return ret;

    for (bufp = buf, i = 0; i < count; i++, bufp += sizeof(res[i].data))
        memcpy(bufp, res[i].data, sizeof(res[i].data));

    gettimeofday(&end, NULL);
    dprintf("rpmb: read data, addr %d, count %d, use %ld us\n", addr, count,
        1000000 * ( end.tv_sec - start.tv_sec ) + (end.tv_usec - start.tv_usec));

    return 0;
}

static int rpmb_write_data(struct rpmb_state *state, const char *buf, uint16_t addr, uint16_t count, bool sync)
{
    int i;
    int ret;
    struct rpmb_key mac;
    struct rpmb_packet cmd[count];
    struct rpmb_packet rescmd = {
        .req_resp = rpmb_u16(RPMB_REQ_RESULT_READ),
    };
    struct rpmb_packet res;
    struct timeval start, end;

    gettimeofday(&start, NULL);

    for (i = 0; i < count; i++) {
        memset(&cmd[i], 0, sizeof(cmd[i]));
        memcpy(cmd[i].data, buf + i * sizeof(cmd[i].data), sizeof(cmd[i].data));
        rpmb_dprint_buf("    data          ", cmd[i].data, sizeof(cmd[i].data));
        cmd[i].write_counter = rpmb_u32(state->write_counter);
        cmd[i].address = rpmb_u16(addr);
        cmd[i].block_count = rpmb_u16(count);
        cmd[i].req_resp = rpmb_u16(RPMB_REQ_DATA_WRITE);
    }
    ret = rpmb_mac(state->key, cmd, count, &cmd[count - 1].key_mac);
    if (ret < 0)
        return ret;

    memset(&res, 0x0, sizeof(res));
    ret = rpmb_send_dummy(state->mmc_handle, cmd, sizeof(cmd), &rescmd, sizeof(rescmd), &res, sizeof(res), sync);
    if (ret < 0)
        return ret;

    ret = rpmb_mac(state->key, &res, 1, &mac);
    if (ret < 0)
        return ret;

    rpmb_dprintf("rpmb: write data, addr %d, count %d, write_counter %d, response\n", addr, count, state->write_counter);
    rpmb_dprint_key("  key/mac       ", res.key_mac, "   expected mac ", mac);
    rpmb_dprint_buf("  nonce         ", res.nonce.byte, sizeof(res.nonce.byte));
    rpmb_dprint_u32("  write_counter ", res.write_counter);
    rpmb_dprint_u16("  address       ", res.address);
    rpmb_dprint_u16("  result        ", res.result);
    rpmb_dprint_u16("  req/resp      ", res.req_resp);

    ret = rpmb_check_response("write data", RPMB_RESP_DATA_WRITE,
                              &res, 1, &mac, NULL, &addr);
    if (ret < 0) {
        if (rpmb_get_u16(res.result) == RPMB_RES_COUNT_FAILURE)
            state->write_counter = 0; /* clear counter to trigger a re-read */
        return ret;
    }

    state->write_counter++;

    gettimeofday(&end, NULL);
    dprintf("rpmb: write data, addr %d, count %d use %ld us\n", addr, count,
        1000000 * ( end.tv_sec - start.tv_sec ) + (end.tv_usec - start.tv_usec));

    return 0;
}

/*
*@addr  the start address of full acces. the address is the block index of the half sectors(256B)
*@count how many block to write.the block count is the number of the half sectors(256B).
*/
int rpmb_write(struct rpmb_state *state, const void *buf, uint16_t addr, uint16_t count, bool sync)
{
    int ret;

    if (!state)
        return -EINVAL;

    if (!state->write_counter) {
        ret = rpmb_read_counter(state, &state->write_counter);
        if (ret < 0)
            return ret;
    }
    return rpmb_write_data(state, buf, addr, count, sync);
}


int rpmb_program_key(struct rpmb_key *key)
{
    struct mmc_ioc_cmd cmds[3];
    struct mmc_ioc_cmd *cmd;

    struct rpmb_packet req;
    struct rpmb_packet res;
    int rc;

    rpmb_dprintf("rpmb_program_key() start \n");

    if (NULL == key) {
        rpmb_dprintf(" rpmb_program_key()  fail, key = 0x%x\n", *(key->byte));
        return -1;
    }

    memset(cmds, 0, sizeof(cmds));
//for write rpmb key req
    cmd = &cmds[0];
    cmd->write_flag = MMC_WRITE_FLAG_RELW;
    cmd->opcode = MMC_WRITE_MULTIPLE_BLOCK;
    cmd->flags = MMC_RSP_R1;
    cmd->blksz = MMC_BLOCK_SIZE;
    cmd->data_timeout_ns = 1000000000;
    cmd->is_acmd = 0;
    cmd->blocks = 1;  //must set 1

    memset(&req, 0, sizeof(req));
    req.req_resp = rpmb_u16(RPMB_REQ_PROGRAM_KEY);
    memcpy(req.key_mac.byte, key->byte, RPMB_KEY_MAC_SIZE);

    mmc_ioc_cmd_set_data((*cmd), &req);
#ifdef RPMB_DEBUG
    rpmb_dprintf("opcode: 0x%x, write_flag: 0x%x\n", cmd->opcode, cmd->write_flag);
#endif
    rc = ioctl(rpmb_fd, MMC_IOC_CMD, cmd);
    if (0 > rc) {
        rpmb_dprintf("%s: mmc ioctl failed: %d, %s\n", __func__, rc, strerror(errno));
        return rc;
    }


//for read result req
    cmd = &cmds[1];
    cmd->write_flag = MMC_WRITE_FLAG_W;
    //the result read sequence is initiated by write Multiple Block command CMD25.
    cmd->opcode = MMC_WRITE_MULTIPLE_BLOCK;
    cmd->flags = MMC_RSP_R1;
    cmd->blksz = MMC_BLOCK_SIZE;
    cmd->data_timeout_ns = 1000000000;
    cmd->is_acmd = 0;
    cmd->blocks = 1;  //must set 1
    memset(&req, 0, sizeof(req));
    req.req_resp = rpmb_u16(RPMB_REQ_RESULT_READ);
    mmc_ioc_cmd_set_data((*cmd), &req);
#ifdef RPMB_DEBUG
    rpmb_dprintf("opcode: 0x%x, write_flag: 0x%x\n", cmd->opcode, cmd->write_flag);
#endif

    rc = ioctl(rpmb_fd, MMC_IOC_CMD, cmd);
    if (0 > rc) {
        rpmb_dprintf("%s: mmc ioctl failed: %d, %s\n", __func__, rc, strerror(errno));
        return rc;
    }


    cmd = &cmds[2];
    cmd->write_flag = MMC_WRITE_FLAG_R;
    //read the result
    cmd->opcode = MMC_READ_MULTIPLE_BLOCK;
    cmd->flags = MMC_RSP_R1;
    cmd->blksz = MMC_BLOCK_SIZE;
    cmd->blocks = 1;
    cmd->data_timeout_ns = 1000000000;
    cmd->is_acmd = 0;

    memset(&res, 0, sizeof(res));
    mmc_ioc_cmd_set_data((*cmd), &res);

#ifdef RPMB_DEBUG
     rpmb_dprintf("opcode: 0x%x, write_flag: 0x%x\n", cmd->opcode, cmd->write_flag);
#endif

    rc = ioctl(rpmb_fd, MMC_IOC_CMD, cmd);
    if (rc < 0) {
        rpmb_dprintf("%s: mmc ioctl failed: %d, %s\n", __func__, rc, strerror(errno));
        return -1;
    }


//result check
    if (RPMB_RESP_PROGRAM_KEY != rpmb_get_u16(res.req_resp)) {
        rpmb_dprintf("rpmb_program_key: Bad response type, 0x%x, expected 0x%x\n",
        rpmb_get_u16(res.req_resp), RPMB_RESP_PROGRAM_KEY);
        return -1;
    }

    if (RPMB_RES_OK != rpmb_get_u16(res.result)) {
        rpmb_dprintf("rpmb_program_key: Bad result, 0x%x\n", rpmb_get_u16(res.result));
        return -1;
    }


    rpmb_dprintf("rpmb_program_key() successed \n");
    return 0;
}



int rpmb_init(struct rpmb_state **statep,
              void *mmc_handle,
              const struct rpmb_key *key)
{
    struct rpmb_state *state = malloc(sizeof(*state));

    if (!state)
        return -ENOMEM;

    state->mmc_handle = mmc_handle;
    state->key = *key;
    state->write_counter = 0;

    *statep = state;

    return 0;
}

void rpmb_uninit(struct rpmb_state *statep)
{
    free(statep);
}


int  main(void)
{
    int rc = -1;
    struct rpmb_key key;
    struct rpmb_state *state;

    rpmb_fd = open(RPMB_DEV_PATH, O_RDWR);
    if (0 >= rpmb_fd ) {
        rpmb_dprintf("open rpmb device %s failed\n", RPMB_DEV_PATH);
        return -1;

    }

    memcpy(key.byte, rpmb_key_byte, RPMB_KEY_MAC_SIZE);
    rpmb_init(&state, NULL, &key);


/*
//program rpmb authen key
    rc = rpmb_program_key(&key);
    if (0 > rc) {
        rpmb_dprintf("rpmb_program_key fail, return %d\n", rc);
        return rc;
    }



//read rpmb write counter
    rc = rpmb_read_counter(state, &state->write_counter);
    if (0 > rc) {
        rpmb_dprintf("rpmb_read_counter fail, return %d\n", rc);
        return rc;
    }
    rpmb_dprintf("rpmb write count %d\n", state->write_counter);
*/




    uint8_t data_wr[MMC_BLOCK_SIZE];
    uint8_t data_rd[MMC_BLOCK_SIZE];
    uint16_t block_ind, block_count;




/*
//clean rpmb
    memset(&data_wr, 0x0, sizeof(data_wr));
    uint16_t block_max_ind = (RPMB_SIZE / MMC_BLOCK_SIZE - 1) * 2;  //the last block
    block_count = sizeof(data_wr) / RPMB_DATA_SIZE;
    for (uint16_t ind = 0; ind <= block_max_ind; ind += block_count) {
        rpmb_write(state, data_wr, ind, block_count, true);
    }

*/


//clean rpmb first and last block
    memset(&data_wr, 0x0, sizeof(data_wr));
    block_count = sizeof(data_wr) / RPMB_DATA_SIZE;
    block_ind = 0; //the first block
    rpmb_write(state, data_wr, block_ind, block_count, true);


    memset(&data_wr, 0x0, sizeof(data_wr));
    block_ind = (RPMB_SIZE / MMC_BLOCK_SIZE - 1) * 2;  //the last block
    block_count = sizeof(data_wr) / RPMB_DATA_SIZE;
    rpmb_write(state, data_wr, block_ind, block_count, true);


//check clean
    memset(&data_rd, 0x0, sizeof(data_rd));
    block_count = sizeof(data_rd) / RPMB_DATA_SIZE;
    block_ind = 0; //the first block
    rpmb_read(state, data_rd, block_ind, block_count);


    memset(&data_rd, 0x0, sizeof(data_rd));
    block_count = sizeof(data_rd) / RPMB_DATA_SIZE;
    block_ind = (RPMB_SIZE / MMC_BLOCK_SIZE - 1) * 2;  //the last block
    rpmb_read(state, data_rd, block_ind, block_count);




//write rpmb first and last block
    memset(&data_wr, 0xa5, sizeof(data_wr));
    block_count = sizeof(data_wr) / RPMB_DATA_SIZE;
    block_ind = 0; //the first block
    rpmb_write(state, data_wr, block_ind, block_count, true);


    memset(&data_wr, 0x5a, sizeof(data_wr));
    block_ind = (RPMB_SIZE / MMC_BLOCK_SIZE - 1) * 2;  //the last block
    block_count = sizeof(data_wr) / RPMB_DATA_SIZE;
    rpmb_write(state, data_wr, block_ind, block_count, true);



//check write
    memset(&data_rd, 0x0, sizeof(data_rd));
    block_count = sizeof(data_rd) / RPMB_DATA_SIZE;
    block_ind = 0; //the first block
    rpmb_read(state, data_rd, block_ind, block_count);

    memset(&data_rd, 0x0, sizeof(data_rd));
    block_count = sizeof(data_rd) / RPMB_DATA_SIZE;
    block_ind = (RPMB_SIZE / MMC_BLOCK_SIZE - 1) * 2;  //the last block
    rpmb_read(state, data_rd, block_ind, block_count);


    close(rpmb_fd);
    rpmb_uninit(state);

    return 0;

}
