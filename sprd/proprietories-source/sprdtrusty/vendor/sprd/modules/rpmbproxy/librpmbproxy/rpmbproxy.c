/*
 * Copyright (C) 2016 spreadtrum.com
 *
 */
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/uio.h>

#include <android/log.h>
#include <trusty/tipc.h>

#include <openssl/mem.h>
#include <openssl/rand.h>
#include <rpmbproxyinterface.h>

#include "log.h"
#include "rpmbproxy.h"
#include "rpmb_blk_rng.h"
#include "rpmb.h"
#include "rpmb_ops.h"

static int mmc_handle = -1;
static int session = -1;
static int init_done = -1;



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


static int rpmb_check_res(const char *cmd_str, enum rpmb_response response_type,
                               struct rpmb_packet *res, int res_count,
                               struct rpmb_key *mac, struct rpmb_nonce *nonce,
                               int *addrp)
{
    int i;
    for (i = 0; i < res_count; i++) {
        if (rpmb_get_u16(res[i].req_resp) != response_type) {
            ALOGE("%s: Bad response type, 0x%x, expected 0x%x\n",
                    cmd_str, rpmb_get_u16(res[i].req_resp), response_type);
            return -1;
        }

        if (rpmb_get_u16(res[i].result) != RPMB_RES_OK) {
            if (rpmb_get_u16(res[i].result) == RPMB_RES_ADDR_FAILURE) {
                ALOGE("%s: Addr failure, %u\n",
                        cmd_str, rpmb_get_u16(res[i].address));
                return -ENOENT;
            }
            ALOGE("%s: Bad result, 0x%x\n", cmd_str, rpmb_get_u16(res[i].result));
            return -1;
        }

        if (i == res_count - 1 && mac && CRYPTO_memcmp(res[i].key_mac.byte, mac->byte, sizeof(mac->byte))) {
            ALOGE("%s: Bad MAC\n", cmd_str);
            return -1;
        }

        if (nonce && CRYPTO_memcmp(res[i].nonce.byte, nonce->byte, sizeof(nonce->byte))) {
            ALOGE("%s: Bad nonce\n", cmd_str);
            return -1;
        }

        if (addrp && *addrp != rpmb_get_u16(res[i].address)) {
            ALOGE("%s: Bad addr, got %u, expected %u\n",
                    cmd_str, rpmb_get_u16(res[i].address), *addrp);
            return -1;
        }
    }

    return 0;
}

/*lint -e527*/
static int check_res(struct rpmbproxy_msg *msg, int res)
{
    if (res < 0)
        return res;

    if ((size_t)res < sizeof(*msg)) {
        ALOGE("invalid msg length (%zd < %zd)\n", res, sizeof(*msg));
        return -EIO;
    }

    //ALOGV("cmd 0x%x: server returned %u\n", msg->cmd, msg->result);

    switch(msg->result) {
        case RPMBPROXY_NO_ERROR:
            return res - sizeof(*msg);

        case RPMBPROXY_ERR_NOT_VALID:
            ALOGE("cmd 0x%x: parameter is invalid\n", msg->cmd);
            return -EINVAL;

        case RPMBPROXY_ERR_UNIMPLEMENTED:
            ALOGE("cmd 0x%x: is unhandles command\n", msg->cmd);
            return -EINVAL;

        case RPMBPROXY_ERR_GENERIC:
            ALOGE("cmd 0x%x: internal server error\n", msg->cmd);
            return -EIO;

        default:
            ALOGE("cmd 0x%x: unhandled server response %u\n",
                   msg->cmd, msg->result);
    }

    return -EIO;
}

static int request_send(const struct iovec *tx_iovs, uint tx_iovcnt,
                         const struct iovec *rx_iovs, uint rx_iovcnt)
{
    int rc;

    rc = writev(session, tx_iovs, tx_iovcnt);
    if (rc < 0) {
        rc = -errno;
        ALOGE("failed to send request: %s\n", strerror(errno));
        return rc;
    }


    rc = readv(session, rx_iovs, rx_iovcnt);
    if (rc < 0) {
        rc = -errno;
        ALOGE("failed to recv response: %s\n", strerror(errno));
        return rc;
    }


    return rc;
}



static int rpmbproxy_mac(struct rpmb_packet *packet, size_t packet_count,
                    struct rpmb_key *mac)
{
    struct rpmbproxy_msg msg = { .cmd = RPMBPROXY_MAC};
    struct rpmbproxy_mac_req req = { .rpmb_packet_num = packet_count };
    struct iovec tx[3] = {{&msg, sizeof(msg)}, {&req, sizeof(req)},
        {(void *)packet, sizeof(struct rpmb_packet) * packet_count}};
    struct iovec rx[2] = {{&msg, sizeof(msg)}, {(void *)mac, sizeof(struct rpmb_key)}};

    int rc = request_send(tx, 3, rx, 2);
    if(rc >= 0) {
        rc = check_res(&msg, rc);
    }
    return rc;
}

static struct rpmb_nonce rpmbproxy_nonce_init(void)
{
    struct rpmb_nonce rpmb_nonce;

    RAND_bytes(rpmb_nonce.byte, sizeof(rpmb_nonce.byte));

    return rpmb_nonce;
}


static int rpmbproxy_read_counter(uint32_t *write_counter)
{
    int ret;
    struct rpmb_key mac;
    struct rpmb_nonce nonce = rpmbproxy_nonce_init();
    struct rpmb_packet cmd = {
        .nonce = nonce,
        .req_resp = rpmb_u16(RPMB_REQ_GET_COUNTER),
    };
    struct rpmb_packet res;

    //cmd->blocks = 1,the block count must be set to 1.
    ret = rpmb_ops_send(mmc_handle, NULL, (size_t)0, (void *)&cmd, (size_t)sizeof(cmd), (void *)&res, (size_t)sizeof(res));
    if (ret < 0) {
        ALOGE("rpmb_read_counter failed\n");
        return ret;
    }

    ret = rpmbproxy_mac(&res, 1, &mac);
    if (ret < 0) {
        ALOGE("rpmb_read_counter check mac failed\n");
        return ret;
    }

    ret = rpmb_check_res("read counter", RPMB_RESP_GET_COUNTER,
                              &res, 1, &mac, &nonce, NULL);
    if (write_counter)
        *write_counter = rpmb_get_u32(res.write_counter);

    return ret;
}

int rpmb_read(unsigned char *buf, int blk_ind, int blk_cnt)
{
    int i;
    int ret;
    struct rpmb_key mac;
    struct rpmb_nonce nonce = rpmbproxy_nonce_init();
    struct rpmb_packet cmd = {
        .nonce = nonce,
        .address = rpmb_u16(blk_ind),
        .req_resp = rpmb_u16(RPMB_REQ_DATA_READ),
    };
    struct rpmb_packet res[blk_cnt];
    uint8_t *bufp;
    ssize_t read_size = 0;

    if (!init_done) {
        ALOGE("not init,please call init_rpmb()\n");
        return -EINVAL;
    }

    if (blk_cnt > MAX_RPMB_BLOCK_CNT) {
        ALOGE("the block count is to lager: %d max %d\n", blk_cnt, MAX_RPMB_BLOCK_CNT);
        return -EINVAL;
    }

    if (blk_ind > RPMB_BLOCK_IND_END || blk_ind < RPMB_BLOCK_IND_START) {
        ALOGE("the block index %d is out of bounds: %d - %d\n",
            blk_ind, RPMB_BLOCK_IND_START, RPMB_BLOCK_IND_END);
        return -EINVAL;
    }


    memset(res, 0x0, sizeof(res));
    ret = rpmb_ops_send(mmc_handle, NULL, (size_t)0, (void *)&cmd, (size_t)sizeof(cmd), (void *)res, (size_t)sizeof(res));
    if (ret < 0) {
        ALOGE("failed to write rpmb: %s\n", strerror(errno));
        return ret;
    }
    ret = rpmbproxy_mac(res, blk_cnt, &mac);
    if (ret < 0) {
        ALOGE("failed to get mac: %d\n", ret);
        return ret;
    }


    ALOGI("rpmb: read data, addr %d, count %d, response:\n", blk_ind, blk_cnt);

    ret = rpmb_check_res("read data", RPMB_RESP_DATA_READ, res, blk_cnt, &mac, &nonce, &blk_ind);
    if (ret < 0)
        return ret;

    for (bufp = buf, i = 0; i < blk_cnt; i++, bufp += sizeof(res[i].data)){
        memcpy(bufp, res[i].data, sizeof(res[i].data));
        read_size += sizeof(res[i].data);
    }

    return read_size;
}


int rpmb_write(const unsigned char *buf, int blk_ind, int blk_cnt)
{
    int i;
    int ret;
    struct rpmb_key mac;
    struct rpmb_packet cmd[blk_cnt];
    struct rpmb_packet rescmd = {
        .req_resp = rpmb_u16(RPMB_REQ_RESULT_READ),
    };
    struct rpmb_packet res;
    uint32_t write_counter = 0;
    ssize_t write_size = 0;

    if (!init_done) {
        ALOGE("not init,please call init_rpmb()\n");
        return -EINVAL;
    }

    if (blk_cnt > MAX_RPMB_BLOCK_CNT) {
        ALOGE("the block count is to lager: %d max %d\n", blk_cnt, MAX_RPMB_BLOCK_CNT);
        return -EINVAL;
    }

    if (blk_ind > RPMB_BLOCK_IND_END || blk_ind < RPMB_BLOCK_IND_START) {
        ALOGE("the block index %d is out of bounds: %d - %d\n",
            blk_ind, RPMB_BLOCK_IND_START, RPMB_BLOCK_IND_END);
        return -EINVAL;
    }

    ret = rpmbproxy_read_counter(&write_counter);
    if (ret < 0) {
        ALOGE("failed to read write counter: %d\n", ret);
        return ret;
    }

    for (i = 0; i < blk_cnt; i++) {
        memset(&cmd[i], 0, sizeof(cmd[i]));
        memcpy(cmd[i].data, buf + i * sizeof(cmd[i].data), sizeof(cmd[i].data));
        write_size += sizeof(cmd[i].data);
        cmd[i].write_counter = rpmb_u32(write_counter);
        cmd[i].address = rpmb_u16(blk_ind);
        cmd[i].block_count = rpmb_u16(blk_cnt);
        cmd[i].req_resp = rpmb_u16(RPMB_REQ_DATA_WRITE);
    }

    ret = rpmbproxy_mac(cmd, blk_cnt, &cmd[blk_cnt - 1].key_mac);
    if (ret < 0) {
        ALOGE("failed to get mac: %d\n", ret);
        return ret;
    }

    memset(&res, 0x0, sizeof(res));
    ret = rpmb_ops_send(mmc_handle, (void *)cmd, (size_t)sizeof(cmd), (void *)&rescmd, (size_t)sizeof(rescmd), (void *)&res, (size_t)sizeof(res));
    if (ret < 0) {
        ALOGE("failed to write rpmb: %s\n", strerror(errno));
        return ret;
    }

    ret = rpmbproxy_mac(&res, 1, &mac);
    if (ret < 0) {
        ALOGE("failed to get mac: %d\n", ret);
        return ret;
    }

    ALOGI("rpmb: write data, addr %d, count %d, write_counter %d, response\n",
                        blk_ind, blk_cnt, write_counter);


    ret = rpmb_check_res("write data", RPMB_RESP_DATA_WRITE, &res, 1, &mac, NULL, &blk_ind);
    if (ret < 0) {
        if (rpmb_get_u16(res.result) == RPMB_RES_COUNT_FAILURE)
            write_counter = 0;
        return ret;
    }

    return write_size;
}


int init_rpmb()
{
    int ret;

    ret = rpmb_ops_open(RPMB_DEV_PATH);
    if (0 > ret){
        return ret;
    }
    mmc_handle = ret;

    ret = tipc_connect(TRUSTY_DEVICE_NAME, RPMBPROXY_PORT);
    if (0 > ret) {
        ALOGE("tipc_connect(%s, %s) failed!\n", TRUSTY_DEVICE_NAME, RPMBPROXY_PORT);
        return ret;
    }

    session = ret;
    init_done = 1;

    return 0;
}

void release_rpmb()
{
    rpmb_ops_close(mmc_handle);
    mmc_handle = -1;

    tipc_close(session);
    session = -1;

    init_done = -1;
}
