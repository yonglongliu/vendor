#ifndef __CONFIG_WINDOWS_INC__
#define  __CONFIG_WINDOWS_INC__

#include <stdio.h>
#include <stdint.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "NXP_I2C.h"

#include <winsock.h> /* htons() */

#if BYTE_ORDER == LITTLE_ENDIAN
  #define htobe16(x) htons(x)
#elif BYTE_ORDER == BIG_ENDIAN
  #define htobe16(x) (x)
#else
  #error byte order not supported
#endif

/* dbgprint.h */
#ifndef _ASSERT
#define _ASSERT(e)
#endif
#ifndef _ERRORMSG
#define _ERRORMSG(fmt,...) printf("tfa98xx: %s:%s:%d: "fmt,__FILE__,__FUNCTION__,__LINE__,__VA_ARGS__)
#endif
#ifndef ERRORMSG
#define ERRORMSG(fmt,...) _ERRORMSG(fmt,__VA_ARGS__)
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

#ifndef snprintf
#define snprintf(str, size, format, ...) _snprintf((str), (size), (format), __VA_ARGS__)
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

#define BIT(x) (1 << (x))

#ifndef cpu_to_be16
#define cpu_to_be16 htobe16
#endif

/* tfa/src/tfa_osal.c */
void *kmalloc(size_t size, gfpt_t flags);
void kfree(const void *ptr);

/* tfa/src/tfa_osal.c */
unsigned long msleep_interruptible(unsigned int msecs);

/** 
 * Obtain the calculated crc.
 * tfa/src/tfa_container_crc32.c 
 */
uint32_t crc32_le(uint32_t crc, unsigned char const *buf, size_t len);

#ifndef __GNUC__
typedef int bool;
#define false 0
#define true 1
#endif

#endif /*  __CONFIG_WINDOWS_INC__ */

