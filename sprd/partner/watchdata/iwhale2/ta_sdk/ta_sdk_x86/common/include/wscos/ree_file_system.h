/*
copyright 2016, WatchData
 */

#ifndef __REE_FILE_SYSTEM_H__
#define __REE_FILE_SYSTEM_H__

/* Include Kconfig variables. */
#include "wd_file_system.h"

/*
 * fs_open
 */
#define FS_O_RDONLY 		0x1
#define FS_O_WRONLY 	0x2
#define FS_O_RDWR  	 	0x4
#define FS_O_CREATE 		0x8
#define FS_O_EXCL   			0x10
#define FS_O_APPEND 		0x20
#define FS_O_TRUNC  		0x40

/*
 * fs_lseek
 */
#define FS_SEEK_SET 0x1
#define FS_SEEK_END 0x2
#define FS_SEEK_CUR 0x4

#define fs_open     wd_fs_open
#define fs_read     wd_fs_read
#define fs_write    wd_fs_write
#define fs_unlink   wd_fs_unlink
#define fs_truncate wd_fs_truncate
#define fs_close    wd_fs_close
#define fs_lseek    wd_fs_lseek
#define fs_mkdir    wd_fs_mkdir
#define fs_rmdir    wd_fs_rmdir

int tee_print_log(const void *buf, uint32_t len);
int tee_get_time(void *buf, uint32_t len);
void tee_time_wait(uint32_t timeout);
int tee_get_cancel();
int tee_ta_data_close(int deathID);
/*
typedef signed long long  int64_t;

int fs_open(const char *file, int flags);
int fs_read(int fd, void *buf, size_t len);
int fs_write(int fd, const void *buf, size_t len);
int fs_unlink(const char *file);
int fs_truncate(int fd, int64_t length);
int fs_close(int fd);
int fs_lseek(int fd, int64_t offset, int whence);
int fs_mkdir(const char *path, uint32_t mode);
int fs_rmdir(const char *name);
int tee_print_log(const void *buf, uint32_t len);
int tee_get_time(void *buf, uint32_t len);
void tee_time_wait(uint32_t timeout);
int tee_get_cancel();
int tee_ta_data_close(int deathID);
*/
#endif
