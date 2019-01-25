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
 *  Module:       tbase_packrestore.h 
 *  Description:
 *     restore previous packing setting                
 ************************************************************************/

/* NOTE: No include guards must be used here! */

#if !defined(TBASE_COMPILER_DETECTED)
	#error Compiler not detected. tbase_platform must be included before.
#endif

/* do not complain about missing include guard */
/*lint -efile(451,tbase_packrestore.h) */


#if defined(TBASE_COMPILER_MICROSOFT)
/* MS Windows */
#include <poppack.h>

#elif defined(TBASE_COMPILER_GNU)
/* GNU compiler */
/* restore previous alignment */
#pragma pack(pop)

#elif defined(TBASE_COMPILER_ARM)
/* ARM (Keil) compiler */
/* restore previous alignment */
#pragma pop 

#elif defined(TBASE_COMPILER_TMS470)
/* ARM Optimizing C/C++ Compiler (Texas Instruments/TMS470) */
/* TI compilers do not currently support the #pragma pack construct */
/* ### */

#elif defined(TBASE_COMPILER_GHS)
/* Green Hills C/C++ Compiler */
#pragma pack() 

#elif defined(TBASE_COMPILER_DIAB)
/* DiabData / WindRiver C/C++ Compiler */
/* set default alignment, last setting is NOT restored */
#pragma pack(0,0,0)

#elif defined(TBASE_COMPILER_PIC32)
/* Microchip PIC32 */
#pragma pack(pop)

#else
#error Compiler not detected. This file needs to be extended.
#endif

/******************************** EOF ****************************************/
