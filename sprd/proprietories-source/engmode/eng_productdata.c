#include <fcntl.h>
#include "engopt.h"
#include <pthread.h>
#include "eng_productdata.h"

#ifdef CONFIG_NAND
#include <sys/ioctl.h>
#include <ubi-user.h>
#endif

//	Android8.0 miscdata need getfrom  property_get("ro.product.partitionpath", miscdata_path, "")
//#ifdef CONFIG_NAND
//#define PRODUCTINFO_FILE "/dev/ubi0_miscdata"
//#else
//#define PRODUCTINFO_FILE "/dev/block/platform/sdio_emmc/by-name/miscdata"
//#endif

int eng_read_productnvdata(char *databuf, int data_len) {
  int ret = 0;
  int len;
  char prop[128] = {0};
  char miscdata_path[128] = {0};
  int fd = -1;

  if (-1 == property_get("ro.product.partitionpath", prop, "")){
    ENG_LOG("%s: get partitionpath fail\n", __FUNCTION__);
    return 0;
  }

  sprintf(miscdata_path, "%smiscdata", prop);
  fd = open(miscdata_path, O_RDONLY);
  if (fd >= 0) {
    ENG_LOG("%s open Ok miscdata_path = %s ", __FUNCTION__,
            miscdata_path);
    len = read(fd, databuf, data_len);

    if (len <= 0) {
      ret = 1;
      ENG_LOG("%s read fail miscdata_path = %s ", __FUNCTION__,
              miscdata_path);
    }
    close(fd);
  } else {
    ENG_LOG("%s open fail miscdata_path = %s ", __FUNCTION__,
            miscdata_path);
    ret = 1;
  }
  return ret;
}

int eng_write_productnvdata(char *databuf, int data_len) {
  int ret = 0;
  int len;
  int fd = -1;
  char prop[128] = {0};
  char miscdata_path[128] = {0};

  if (-1 == property_get("ro.product.partitionpath", prop, "")){
    ENG_LOG("%s: get partitionpath fail\n", __FUNCTION__);
    return 0;
  }

  sprintf(miscdata_path, "%smiscdata", prop);
  fd = open(miscdata_path, O_WRONLY);
  if (fd >= 0) {
    ENG_LOG("%s open Ok miscdata_path = %s ", __FUNCTION__,
            miscdata_path);
#ifdef CONFIG_NAND
    __s64 up_sz = data_len;
    ioctl(fd, UBI_IOCVOLUP, &up_sz);
#endif
    len = write(fd, databuf, data_len);

    if (len <= 0) {
      ret = 1;
      ENG_LOG("%s read fail miscdata_path = %s ", __FUNCTION__,
              miscdata_path);
    }
    fsync(fd);
    close(fd);
  } else {
    ENG_LOG("%s open fail miscdata_path = %s ", __FUNCTION__,
            miscdata_path);
    ret = 1;
  }
  return ret;
}
