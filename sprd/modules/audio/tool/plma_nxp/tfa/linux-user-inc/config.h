#ifndef __CONFIG_LINUX_USER_INC__
#define  __CONFIG_LINUX_USER_INC__

#include <stdio.h>
#include <stdint.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "NXP_I2C.h"

#ifdef __ANDROID__
#include <sys/endian.h>
#else
#include <endian.h>
#endif

/* https://android-review.googlesource.com/#/c/112295/ */
#if defined(__ANDROID__) && !defined(HOST_NAME_MAX)
   #define HOST_NAME_MAX 255
#endif

/* dbgprint.h */
#define _ASSERT(e)

#ifndef _ERRORMSG
#define _ERRORMSG(fmt,va...) fprintf(stderr, "tfa98xx: ERROR %s:%s:%d: "fmt,__FILE__,__func__,__LINE__, ##va)
#endif
#ifndef ERRORMSG
#define ERRORMSG(x...) _ERRORMSG(x)
#endif
#ifndef PRINT
#define PRINT(...)	printf(__VA_ARGS__)
#endif
#ifndef PRINT_ERROR
#define PRINT_ERROR(...)	 fprintf(stderr,__VA_ARGS__)
#endif
#ifndef PRINT_ASSERT
#define PRINT_ASSERT(e)if ((e)) fprintf(stderr, "PrintAssert:%s (%s:%d) error code:%d\n",__FUNCTION__,__FILE__,__LINE__, e)
#endif

#ifndef __GNUC__
typedef int bool;
#define false 0
#define true 1
#endif

#define tfa98xx_trace_printk PRINT
#define pr_debug PRINT
#define pr_info PRINT
#define pr_err ERRORMSG

#define GFP_KERNEL 0

typedef unsigned gfpt_t;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef _Bool  bool;

enum {
	false = 0,
	true = 1
};

#define BIT(x) (1 << (x))

#ifndef cpu_to_be16
#define cpu_to_be16 htobe16
#endif

/* tfa/src/tfa_osal.c */
void *kmalloc(size_t size, gfpt_t flags);
void kfree(const void *ptr);

/* tfa/src/tfa_osal.c */
unsigned long msleep_interruptible(unsigned int msecs);

/* tfa/src/tfa_container_crc32.c */
uint32_t crc32_le(uint32_t crc, unsigned char const *buf, size_t len);


#endif /* __CONFIG_LINUX_USER_INC__ */

