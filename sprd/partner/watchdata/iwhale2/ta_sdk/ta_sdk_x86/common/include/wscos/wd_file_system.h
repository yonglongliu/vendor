/*
copyright 2016, WatchData
 */

#ifndef __WD_FILE_SYSTEM_H__
#define __WD_FILE_SYSTEM_H__

/* Include Kconfig variables. */
//#include <autoconf.h>

/*
 * fs_open
 */
#define WD_FS_O_RDONLY 		0x1
#define WD_FS_O_WRONLY 	0x2
#define WD_FS_O_RDWR  	 	0x4
#define WD_FS_O_CREATE 		0x8
#define WD_FS_O_EXCL   			0x10
#define WD_FS_O_APPEND 		0x20
#define WD_FS_O_TRUNC  		0x40

/*
 * fs_lseek
 */
#define WD_FS_SEEK_SET 0x1
#define WD_FS_SEEK_END 0x2
#define WD_FS_SEEK_CUR 0x4

typedef signed long long  int64_t;

int wd_fs_open(const char *file, int flags);
int wd_fs_read(int fd, void *buf, size_t len);
int wd_fs_write(int fd, const void *buf, size_t len);
int wd_fs_unlink(const char *file);
int wd_fs_truncate(int fd, int64_t length);
int wd_fs_close(int fd);
int wd_fs_lseek(int fd, int64_t offset, int whence);
int wd_fs_mkdir(const char *path, uint32_t mode);
int wd_fs_rmdir(const char *name);

#endif
