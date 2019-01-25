/*
 * Copyright (C) 2016 The Android Open Source Project
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

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdio.h>
#include <sys/time.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#include <trusty/lib/storage.h>

#define TRUSTY_DEVICE_NAME "/dev/trusty-ipc-dev0"

#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))

#define dprintf(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)

#define STORAGE_CLIENT_TD_PORT     "com.android.trusty.storage.client.td"
#define STORAGE_CLIENT_TDEA_PORT   "com.android.trusty.storage.client.tdea"
#define STORAGE_CLIENT_TP_PORT     "com.android.trusty.storage.client.tp"


static void fill_pattern32(uint32_t *buf, size_t len, storage_off_t off)
{
    size_t cnt = len / sizeof(uint32_t);
    uint32_t pattern = (uint32_t)(off / sizeof(uint32_t));
    while (cnt--) {
        *buf++ = pattern++;
    }
}

static bool check_pattern32(const uint32_t *buf, size_t len, storage_off_t off)
{
    size_t cnt = len / sizeof(uint32_t);
    uint32_t pattern = (uint32_t)(off / sizeof(uint32_t));
    while (cnt--) {
        if (*buf != pattern)
            return false;
        buf++;
        pattern++;
    }
    return true;
}


size_t WritePatternChunk(file_handle_t handle, storage_off_t off, size_t chunk_len, bool complete)
{
    size_t rc;
    uint32_t data_buf[chunk_len/sizeof(uint32_t)];
//    struct timeval start, end;

//    gettimeofday(&start, NULL);


    fill_pattern32(data_buf, chunk_len, off);

    rc = storage_write(handle, off, data_buf, sizeof(data_buf),
            complete ? STORAGE_OP_COMPLETE : STORAGE_OP_COMPLETE); //0
    if (rc != chunk_len) {
        dprintf("storage_write() faile, write lenth %d, expect %d\n", rc, chunk_len);
    }
//   gettimeofday(&end, NULL);
//    dprintf("storage_write(%d) use %ld us\n", sizeof(data_buf),
//        1000000 * ( end.tv_sec - start.tv_sec ) + (end.tv_usec - start.tv_usec));

    return rc;
}

int WritePattern(file_handle_t handle, storage_off_t off,
                                      size_t data_len, size_t chunk_len, bool complete)
{
    size_t write_len = 0;

    while (data_len) {
        if (data_len < chunk_len)
            chunk_len = data_len;

        write_len = WritePatternChunk(handle, off, chunk_len, ((chunk_len == data_len) && complete));
        if (write_len != chunk_len) {
          return -1;
        }
        off += chunk_len;
        data_len -= chunk_len;
    }

    return 0;
}



int ReadPattern(file_handle_t handle, storage_off_t off,
                                     size_t data_len, size_t chunk_len)
{
    size_t read_len = 0;
    uint32_t data_buf[chunk_len/sizeof(uint32_t)];

    while (data_len) {
        if (chunk_len > data_len)
            chunk_len = data_len;
        read_len = storage_read(handle, off, data_buf, sizeof(data_buf));
        if (read_len != chunk_len){
            dprintf("storage_read() failed, read lenth %d, expect %d\n", read_len, chunk_len);
            return -1;
        } else {
            bool ret = check_pattern32(data_buf, chunk_len, off);
            if ( !ret) {
                dprintf("ReadPattern() check pattern failed!\n");
                return -1;
            }
        }
        off += chunk_len;
        data_len -= chunk_len;
    }

    return 0;
}


int test_storage(const char *port, size_t data_len, size_t chunk_len)
{
    const char *fname = "test_file";
    storage_session_t session_;
    file_handle_t handle;
    int rc = -1;
    struct timeval start, end;

    gettimeofday(&start, NULL);

    //STORAGE_CLIENT_TP_PORT STORAGE_CLIENT_TDEA_PORT  STORAGE_CLIENT_TD_PORT
    rc = storage_open_session(TRUSTY_DEVICE_NAME, &session_, port);
    if (0 > rc) {
        dprintf("storage_open_session(%s) failed!\n", port);
        return -1;
    }
    gettimeofday(&end, NULL);
    dprintf("storage_open_session(%s) use %ld us\n", port,
        1000000 * ( end.tv_sec - start.tv_sec ) + (end.tv_usec - start.tv_usec));

    // make sure test file does not exist (expect success or -ENOENT)
    rc = storage_delete_file(session_, fname, STORAGE_OP_COMPLETE);

    //dprintf("storage_delete_file(%s) return %d\n", fname, rc);

    gettimeofday(&start, NULL);
    rc = storage_open_file(session_, &handle, fname,
                        STORAGE_FILE_OPEN_CREATE | STORAGE_FILE_OPEN_CREATE_EXCLUSIVE,
                        STORAGE_OP_COMPLETE);
    if ( 0 > rc) {
        dprintf("storage_open_file(%s) failed! return %d\n", fname, rc);
        storage_close_session(session_);
        return -1;
    }
    gettimeofday(&end, NULL);
    dprintf("storage_open_file(%s) use %ld us\n", fname,
            1000000 * ( end.tv_sec - start.tv_sec ) + (end.tv_usec - start.tv_usec));

    gettimeofday(&start, NULL);
    rc = WritePattern(handle, 0, data_len, chunk_len, true);
    if ( 0 != rc) {
        dprintf("WritePatternChunk() failed!\n");
        goto out;
    }
    gettimeofday(&end, NULL);
    dprintf("write(%d) use %ld us\n", data_len,
        1000000 * ( end.tv_sec - start.tv_sec ) + (end.tv_usec - start.tv_usec));

    rc = ReadPattern(handle, 0, data_len, chunk_len);
    if ( 0 != rc) {
        dprintf("ReadPattern() failed!\n");
        goto out;
    }

out:
    gettimeofday(&start, NULL);
    storage_close_file(handle);
    gettimeofday(&end, NULL);
    dprintf("storage_close_file(%s) use %ld us\n", fname,
        1000000 * ( end.tv_sec - start.tv_sec ) + (end.tv_usec - start.tv_usec));

    gettimeofday(&start, NULL);
    storage_delete_file(session_, fname, STORAGE_OP_COMPLETE);
    gettimeofday(&end, NULL);
    dprintf("storage_delete_file(%s) use %ld us\n", fname,
        1000000 * ( end.tv_sec - start.tv_sec ) + (end.tv_usec - start.tv_usec));

    gettimeofday(&start, NULL);
    storage_close_session(session_);
    gettimeofday(&end, NULL);
    dprintf("storage_close_session(%s) use %ld us\n", port,
        1000000 * ( end.tv_sec - start.tv_sec ) + (end.tv_usec - start.tv_usec));

    return rc;
}

int test_storage_write(const char *port,  char *fname, char *data_buf, int data_len, storage_off_t  offset, bool complete)
{
    storage_session_t session_;
    file_handle_t handle;
    int rc = -1;
    struct timeval start, end;

    rc = storage_open_session(TRUSTY_DEVICE_NAME, &session_, port);
    if (0 > rc) {
        dprintf("storage_open_session(%s) failed!\n", port);
        return rc;
    }

    gettimeofday(&start, NULL);
    rc = storage_open_file(session_, &handle, fname,
                        STORAGE_FILE_OPEN_CREATE,
                        complete ? STORAGE_OP_COMPLETE : 0);
    if ( 0 > rc) {
        dprintf("storage_open_file(%s) failed! return %d\n", fname, rc);
        goto err1;
    }

	rc = storage_write(handle, offset, data_buf, data_len,
                        complete ? STORAGE_OP_COMPLETE : 0);
    if ( rc != data_len) {
        dprintf("storage_write() failed! return %d,exp %d\n", rc, data_len);
		rc = -1;
        goto out;
    }

out:
    storage_close_file(handle);
    storage_end_transaction(session_, STORAGE_OP_COMPLETE);
err1:
    storage_close_session(session_);


    gettimeofday(&end, NULL);
    dprintf("test_storage_write(%s,%s,%d,) use %ld us\n", port, fname, data_len,
        1000000 * ( end.tv_sec - start.tv_sec ) + (end.tv_usec - start.tv_usec));

    return rc;
}

int test_storage_read(const char *port,  const char *fname, char *data_buf, storage_off_t data_len, storage_off_t  offset, bool complete)
{
    storage_session_t session_;
    file_handle_t handle;
    int rc = -1;
    struct timeval start, end;
    storage_off_t  file_size = 0,read_len = 0;

    rc = storage_open_session(TRUSTY_DEVICE_NAME, &session_, port);
    if (0 > rc) {
        dprintf("storage_open_session(%s) failed!\n", port);
        return rc;
    }


    gettimeofday(&start, NULL);
    rc = storage_open_file(session_, &handle, fname, 0,
                        complete ? STORAGE_OP_COMPLETE : 0);
    if ( 0 > rc) {
        dprintf("storage_open_file(%s) failed! return %d\n", fname, rc);
        goto err1;
    }

    rc = storage_get_file_size(handle, &file_size);
    if ( 0 > rc) {
        dprintf("storage_get_file_size(%s) failed! return %d\n", fname, rc);
        goto out;
    }

	if (data_len > (file_size - offset)) {
		data_len = file_size - offset;
	}
	read_len = storage_read(handle, offset, data_buf, data_len);
    if ( read_len != data_len) {
        dprintf("storage_read() failed! return %lld,exp %lld\n", read_len, data_len);
		rc = -1;
        goto out;
    }

out:
    storage_close_file(handle);
err1:
    storage_close_session(session_);


    gettimeofday(&end, NULL);
    dprintf("test_storage_read(%s, %s, file_size %lld, read %lld,) use %ld us\n", port, fname, file_size, read_len,
        1000000 * ( end.tv_sec - start.tv_sec ) + (end.tv_usec - start.tv_usec));

    return rc;
}

int test_storage_speed(const char *port, char *data_buf, int data_len, bool complete)
{
    const char *fname = "test_file";
    storage_session_t session_;
    file_handle_t handle;
    int rc = -1;
    struct timeval start, end;

    rc = storage_open_session(TRUSTY_DEVICE_NAME, &session_, port);
    if (0 > rc) {
        dprintf("storage_open_session(%s) failed!\n", port);
        return rc;
    }


    // make sure test file does not exist (expect success or -ENOENT)
    rc = storage_delete_file(session_, fname, STORAGE_OP_COMPLETE);

    //dprintf("storage_delete_file(%s) return %d\n", fname, rc);

    gettimeofday(&start, NULL);
    rc = storage_open_file(session_, &handle, fname,
                        STORAGE_FILE_OPEN_CREATE | STORAGE_FILE_OPEN_CREATE_EXCLUSIVE,
                        complete ? STORAGE_OP_COMPLETE : 0);
    if ( 0 > rc) {
        dprintf("storage_open_file(%s) failed! return %d\n", fname, rc);
        storage_close_session(session_);
        goto err1;
    }

	rc = storage_write(handle, 0, data_buf, data_len,
                        complete ? STORAGE_OP_COMPLETE : 0);
    if ( rc != data_len) {
        dprintf("storage_write() failed! return %d,exp %d\n", rc, data_len);
		rc = -1;
        goto out;
    }

out:
    storage_close_file(handle);
    storage_end_transaction(session_, STORAGE_OP_COMPLETE);
err1:
    storage_close_session(session_);


    gettimeofday(&end, NULL);
    dprintf("test_storage_speed(%s,%s, %dk,) use %ld us\n", port, fname, data_len/1024,
        1000000 * ( end.tv_sec - start.tv_sec ) + (end.tv_usec - start.tv_usec));

    return rc;
}

int test_file_speed(const char *fname, char *data_buf, int data_len)
{
    int rc = -1, fd;
    struct timeval start, end;
    gettimeofday(&start, NULL);

    rc = open(fname, O_RDWR|O_CREAT, S_IRUSR | S_IWUSR);
    if (0 > rc) {
        dprintf("open(%s) failed!\n", fname);
        goto err;
    }
	fd = rc;


	rc = pwrite(fd, data_buf, data_len, 0);
    if ( rc != data_len) {
        dprintf("pwrite() failed! return %d,exp %d\n", rc, data_len);
		rc = -1;
        goto out;
    }

	rc = fsync(fd);
    if (0 > rc) {
		rc = errno;
        dprintf("fsync(%d) failed!, %s\n", fd, strerror(errno));
    }

out:
    close(fd);
	unlink(fname);
err:
    gettimeofday(&end, NULL);
    dprintf("test_file_speed(%s,%dk,) use %ld us\n", fname, data_len/1024,
        1000000 * ( end.tv_sec - start.tv_sec ) + (end.tv_usec - start.tv_usec));

    return rc;
}

int test_file_rename_add(const char *port,const char *old_file_name, const char *new_file_name)
{
    storage_session_t session_;
    file_handle_t handle;
    int rc = -1;
    struct timeval start, end;
    storage_off_t  data_len = 4024 * 3, chunk_len = 4024;


    gettimeofday(&start, NULL);

    rc = storage_open_session(TRUSTY_DEVICE_NAME, &session_, port);
    if (0 > rc) {
        dprintf("storage_open_session(%s) failed!\n", port);
        return rc;
    }

    rc = storage_delete_file(session_, old_file_name, STORAGE_OP_COMPLETE);
	rc = storage_delete_file(session_, new_file_name, STORAGE_OP_COMPLETE);

    rc = storage_open_file(session_, &handle, old_file_name, STORAGE_FILE_OPEN_CREATE, 0);
    if ( 0 > rc) {
        dprintf("storage_open_file(%s) failed! return %d\n", old_file_name, rc);
        goto err1;
    }

	rc = WritePattern(handle, 0, data_len, chunk_len, 0);
    if ( 0 > rc) {
        dprintf("WritePatternChunk() failed!\n");
        goto out;
    }

    storage_close_file(handle);



	rc = storage_rename_file(session_, old_file_name, new_file_name, STORAGE_OP_COMPLETE);
    if ( 0 > rc) {
        dprintf("storage_rename_file(%s,%s) failed! return %d\n", old_file_name, new_file_name, rc);
        storage_close_session(session_);
        return rc;
    }

    storage_end_transaction(session_, STORAGE_OP_COMPLETE);
    storage_close_session(session_);


    rc = storage_open_session(TRUSTY_DEVICE_NAME, &session_, port);
    if (0 > rc) {
        dprintf("storage_open_session(%s) failed!\n", port);
        return rc;
    }

    rc = storage_open_file(session_, &handle, new_file_name, 0, 0);
    if ( 0 > rc) {
        dprintf("storage_open_file(%s) failed! return %d\n", new_file_name, rc);
        goto err1;
    }

    rc = ReadPattern(handle, 0, data_len, chunk_len);
    if ( 0 != rc) {
	    dprintf("ReadPattern() failed!\n");
	    goto out;
    }

out:
    storage_close_file(handle);
err1:
    storage_close_session(session_);


    gettimeofday(&end, NULL);
    dprintf("test_file_rename(%s) use %ld us\n", port,
        1000000 * ( end.tv_sec - start.tv_sec ) + (end.tv_usec - start.tv_usec));

    return rc;
}

int test_file_rename_infs(const char *port, const char *old_file_name, const char *new_file_name)
{
    storage_session_t session_;
    file_handle_t handle;
    int rc = -1;
    struct timeval start, end;
    storage_off_t  data_len = 4024 * 3, chunk_len = 4024;

    gettimeofday(&start, NULL);

    rc = storage_open_session(TRUSTY_DEVICE_NAME, &session_, port);
    if (0 > rc) {
        dprintf("storage_open_session(%s) failed!\n", port);
        return rc;
    }

	rc = storage_delete_file(session_, old_file_name, STORAGE_OP_COMPLETE);
	rc = storage_delete_file(session_, new_file_name, STORAGE_OP_COMPLETE);

    rc = storage_open_file(session_, &handle, old_file_name, STORAGE_FILE_OPEN_CREATE, 0);
    if ( 0 > rc) {
        dprintf("storage_open_file(%s) failed! return %d\n", old_file_name, rc);
        goto err1;
    }

	rc = WritePattern(handle, 0, data_len, chunk_len, 0);
    if ( 0 > rc) {
        dprintf("WritePatternChunk() failed!\n");
        goto out;
    }

    storage_close_file(handle);
    storage_end_transaction(session_, STORAGE_OP_COMPLETE);
    storage_close_session(session_);

    sleep(1);

    //rename
    rc = storage_open_session(TRUSTY_DEVICE_NAME, &session_, port);
    if (0 > rc) {
        dprintf("storage_open_session(%s) failed!\n", port);
        return rc;
    }
	rc = storage_rename_file(session_, old_file_name, new_file_name, STORAGE_OP_COMPLETE);
    if ( 0 > rc) {
        dprintf("storage_rename_file(%s,%s) failed! return %d\n", old_file_name, new_file_name, rc);
        storage_close_session(session_);
        return rc;
    }
    storage_end_transaction(session_, STORAGE_OP_COMPLETE);
    storage_close_session(session_);


    rc = storage_open_session(TRUSTY_DEVICE_NAME, &session_, port);
    if (0 > rc) {
        dprintf("storage_open_session(%s) failed!\n", port);
        return rc;
    }

    rc = storage_open_file(session_, &handle, new_file_name, 0, 0);
    if ( 0 > rc) {
        dprintf("storage_open_file(%s) failed! return %d\n", new_file_name, rc);
        goto err1;
    }

    rc = ReadPattern(handle, 0, data_len, chunk_len);
    if ( 0 != rc) {
	    dprintf("ReadPattern() failed!\n");
	    goto out;
    }

out:
    storage_close_file(handle);
err1:
    storage_close_session(session_);

    gettimeofday(&end, NULL);
    dprintf("test_file_rename(%s) use %ld us\n", port,
        1000000 * ( end.tv_sec - start.tv_sec ) + (end.tv_usec - start.tv_usec));

    return rc;
}





int main(void)
{

    int rc = -1;
/*


    rc = test_storage(STORAGE_CLIENT_TP_PORT, 512, 512);
    dprintf("test %s: %s!\n", STORAGE_CLIENT_TP_PORT, (rc == 0 ? "success" : "failed"));

    rc = test_storage(STORAGE_CLIENT_TDEA_PORT, 512, 512);
    dprintf("test %s: %s!\n", STORAGE_CLIENT_TDEA_PORT, (rc == 0 ? "success" : "failed"));

    rc = test_storage(STORAGE_CLIENT_TD_PORT, 4040, 4040);
    dprintf("test %s: %s!\n", STORAGE_CLIENT_TD_PORT, (rc == 0 ? "success" : "failed"));
    dprintf("---------------------------------------------\n");
*/

	int data_len = 4024;
	char data_buf[data_len * 1000];
	memset(data_buf, 0xFF, data_len * 1000);

/*
	test_storage_speed(STORAGE_CLIENT_TD_PORT, data_buf, 4024 * 100, true);
    dprintf("---------------------------------------------\n");
	for (int i = 1; i < 10; i++) {
		test_storage_speed(STORAGE_CLIENT_TD_PORT, data_buf, data_len*10, true);
		test_storage_speed(STORAGE_CLIENT_TD_PORT, data_buf, data_len*10, false);
		test_file_speed("/data/ss/test_file", data_buf, data_len*10);
	}

    char file_name[20];
    for (int i = 1; i < 10; i++) {
		sprintf(file_name,"test_file_%d", i);
        test_storage_read(STORAGE_CLIENT_TD_PORT,file_name, data_buf, 4024 * 100, 0, false);
    }
 */
    const char *old_file_name = "test_file_rename_1";
    const char *new_file_name = "test_file_rename_2";

    test_storage_read(STORAGE_CLIENT_TD_PORT, old_file_name, data_buf, 4024 * 3, 0, false);
    test_storage_read(STORAGE_CLIENT_TD_PORT, new_file_name, data_buf, 4024 * 3, 0, false);

	//rc = test_file_rename_add(STORAGE_CLIENT_TD_PORT, old_file_name, new_file_name);
	//dprintf("test_file_rename(%s): %s!\n", STORAGE_CLIENT_TD_PORT, (rc == 0 ? "success" : "failed"));
    return rc;

}
