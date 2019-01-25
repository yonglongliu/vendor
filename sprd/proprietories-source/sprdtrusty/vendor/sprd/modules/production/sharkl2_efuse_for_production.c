/*
 * Copyright (C) 2012 Spreadtrum Communications Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
/*
*  sprd_efuse_hw.c
*
*  Created on: 2014-06-05
*  Author: ronghua.yu
*  Modify: xianke.cui 2014-10-23
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <getopt.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <cutils/log.h>
//#include <sprd_efuse_hw.h>


#define LOG_TAG "TrustyProduction"

#define SECURE_BOOT_BLOCK (2)
#define SECURE_BOOT_BIT (0)
#define BLOCK_SIZE (4)

#undef DIS_EFUSE_DEBUG
#define DIS_EFUSE_DEBUG 1
#if DIS_EFUSE_DEBUG
#define DEV_NODE "/dev/sprd_otp_ap_efuse"
#else
#define DEV_NODE "/dev/sprd_dummy_otp"
#endif

#define DEV_NODE_MAGIC "/sys/class/misc/sprd_otp_ap_efuse/magic"

#define MAGIC_NUMBER (0x8810)

static int efuse_write(unsigned int blk, unsigned int val) {
  char magic[8] = {0}, buf[16] = {0};
  off_t curpos, offset;
  int fd = -1;
  int len = 0, ret = 0;
  char *pBuf = (char *)&val;

  ALOGD("%s()->Line:%d; blk = %d, data = 0x%08x \n", __FUNCTION__, __LINE__,
        blk, val);

  fd = open(DEV_NODE_MAGIC, O_WRONLY);
  if (fd < 0) {
    ALOGE("CXK MAGIC open fail");
    return -1;
  }
  sprintf(magic, "%x", MAGIC_NUMBER);
  len = write(fd, magic, strlen(magic));
  if (len <= 0) {
    ALOGE("%s()->Line:%d; write magic error\n", __FUNCTION__, __LINE__);
    close(fd);
    return -2;
  }
  close(fd);
  fd = open(DEV_NODE, O_RDWR);
  if (fd < 0) {
    ALOGE("%s()->Line:%d; %s open error, fd = %d. \n", __FUNCTION__, __LINE__,
          DEV_NODE, fd);
    return -3;
  }

  sprintf(buf, "%08x", val);

  offset = blk * BLOCK_SIZE;
  curpos = lseek(fd, offset, SEEK_CUR);
  if (curpos == -1) {
    ALOGE("%s()->Line:%d; lseek error\n", __FUNCTION__, __LINE__);
    close(fd);
    return -4;
  }
  ALOGD("val=%x,buf=%s\n", val, buf);

  len = write(fd, pBuf, 4);
  ALOGD("pBuf[0~3]=%02x,%02x,%02x,%02x\n", pBuf[0], pBuf[1], pBuf[2], pBuf[3]);
  if (len <= 0) {
    ALOGE(
        "%s()->Line:%d; write efuse block(%d) data(%s) error, retcode = %d; \n",
        __FUNCTION__, __LINE__, blk, buf, len);
    close(fd);
    return -5;
  }
  close(fd);
  return len;
}

static int efuse_read(unsigned int blk, unsigned char *data) {
  char buf[5] = {0};
  off_t curpos, offset;
  int fd = -1, ret = 0, i = 0;
  if (data == 0) return -1;

  fd = open(DEV_NODE, O_RDWR);
  if (fd < 0) {
    ALOGE("%s()->Line:%d; %s open error, fd = %d. \n", __FUNCTION__, __LINE__,
          DEV_NODE, fd);
    return -2;
  }
  offset = blk * BLOCK_SIZE;
  curpos = lseek(fd, offset, SEEK_CUR);
  if (curpos == -1) {
    ALOGE("%s()->Line:%d; lseek error\n", __FUNCTION__, __LINE__);
    close(fd);
    return -3;
  }
  ret = read(fd, buf, 4);
  if (ret <= 0) {
    ALOGE("%s()->Line:%d; read efuse block(%d) data error, retcode = %d; \n",
          __FUNCTION__, __LINE__, blk, ret);
    close(fd);
    return -4;
  }
  close(fd);
  sprintf(data, "%02x%02x%02x%02x", buf[3], buf[2], buf[1], buf[0]);
  ALOGD("buf=%s\n", data);
  return 1;
}

int production_efuse_secure_enable(void) {
  int ret = 0;
  ret = efuse_write(SECURE_BOOT_BLOCK, (1 << SECURE_BOOT_BIT));
  if (ret <= 0) return -2;
  return 0;
}

int production_efuse_secure_is_enabled(void) {
    unsigned char value[10] = {0};
    unsigned int _value = 0;
    int ret = efuse_read(SECURE_BOOT_BLOCK, value);
    _value = (unsigned int)(strtoul(value, 0, 16));
    if (ret <= 0) return 0;
    return (_value & (1 << SECURE_BOOT_BIT)) ? 1 : 0;
}
