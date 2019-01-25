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
 *  Module:       tbase_func_TbTrace.inc
 *  Description:
 *     implementation of tbase function
 ************************************************************************/

/*
 NOTE: This file is to be included in *exactly one* C file of the conrete project.
 This way you will pull in the implementation of the function below.
*/

#ifndef TB_TRACE_OUTPUT_AVAILABLE
#error tbase_trace.h must be included before this file.
#endif
#ifndef TB_TRACE_IS_INTERLOCKED
#error tbase_trace.h must be included before this file.
#endif


#if TB_TRACE_OUTPUT_AVAILABLE

#include <stdarg.h>


#if TB_TRACE_IS_INTERLOCKED
volatile T_INT32 gTbInInterruptService = 0;
#endif



// print string, no lock
static
void
TbOutputString(const char* str)
{
	while ( 0 != *str ) TbTraceOutputChar(*str++);
}


void TbTrace(const char* format, ...)
{
	va_list va;
	va_start(va, format);
	TbAcquireTraceLock();
	TbTraceVPrintf(format, va);
	TbReleaseTraceLock();
	va_end(va);
}


void TbTraceNoLock(const char* format, ...)
{
	va_list va;
	va_start(va, format);
	TbTraceVPrintf(format, va);
	va_end(va);
}


void TbTracePfx(const char* prefix, const char* format, ...)
{
	va_list va;
	va_start(va, format);
	TbAcquireTraceLock();
	if ( prefix != NULL ) {
		TbOutputString(prefix);
		TbOutputString(":");
	}
	TbTraceVPrintf(format, va);
	TbReleaseTraceLock();
	va_end(va);
}


void TbTracePfx2(const char* prefix1, const char* prefix2, const char* format, ...)
{
	va_list va;
	va_start(va, format);
	TbAcquireTraceLock();
	if ( prefix1 != NULL ) {
		TbOutputString(prefix1);
		TbOutputString(":");
	}
	if ( prefix2 != NULL ) {
		TbOutputString(prefix2);
		TbOutputString(":");
	}
	TbTraceVPrintf(format, va);
	TbReleaseTraceLock();
	va_end(va);
}



/* this function is optimized to use a minimum of stack size */
void TbDumpBytesEx(const void* ptr, size_t byteCount, size_t bytesPerLine)
{
	char tmp[5];
	size_t lineCount;
	size_t offset = 0;
	unsigned char* p = (unsigned char*)ptr;
	unsigned char* p_ascii = p;
	unsigned char* p_limit = p + byteCount;
	size_t i;

	TbAcquireTraceLock();

	if (NULL == ptr) {
		TbOutputString("<<NULL ptr>>\n");
	} else {

		if (0 == byteCount) {
			TbOutputString("<<zero length>>\n");
		} else {

			if (bytesPerLine < 1) {
				bytesPerLine = 1;
			}
			
			/* round up */
			lineCount = (byteCount+bytesPerLine-1) / bytesPerLine;

			while ((lineCount--) != 0) {
				/* dump offset */
				tmp[0] = TB_HEX_UCASE_CHAR_FROM_INT((offset>>12)&0xF);
				tmp[1] = TB_HEX_UCASE_CHAR_FROM_INT((offset>>8)&0xF);
				tmp[2] = TB_HEX_UCASE_CHAR_FROM_INT((offset>>4)&0xF);
				tmp[3] = TB_HEX_UCASE_CHAR_FROM_INT(offset&0xF);
				tmp[4] = 0;
				TbOutputString(tmp);
				TbOutputString(": ");
				offset += bytesPerLine;
				
				/* dump data bytes */
				for (i = 0; i < bytesPerLine; i++) {
					if (p < p_limit) {
						tmp[0] = TB_HEX_UCASE_CHAR_FROM_INT((*p>>4)&0xF);
						tmp[1] = TB_HEX_UCASE_CHAR_FROM_INT((*p)&0xF);
						p++;
					} else {
						tmp[0] = ' ';
						tmp[1] = ' ';
					}
					tmp[2] = 0;
					TbOutputString(tmp);
					TbOutputString(" ");
				}
				
				/* dump ASCII */
				for (i = 0; i < bytesPerLine; i++) {
					if (p_ascii < p_limit) {
						tmp[0] = ( ((*p_ascii)>=0x20) && ((*p_ascii)<=0x7F) ) ? (char)(*p_ascii) : '.';
						p_ascii++;
					} else {
						tmp[0] = ' ';
					}
					tmp[1] = 0;
					TbOutputString(tmp);
				}
				
				/* new line */
				TbOutputString("\n");
			}
		}
	}

	TbReleaseTraceLock();
}


#endif  /* TB_TRACE_OUTPUT_AVAILABLE */

/*************************** EOF **************************************/
