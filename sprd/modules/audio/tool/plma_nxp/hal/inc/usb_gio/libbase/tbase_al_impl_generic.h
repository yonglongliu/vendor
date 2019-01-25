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
 *  Module:       tbase_al_impl_generic.h
 *  Description:
 *     abstraction layer implementation for common compilers                
 ************************************************************************/

#ifndef __tbase_al_impl_generic_h__
#define __tbase_al_impl_generic_h__


/* memcpy, memmove and memset are defined by the C99 standard and should be declared in string.h */
#include <string.h>

/* note: lint complains about these symbols already defined as functions */

/*lint -esym(652,TbCopyMemory) */
#define TbCopyMemory(dst,src,count) memcpy((dst),(src),(count))

/*lint -esym(652,TbMoveMemory) */
#define TbMoveMemory(dst,src,count) memmove((dst),(src),(count))

/*lint -esym(652,TbSetMemory) */
#define TbSetMemory(mem,val,count)  memset((mem),(val),(count))

/*lint -esym(652,TbZeroMemory) */
#define TbZeroMemory(mem,count)			memset((mem),0,(count))

/*lint -esym(652,TbIsEqualMemory) */
#define TbIsEqualMemory(buf1,buf2,count)	( 0 == memcmp((buf1),(buf2),(count)) )


#endif  /* __tbase_al_impl_generic_h__ */

/*************************** EOF **************************************/
