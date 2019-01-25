/*
 *Copyright 2014 NXP Semiconductors
 *
 *Licensed under the Apache License, Version 2.0 (the "License");
 *you may not use this file except in compliance with the License.
 *You may obtain a copy of the License at
 *            
 *http://www.apache.org/licenses/LICENSE-2.0
 *             
 *Unless required by applicable law or agreed to in writing, software
 *distributed under the License is distributed on an "AS IS" BASIS,
 *WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *See the License for the specific language governing permissions and
 *limitations under the License.
 */

/************************************************************************
 *  Module:       tbase_trace.h
 *  Description:
 *     debug trace support                
 ************************************************************************/

#ifndef __tbase_trace_h__
#define __tbase_trace_h__

#if defined (__cplusplus)
extern "C" {
#endif

#ifndef TB_DEBUG
#error TB_DEBUG is not defined.
#endif

/*
	By default, tracing functions are available in debug builds only.
	To enable them explicitly in release builds, TB_TRACE_OUTPUT_AVAILABLE=1 needs to be defined.
*/
#ifndef TB_TRACE_OUTPUT_AVAILABLE
#if TB_DEBUG
#define TB_TRACE_OUTPUT_AVAILABLE	1
#else
#define TB_TRACE_OUTPUT_AVAILABLE	0
#endif
#endif

/*
	By default, trace functions are not interlocked.
	To enable lock/unlock encapsulation, set TB_TRACE_IS_INTERLOCKED to 1 in trace_config.h.
	See also TbAcquireTraceLock() below.
*/
#ifndef TB_TRACE_IS_INTERLOCKED
#define TB_TRACE_IS_INTERLOCKED	0
#endif

/*
	By default, prints to the UART are not buffered.
	To enable the intermediate buffer, set TB_TRACE_IS_BUFFERED to 1 in trace_config.h.
	See also TbEnableTraceBuffer() below.
*/
#ifndef TB_TRACE_IS_BUFFERED
#define TB_TRACE_IS_BUFFERED	0
#endif


/*
  default trace configuration
*/
#ifndef TB_TRACE_ERR_ENABLE
#if TB_DEBUG
#define TB_TRACE_ERR_ENABLE   1
#else
#define TB_TRACE_ERR_ENABLE   0
#endif
#endif

#ifndef TB_TRACE_WRN_ENABLE
#if TB_DEBUG
#define TB_TRACE_WRN_ENABLE   1
#else
#define TB_TRACE_WRN_ENABLE   0
#endif
#endif

#ifndef TB_TRACE_INF_ENABLE
#if TB_DEBUG
#define TB_TRACE_INF_ENABLE   1
#else
#define TB_TRACE_INF_ENABLE   0
#endif
#endif

#ifndef TB_ASSERT_ENABLE
#if TB_DEBUG
#define TB_ASSERT_ENABLE    1
#else
#define TB_ASSERT_ENABLE    0
#endif
#endif


/*
	Output character to trace target, e.g. UART
	used by default implementation of TbTrace.
	Should not be used directly by an application, normally.
*/
#if TB_TRACE_OUTPUT_AVAILABLE
void TbTraceOutputChar(char c);
#endif

/*
  Flush trace UART. Blocks until all characters are written to UART.
*/
#if TB_TRACE_OUTPUT_AVAILABLE
void TbFlushTraceOutput(void);
#else
// dummy function
static TB_INLINE void TbFlushTraceOutput(void) { }
#endif


/* 
  Print formatted to debug output
  Used for debug trace messages.
  To get a default implementation of TbTrace, include tbase_func_TbTrace.inc
  in exactly one .c file of your project.
  
  NOTE: The format string supports a subset of the printf-style formatting codes:
  %c %d %u %s %x %X %p
  
  IMPORTANT: To write portable code, you need to use the right formatters according 
  to the table below:
  
  T_INT8,   char                  %hhd, %hhx, %hhX, %c
  T_UINT8,  unsigned char         %hhu, %hhx, %hhX
  T_INT16,  short                 %hd, %hx, %hX
  T_UINT16, unsigned short        %hu, %hx, %hX
  T_INT32,  int                   %d, %x, %X
  T_UINT32, unsigned int          %u, %x, %X
            long                  %ld, %lx, %lX
            unsigned long         %lu, %lx, %lX
  T_INT64,  long long             %lld, %llx, %llX
  T_UINT64, unsigned long long    %llu, %llx, %llX
            size_t                %d, %x, %X   you need to cast to int, e.g. if x is of type size_t push (int)x onto the stack
                                               note: C99 'z' size modifier is not supported by Microsoft
*/
#if TB_TRACE_OUTPUT_AVAILABLE
// normal variant, includes locks
void TbTrace(const char* format, ...);
// special variant that uses no locks, e.g. for use in fault handlers
void TbTraceNoLock(const char* format, ...);
#endif


/*
	These variants support a prefix that is printed 
	at the beginning of the line followed by a colon ':'.
	Each prefix is optional. Specify a NULL pointer if no prefix is to be printed.
*/
#if TB_TRACE_OUTPUT_AVAILABLE
void TbTracePfx(const char* prefix, const char* format, ...);
void TbTracePfx2(const char* prefix1, const char* prefix2, const char* format, ...);
#endif


/*
  Print hex dump through TbTrace

  To get a default implementation of TbDumpBytesEx, include tbase_func_TbDumpBytesEx.inc
  in exactly one .c file of your project.
*/
#if TB_TRACE_OUTPUT_AVAILABLE
void TbDumpBytesEx(const void* ptr, size_t byteCount, size_t bytesPerLine);
#endif

/* shortcut for most common case: 16 bytes width */
#define TbDumpBytes(ptr, byteCount)   TbDumpBytesEx((ptr), (byteCount), 16)


/*
  The general idea is:
  Trace channels ERROR, WARNING, INFO are global and will be used by all modules.
  Optionally, each module can add its own specific trace channels.

  Definition of module specific trace macros:
  
  In each module specific macros can be defined which represent
  an individual trace level. For example:
#if TB_DEBUG && 0  // to enable the trace level, change this to #if TB_DEBUG && 1
#define CLSHID_TRACE_FUNC(x) TB_TRACE_impl(x)
#else
#define CLSHID_TRACE_FUNC(x)
#endif

  Alternatively, define a CLSHID_TRACE_FUNC_ENABLE flag and set it to 0 by default, e.g.:
#ifndef CLSHID_TRACE_FUNC_ENABLE
#define CLSHID_TRACE_FUNC_ENABLE 0
#endif
#if     CLSHID_TRACE_FUNC_ENABLE
#define CLSHID_TRACE_FUNC(x) TB_TRACE_impl(x)
#else 
#define CLSHID_TRACE_FUNC(x) 
#endif

  In the application add a trace_config.h file which has:
#if TB_DEBUG
#define CLSHID_TRACE_FUNC_ENABLE 1    / * set to 1 or 0 to enable/disable this trace level * /
#endif

*/


/*
  ERROR trace.
  To be used for fatal errors, asserts, unexpected conditions, etc.
  
*/
#if     TB_TRACE_ERR_ENABLE
#define TB_TRACE_ERR(x) TB_TRACE_impl(x)
#else
#define TB_TRACE_ERR(x)
#endif

/*
  WARNING trace.
  To be used for non-critical but unexpected conditions, e.g. additional data that is ignored
  
*/
#if     TB_TRACE_WRN_ENABLE
#define TB_TRACE_WRN(x) TB_TRACE_impl(x)
#else
#define TB_TRACE_WRN(x)
#endif

/*
  INFO trace.
  To be used for general information such as program start, version info
  init, shutdown, PnP events, etc. 
  Not to be used for repeating trace messages printed periodically.
*/
#if     TB_TRACE_INF_ENABLE
#define TB_TRACE_INF(x) TB_TRACE_impl(x)
#else
#define TB_TRACE_INF(x)
#endif

/*
Usage examples:

  use %s to print TB_FUNC, dangerous if TB_FUNC is missing by mistake
    TB_TRACE_ERR(TbTrace("%s: Hello World! %u\n", TB_FUNC, 88));
  
  use TbTracePfx instead, no %s needed, less dangerous
    TB_TRACE_ERR(TbTracePfx(TB_FUNC, "Hello World! %u\n", 88));

  TB_TRACE_INF(
    TbTracePfx(TB_FUNC, "Hello World! %u\n", 88);
    TbTracePfx(TB_FUNC, "xxx %u\n", 88);
    );
*/


/*
  Trace macro implementation
  Intended to be used to implement TB_TRACE_ERR etc. 
  and module-specific trace macros, e.g. CLSHID_TRACE_FUNC
*/
#define TB_TRACE_impl(x)  { x; }



/*
  runtime assertion
  
  usage examples:
  
  TB_ASSERT(x==y);
  
  instead of 
  TB_ASSERT(0);
  use
  TB_ASSERT_FALSE;
*/


#if TB_ASSERT_ENABLE

	#if defined(_lint)
	/* map to __assert to make lint aware of the special meaning of this function */
	// note: in C lint does not know the prototype of __assert
	void __assert(int cond);
	#define TB_ASSERT(condition)		__assert(condition)
	#define TB_ASSERT_FALSE					__assert(0)
	#else
	/* implementation */
	#define TB_ASSERT(condition)  if ( !(condition) ) { TbAssertionFailed(__FILE__, __LINE__); }
	#define TB_ASSERT_FALSE       { TbAssertionFailed(__FILE__, __LINE__); }
	#endif

#else

	/* empty */
	#define TB_ASSERT(condition)
	#define TB_ASSERT_FALSE

#endif


/*
	This function is called if an TB_ASSERT check fails.
	
	tag is a string, e.g. c module name, or __FILE__
	line is the line number, __LINE__

	To get a default implementation of TbAssertionFailed, include tbase_func_TbAssertionFailed.inc
	in exactly one .c file of your project.
*/
void TbAssertionFailed(const char* tag, int line);


/*
	Support for tracing from ISR or in multi-tasking environment.
*/
#if (TB_TRACE_OUTPUT_AVAILABLE && TB_TRACE_IS_INTERLOCKED)

/* counter used to track ISR context */
extern volatile T_INT32 gTbInInterruptService;

/* Enter/Leave ISR context */
/* Must be called at ISR entry/exit if that ISR prints traces. */
static TB_INLINE void TbEnterInterruptService(void)	{ if ( TbAtomicIncrementInt32(&gTbInInterruptService) <= 0 ) TB_ASSERT_FALSE; }
static TB_INLINE void TbLeaveInterruptService(void)	{ if ( TbAtomicDecrementInt32(&gTbInInterruptService) < 0 ) TB_ASSERT_FALSE; }
/* returns true if we are in interrupt context */
static TB_INLINE T_BOOL TbIsInInterruptService(void) { return ( (T_BOOL)(0 != gTbInInterruptService) ); }

// lock/unlock functions called from TbTrace
// to be implemented elsewhere
void TbAcquireTraceLock(void);
void TbReleaseTraceLock(void);

#else
// feature is disabled, provide dummy functions
static TB_INLINE void TbEnterInterruptService(void) { }
static TB_INLINE void TbLeaveInterruptService(void) { }
static TB_INLINE T_BOOL TbIsInInterruptService(void) { return T_FALSE; }
static TB_INLINE void TbAcquireTraceLock(void) { }
static TB_INLINE void TbReleaseTraceLock(void) { }

#endif /* (TB_TRACE_OUTPUT_AVAILABLE && TB_TRACE_IS_INTERLOCKED) */


#if defined (__cplusplus)
}
#endif

#endif  /* __tbase_trace_h__ */

/*************************** EOF **************************************/
