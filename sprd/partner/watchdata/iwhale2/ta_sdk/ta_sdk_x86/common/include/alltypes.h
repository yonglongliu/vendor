/* @LICENSE(MUSLC_MIT) */

#if defined(__NEED_size_t) && !defined(__DEFINED_size_t)
typedef unsigned size_t;
#define __DEFINED_size_t
#endif

#if defined(__NEED_ssize_t) && !defined(__DEFINED_ssize_t)
typedef int ssize_t;
#define __DEFINED_ssize_t
#endif

#if defined(__NEED_ptrdiff_t) && !defined(__DEFINED_ptrdiff_t)
typedef int ptrdiff_t;
#define __DEFINED_ptrdiff_t
#endif


#if defined(__NEED_va_list) && !defined(__DEFINED_va_list)
typedef __builtin_va_list va_list;
#define __DEFINED_va_list
#endif


#ifndef __cplusplus
#if defined(__NEED_wchar_t) && !defined(__DEFINED_wchar_t)
typedef unsigned wchar_t;
#define __DEFINED_wchar_t
#endif

#endif
#if defined(__NEED_wint_t) && !defined(__DEFINED_wint_t)
typedef int wint_t;
#define __DEFINED_wint_t
#endif

#if defined(__NEED_wctrans_t) && !defined(__DEFINED_wctrans_t)
typedef long wctrans_t;
#define __DEFINED_wctrans_t
#endif

#if defined(__NEED_wctype_t) && !defined(__DEFINED_wctype_t)
typedef long wctype_t;
#define __DEFINED_wctype_t
#endif


#if defined(__NEED_int8_t) && !defined(__DEFINED_int8_t)
typedef signed char int8_t;
#define __DEFINED_int8_t
#endif

#if defined(__NEED_int16_t) && !defined(__DEFINED_int16_t)
typedef short       int16_t;
#define __DEFINED_int16_t
#endif

#if defined(__NEED_int32_t) && !defined(__DEFINED_int32_t)
typedef int         int32_t;
#define __DEFINED_int32_t
#endif

#if defined(__NEED_int64_t) && !defined(__DEFINED_int64_t)
typedef long long   int64_t;
#define __DEFINED_int64_t
#endif


#if defined(__NEED_uint8_t) && !defined(__DEFINED_uint8_t)
typedef unsigned char      uint8_t;
#define __DEFINED_uint8_t
#endif

#if defined(__NEED_uint16_t) && !defined(__DEFINED_uint16_t)
typedef unsigned short     uint16_t;
#define __DEFINED_uint16_t
#endif

#if defined(__NEED_uint32_t) && !defined(__DEFINED_uint32_t)
typedef unsigned int       uint32_t;
#define __DEFINED_uint32_t
#endif

#if defined(__NEED_uint64_t) && !defined(__DEFINED_uint64_t)
typedef unsigned long long uint64_t;
#define __DEFINED_uint64_t
#endif


#if defined(__NEED___uint16_t) && !defined(__DEFINED___uint16_t)
typedef unsigned short     __uint16_t;
#define __DEFINED___uint16_t
#endif

#if defined(__NEED___uint32_t) && !defined(__DEFINED___uint32_t)
typedef unsigned int       __uint32_t;
#define __DEFINED___uint32_t
#endif

#if defined(__NEED___uint64_t) && !defined(__DEFINED___uint64_t)
typedef unsigned long long __uint64_t;
#define __DEFINED___uint64_t
#endif


#if defined(__NEED_int_fast8_t) && !defined(__DEFINED_int_fast8_t)
typedef int8_t    int_fast8_t;
#define __DEFINED_int_fast8_t
#endif

#if defined(__NEED_int_fast16_t) && !defined(__DEFINED_int_fast16_t)
typedef int       int_fast16_t;
#define __DEFINED_int_fast16_t
#endif

#if defined(__NEED_int_fast32_t) && !defined(__DEFINED_int_fast32_t)
typedef int       int_fast32_t;
#define __DEFINED_int_fast32_t
#endif

#if defined(__NEED_int_fast64_t) && !defined(__DEFINED_int_fast64_t)
typedef int64_t   int_fast64_t;
#define __DEFINED_int_fast64_t
#endif


#if defined(__NEED_uint_fast8_t) && !defined(__DEFINED_uint_fast8_t)
typedef unsigned char      uint_fast8_t;
#define __DEFINED_uint_fast8_t
#endif

#if defined(__NEED_uint_fast16_t) && !defined(__DEFINED_uint_fast16_t)
typedef unsigned int       uint_fast16_t;
#define __DEFINED_uint_fast16_t
#endif

#if defined(__NEED_uint_fast32_t) && !defined(__DEFINED_uint_fast32_t)
typedef unsigned int       uint_fast32_t;
#define __DEFINED_uint_fast32_t
#endif

#if defined(__NEED_uint_fast64_t) && !defined(__DEFINED_uint_fast64_t)
typedef uint64_t           uint_fast64_t;
#define __DEFINED_uint_fast64_t
#endif


#if defined(__NEED_intptr_t) && !defined(__DEFINED_intptr_t)
typedef long          intptr_t;
#define __DEFINED_intptr_t
#endif

#if defined(__NEED_uintptr_t) && !defined(__DEFINED_uintptr_t)
typedef unsigned long uintptr_t;
#define __DEFINED_uintptr_t
#endif


#if defined(__NEED_float_t) && !defined(__DEFINED_float_t)
typedef float float_t;
#define __DEFINED_float_t
#endif

#if defined(__NEED_double_t) && !defined(__DEFINED_double_t)
typedef double double_t;
#define __DEFINED_double_t
#endif


#if defined(__NEED_time_t) && !defined(__DEFINED_time_t)
typedef long time_t;
#define __DEFINED_time_t
#endif

#if defined(__NEED_suseconds_t) && !defined(__DEFINED_suseconds_t)
typedef int suseconds_t;
#define __DEFINED_suseconds_t
#endif

#if defined(__NEED_struct_timeval) && !defined(__DEFINED_struct_timeval)
struct timeval { time_t tv_sec; int tv_usec; };
#define __DEFINED_struct_timeval
#endif

#if defined(__NEED_struct_timespec) && !defined(__DEFINED_struct_timespec)
struct timespec { time_t tv_sec; long tv_nsec; };
#define __DEFINED_struct_timespec
#endif


#if defined(__NEED_pid_t) && !defined(__DEFINED_pid_t)
typedef int pid_t;
#define __DEFINED_pid_t
#endif

#if defined(__NEED_id_t) && !defined(__DEFINED_id_t)
typedef int id_t;
#define __DEFINED_id_t
#endif

#if defined(__NEED_uid_t) && !defined(__DEFINED_uid_t)
typedef int uid_t;
#define __DEFINED_uid_t
#endif

#if defined(__NEED_gid_t) && !defined(__DEFINED_gid_t)
typedef int gid_t;
#define __DEFINED_gid_t
#endif

#if defined(__NEED_key_t) && !defined(__DEFINED_key_t)
typedef int key_t;
#define __DEFINED_key_t
#endif


#if defined(__NEED_pthread_t) && !defined(__DEFINED_pthread_t)
typedef struct __pthread * pthread_t;
#define __DEFINED_pthread_t
#endif

#if defined(__NEED_pthread_once_t) && !defined(__DEFINED_pthread_once_t)
typedef int pthread_once_t;
#define __DEFINED_pthread_once_t
#endif

#if defined(__NEED_pthread_key_t) && !defined(__DEFINED_pthread_key_t)
typedef int pthread_key_t;
#define __DEFINED_pthread_key_t
#endif

#if defined(__NEED_pthread_spinlock_t) && !defined(__DEFINED_pthread_spinlock_t)
typedef int pthread_spinlock_t;
#define __DEFINED_pthread_spinlock_t
#endif


#if defined(__NEED_pthread_attr_t) && !defined(__DEFINED_pthread_attr_t)
typedef struct { union { int __i[9]; size_t __s[9]; } __u; } pthread_attr_t;
#define __DEFINED_pthread_attr_t
#endif

#if defined(__NEED_pthread_mutexattr_t) && !defined(__DEFINED_pthread_mutexattr_t)
typedef unsigned pthread_mutexattr_t;
#define __DEFINED_pthread_mutexattr_t
#endif

#if defined(__NEED_pthread_condattr_t) && !defined(__DEFINED_pthread_condattr_t)
typedef unsigned pthread_condattr_t;
#define __DEFINED_pthread_condattr_t
#endif

#if defined(__NEED_pthread_barrierattr_t) && !defined(__DEFINED_pthread_barrierattr_t)
typedef unsigned pthread_barrierattr_t;
#define __DEFINED_pthread_barrierattr_t
#endif

#if defined(__NEED_pthread_rwlockattr_t) && !defined(__DEFINED_pthread_rwlockattr_t)
typedef struct { unsigned __attr[2]; } pthread_rwlockattr_t;
#define __DEFINED_pthread_rwlockattr_t
#endif


#if defined(__NEED_pthread_mutex_t) && !defined(__DEFINED_pthread_mutex_t)
typedef struct { union { int __i[6]; void *__p[6]; } __u; } pthread_mutex_t;
#define __DEFINED_pthread_mutex_t
#endif

#if defined(__NEED_pthread_cond_t) && !defined(__DEFINED_pthread_cond_t)
typedef struct { union { int __i[12]; void *__p[12]; } __u; } pthread_cond_t;
#define __DEFINED_pthread_cond_t
#endif

#if defined(__NEED_pthread_rwlock_t) && !defined(__DEFINED_pthread_rwlock_t)
typedef struct { union { int __i[8]; void *__p[8]; } __u; } pthread_rwlock_t;
#define __DEFINED_pthread_rwlock_t
#endif

#if defined(__NEED_pthread_barrier_t) && !defined(__DEFINED_pthread_barrier_t)
typedef struct { union { int __i[5]; void *__p[5]; } __u; } pthread_barrier_t;
#define __DEFINED_pthread_barrier_t
#endif


#if defined(__NEED_off_t) && !defined(__DEFINED_off_t)
typedef long long off_t;
#define __DEFINED_off_t
#endif


#if defined(__NEED_mode_t) && !defined(__DEFINED_mode_t)
typedef unsigned int mode_t;
#define __DEFINED_mode_t
#endif


#if defined(__NEED_nlink_t) && !defined(__DEFINED_nlink_t)
typedef unsigned int nlink_t;
#define __DEFINED_nlink_t
#endif

#if defined(__NEED_ino_t) && !defined(__DEFINED_ino_t)
typedef unsigned long long ino_t;
#define __DEFINED_ino_t
#endif

#if defined(__NEED_dev_t) && !defined(__DEFINED_dev_t)
typedef long long dev_t;
#define __DEFINED_dev_t
#endif

#if defined(__NEED_blksize_t) && !defined(__DEFINED_blksize_t)
typedef long blksize_t;
#define __DEFINED_blksize_t
#endif

#if defined(__NEED_blkcnt_t) && !defined(__DEFINED_blkcnt_t)
typedef long long blkcnt_t;
#define __DEFINED_blkcnt_t
#endif

#if defined(__NEED_fsblkcnt_t) && !defined(__DEFINED_fsblkcnt_t)
typedef unsigned long long fsblkcnt_t;
#define __DEFINED_fsblkcnt_t
#endif

#if defined(__NEED_fsfilcnt_t) && !defined(__DEFINED_fsfilcnt_t)
typedef unsigned long long fsfilcnt_t;
#define __DEFINED_fsfilcnt_t
#endif


#if defined(__NEED_timer_t) && !defined(__DEFINED_timer_t)
typedef void * timer_t;
#define __DEFINED_timer_t
#endif

#if defined(__NEED_clockid_t) && !defined(__DEFINED_clockid_t)
typedef int clockid_t;
#define __DEFINED_clockid_t
#endif

#if defined(__NEED_clock_t) && !defined(__DEFINED_clock_t)
typedef unsigned long clock_t;
#define __DEFINED_clock_t
#endif


#if defined(__NEED_sigset_t) && !defined(__DEFINED_sigset_t)
typedef struct { unsigned long __bits[128/sizeof(long)]; } sigset_t;
#define __DEFINED_sigset_t
#endif

#if defined(__NEED_siginfo_t) && !defined(__DEFINED_siginfo_t)
typedef struct __siginfo siginfo_t;
#define __DEFINED_siginfo_t
#endif


#if defined(__NEED_socklen_t) && !defined(__DEFINED_socklen_t)
typedef unsigned int socklen_t;
#define __DEFINED_socklen_t
#endif

#if defined(__NEED_sa_family_t) && !defined(__DEFINED_sa_family_t)
typedef unsigned short sa_family_t;
#define __DEFINED_sa_family_t
#endif

#if defined(__NEED_in_port_t) && !defined(__DEFINED_in_port_t)
typedef unsigned short in_port_t;
#define __DEFINED_in_port_t
#endif

#if defined(__NEED_in_addr_t) && !defined(__DEFINED_in_addr_t)
typedef unsigned int in_addr_t;
#define __DEFINED_in_addr_t
#endif

#if defined(__NEED_struct_in_addr) && !defined(__DEFINED_struct_in_addr)
struct in_addr { in_addr_t s_addr; };
#define __DEFINED_struct_in_addr
#endif


#if defined(__NEED_FILE) && !defined(__DEFINED_FILE)
typedef struct __FILE_s FILE;
#define __DEFINED_FILE
#endif


#if defined(__NEED_nl_item) && !defined(__DEFINED_nl_item)
typedef int nl_item;
#define __DEFINED_nl_item
#endif


#if defined(__NEED_locale_t) && !defined(__DEFINED_locale_t)
typedef struct __locale * locale_t;
#define __DEFINED_locale_t
#endif


#if defined(__NEED_struct_iovec) && !defined(__DEFINED_struct_iovec)
struct iovec { void *iov_base; size_t iov_len; };
#define __DEFINED_struct_iovec
#endif

#define creat 						wd_creat
#define fcntl 						wd_fcntl
#define open 						wd_open
#define openat 					wd_openat
#define posix_fadvise 		wd_posix_fadvise
#define posix_fallocate 	wd_posix_fallocate

#define pipe							wd_pipe
#define close 						wd_close
#define dup							wd_dup
#define dup2 						wd_dup2
#define lseek 						wd_lseek
#define fsync 						wd_fsync
#define fdatasync 				wd_fdatasync
#define read							wd_read
#define write						wd_write
#define pread						wd_pread
#define pwrite						wd_pwrite
#define chown					wd_chown
#define fchown					wd_fchown
#define lchown					wd_lchown
#define fchownat				wd_fchownat
#define link							wd_link
#define linkat						 wd_linkat
#define symlink 					wd_symlink
#define symlinkat 				wd_symlinkat
#define readlink 					wd_readlink
#define readlinkat 				wd_readlinkat
#define unlink 					wd_unlink
#define unlinkat 				wd_unlinkat
#define rmdir 						wd_rmdir
#define truncate 				wd_truncate
#define ftruncate 				wd_ftruncate
#define access 					wd_access
#define faccessat 				wd_faccessat
#define chdir 						wd_chdir
#define fchdir 						wd_fchdir
#define getcwd					 wd_getcwd
#define alarm 						wd_alarm
#define sleep 						wd_sleep
#define pause 						wd_pause
#define fork 							wd_fork
#define execve 					wd_execve
#define execv 						wd_execv
#define execle 					wd_execle
#define execl 						wd_execl
#define execvp 					wd_execvp
#define execlp 					wd_execlp
#define fexecve 					wd_fexecve
#define _exit 						wd__exit
#define getpid 					wd_getpid
#define getppid 					wd_getppid
#define getpgrp 					wd_getpgrp
#define getpgid					 wd_getpgid
#define setpgid 					wd_setpgid
#define setsid 						wd_setsid
#define getsid 						wd_getsid
#define ttyname 				wd_ttyname
#define ttyname_r 			wd_ttyname_r
#define isatty						 wd_isatty
#define tcgetpgrp 				wd_tcgetpgrp
#define tcsetpgrp 				wd_tcsetpgrp
#define getuid 					wd_getuid
#define geteuid 					wd_geteuid
#define getgid 					wd_getgid
#define getegid 					wd_getegid
#define getgroups 			wd_getgroups
#define setuid						 wd_setuid
#define setreuid 				wd_setreuid
#define seteuid 					wd_seteuid
#define setgid 						wd_setgid
#define setregid 					wd_setregid
#define setegid 					wd_setegid
#define getlogin 					wd_getlogin
#define getlogin_r 				wd_getlogin_r
#define gethostname 		wd_gethostname
#define ctermid 					wd_ctermid
#define getopt 					wd_getopt
#define pathconf 				wd_pathconf
#define fpathconf 				wd_fpathconf
#define sysconf 					wd_sysconf
#define confstr 					wd_confstr

#define open						wd_open
#define close						wd_close
#define read							wd_read
#define write						wd_write
#define lseek						wd_lseek
#define fopen						wd_fopen
#define fclose						wd_fclose
#define fread						wd_fread
#define fwrite						wd_fwrite
#define fseek						wd_fseek
#define fgetc						wd_fgetc
#define ftell							wd_ftell
#define access						wd_access
#define creat						wd_creat
#define mkdir						wd_mkdir
#define closedir					wd_closedir
#define opendir					wd_opendir
#define fsync						wd_fsync
#define sync							wd_sync
#define remove					wd_remove
#define malloc 					wd_malloc
#define free 							wd_free
#define realloc						wd_realloc


