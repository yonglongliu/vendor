/*
 * Copyright (C) 2017 spreadtrum
 */

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <sys/uio.h>
#include <errno.h>
#include <unistd.h>



#include <trusty/tipc.h>
#include <trusty/lib/storage.h>

#define TRUSTY_DEVICE_NAME "/dev/trusty-ipc-dev0"
#define STORAGE_UNITTEST_PROXY_PORT "com.android.trusty.storage-unittest"
#define STORAGE_CLIENT_TD_PORT     "com.android.trusty.storage.client.td"
#define STORAGE_CLIENT_TDEA_PORT   "com.android.trusty.storage.client.tdea"
#define STORAGE_CLIENT_TP_PORT     "com.android.trusty.storage.client.tp"


struct storage_unittest_msg {
	uint32_t cmd;
	uint32_t size;
	int32_t  result;
	uint32_t __reserved;
	uint8_t  payload[0];
};

enum storage_unittest_cmd {
	STORAGE_UNITTEST_REQ_SHIFT = 1,
	STORAGE_UNITTEST_RESP_BIT  = 1,

	STORAGE_UNITTEST_RESP_ERR   = STORAGE_UNITTEST_RESP_BIT,

	STORAGE_UNITTEST_ALL          = 1 << STORAGE_UNITTEST_REQ_SHIFT,
	STORAGE_UNITTEST_TD           = 2 << STORAGE_UNITTEST_REQ_SHIFT,
	STORAGE_UNITTEST_TP           = 3 << STORAGE_UNITTEST_REQ_SHIFT,
	STORAGE_UNITTEST_TDEA         = 4 << STORAGE_UNITTEST_REQ_SHIFT,
	STORAGE_UNITTEST_TD_NSW_SER   = 5 << STORAGE_UNITTEST_REQ_SHIFT,
	STORAGE_UNITTEST_TP_NSW_SER   = 6 << STORAGE_UNITTEST_REQ_SHIFT,
	STORAGE_UNITTEST_TDEA_NSW_SER = 7 << STORAGE_UNITTEST_REQ_SHIFT,
	STORAGE_UNITTEST_TD_RENAME    = 8 << STORAGE_UNITTEST_REQ_SHIFT,
	STORAGE_UNITTEST_TP_RENAME    = 9 << STORAGE_UNITTEST_REQ_SHIFT,
	STORAGE_UNITTEST_TDEA_RENAME  = 10 << STORAGE_UNITTEST_REQ_SHIFT,
	SPRDIMGVERSION_UNITTEST_GET   = 11 << STORAGE_UNITTEST_REQ_SHIFT,
	SPRDIMGVERSION_UNITTEST_SET   = 12 << STORAGE_UNITTEST_REQ_SHIFT,

};

enum storage_unittest_res {
	UNITTEST_RES_COMPLETE       = 0,
	UNITTEST_RES_NOT_VALID      = 1,
	UNITTEST_RES_UNIMPLEMENTED  = 2,
	UNITTEST_RES_OPEN_FILE_FAIL = 3,
	UNITTEST_RES_OPEN_SESS_FAIL = 4,
	UNITTEST_RES_READ_FILE_FAIL = 4,
};


#define dprintf(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)


static ssize_t send_request(storage_session_t session,
                         const struct iovec *tx_iovs, uint tx_iovcnt,
                         const struct iovec *rx_iovs, uint rx_iovcnt)
{
    ssize_t rc;

    rc = writev(session, tx_iovs, tx_iovcnt);
    if (rc < 0) {
        rc = -errno;
        dprintf("failed to send request: %s\n", strerror(errno));
        return rc;
    }

    rc = readv(session, rx_iovs, rx_iovcnt);
    if (rc < 0) {
        rc = -errno;
        dprintf("failed to recv response: %s\n", strerror(errno));
        return rc;
    }

    return rc;
}

int storage_unittest_nsw_ser(storage_session_t session, enum storage_unittest_cmd unittest_cmd,
								const char *port, const char *fname)
{
	storage_session_t session_;
	struct storage_unittest_msg msg = { .cmd = unittest_cmd};
	char w_buf[4] = {0x1,0x2,0x3,0x4};
	char r_buf[4];
    struct iovec tx[2] = {{&msg, sizeof(msg)}, {(void *)fname, strlen(fname)}};
    struct iovec rx[2] = {{&msg, sizeof(msg)}, {(void *)r_buf, sizeof(r_buf)}};
	file_handle_t handle;
	int i = 0,rc = -1;

	rc = storage_open_session(TRUSTY_DEVICE_NAME, &session_, port);
    if (0 > rc) {
        dprintf("storage_open_session(%s) failed!\n", port);
		return -1;
	}

	rc = storage_open_file(session_, &handle, fname,
                        STORAGE_FILE_OPEN_CREATE,
                        STORAGE_OP_COMPLETE);
    if ( 0 > rc) {
        dprintf("storage_open_file(%s) failed! return %d\n", fname, rc);

		goto err_open_file;
	}

	rc = storage_write(handle, 0, w_buf, sizeof(w_buf), STORAGE_OP_COMPLETE);
	if (rc != sizeof(w_buf)) {
        dprintf("storage_write(%s) failed! return %d\n", fname, rc);
		goto err_write_file;
	}

	dprintf("storage_write(%s) success!\n", fname);

	storage_close_file(handle);

	storage_end_transaction(session_, true);

	storage_close_session(session_);

	sleep(1);
/*
	rc = storage_open_session(TRUSTY_DEVICE_NAME, &session_, port);
    if (0 > rc) {
        dprintf("storage_open_session(%s) failed!\n", port);
		return -1;
	}

	rc = storage_open_file(session_, &handle, fname,
                        0,
                        0);
    if ( 0 > rc) {
        dprintf("storage_open_file(%s) failed! return %d\n", fname, rc);

		goto err_open_file;
	}

	rc = storage_read(handle, 0, r_buf, sizeof(r_buf));
	if (rc != sizeof(r_buf)) {
        dprintf("storage_read(%s) failed! return %d\n", fname, rc);
		goto err_write_file;
	}
	for (i = 0; i < rc; i++) {
		if (r_buf[i] != w_buf[i]) {
			dprintf("check storage_read file data failed!\n");
		}
	}
	if (i == rc) {
		dprintf("check storage_read file data %x %x %x %x success!\n",
			r_buf[0], r_buf[1], r_buf[2], r_buf[3]);
	}

	storage_close_file(handle);

	storage_close_session(session_);
*/

	rc = send_request(session, tx, 2, rx, 2);
	if (0 > rc) {
		dprintf("send_request failed!\n");
		return rc;
	}

	if (UNITTEST_RES_COMPLETE != msg.result) {
		dprintf("storage unittest (cmd=%d) failed! result %d\n", unittest_cmd, msg.result);
		return -1;
	}

	for (i = 0; i < 4; i++) {
		if (r_buf[i] != w_buf[i]) {
			dprintf("check file data failed!\n");
			break;
		}
	}
    if (i == 4) {
		dprintf("storage unittest (cmd=%d) success!\n", unittest_cmd);
    }

	return rc;

err_write_file:
	storage_close_file(handle);

err_open_file:
	storage_close_session(session_);

	return rc;


}


int storage_unittest(storage_session_t session, enum storage_unittest_cmd unittest_cmd)
{
	struct storage_unittest_msg msg = { .cmd = unittest_cmd};
    struct iovec tx[1] = {{&msg, sizeof(msg)}};
    struct iovec rx[1] = {{&msg, sizeof(msg)}};
	int rc = -1;

    rc = send_request(session, tx, 1, rx, 1);
	if (0 > rc) {
		dprintf("send_request failed!\n");
		return rc;
	}


	if (UNITTEST_RES_COMPLETE == msg.result) {
		dprintf("storage unittest (cmd=%d) success!\n", unittest_cmd);
	} else {
		dprintf("storage unittest (cmd=%d) failed! result %d\n", unittest_cmd, msg.result);
		rc = -1;
	}

	return rc;
}


int main(void)
{
	storage_session_t session;
	//const char *fname = "test_file";
    int rc = -1;

	rc = tipc_connect(TRUSTY_DEVICE_NAME, STORAGE_UNITTEST_PROXY_PORT);
    if (0 > rc) {
        dprintf("tipc_connect(%s, %s) failed!\n", TRUSTY_DEVICE_NAME, STORAGE_UNITTEST_PROXY_PORT);
		return -1;
	}

	session = (storage_session_t)rc;

	//storage_unittest(session, STORAGE_UNITTEST_ALL);
	//storage_unittest(session, STORAGE_UNITTEST_TD);
	//storage_unittest(session, STORAGE_UNITTEST_TDEA);
	//storage_unittest(session, STORAGE_UNITTEST_TP);

	//storage_unittest_nsw_ser(session, STORAGE_UNITTEST_TDEA_NSW_SER, STORAGE_CLIENT_TDEA_PORT, fname);
	//storage_unittest_nsw_ser(session, STORAGE_UNITTEST_TD_NSW_SER, STORAGE_CLIENT_TD_PORT, fname);
	//storage_unittest_nsw_ser(session, STORAGE_UNITTEST_TP_NSW_SER, STORAGE_CLIENT_TP_PORT, fname);

    //storage_unittest(session, STORAGE_UNITTEST_TD_RENAME);

	//storage_unittest(session, SPRDIMGVERSION_UNITTEST_SET);
	storage_unittest(session, SPRDIMGVERSION_UNITTEST_GET);

	tipc_close(session);


	return rc;

}
