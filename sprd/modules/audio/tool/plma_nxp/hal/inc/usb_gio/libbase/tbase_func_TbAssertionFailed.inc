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
 *  Module:       tbase_func_TbAssertionFailed.inc
 *  Description:
 *     implementation of tbase function                
 ************************************************************************/

/*
 NOTE: This file is to be included in *exactly one* C file of the concrete project.
 This way you will pull in the implementation of the function below.
*/

void TbAssertionFailed(const char* tag, int line)
{
#if TB_TRACE_OUTPUT_AVAILABLE
	TbFlushTraceOutput();
	TbTraceNoLock("\nASSERTION FAILED: %s(%d)\n", tag, line);
	TbFlushTraceOutput();
#else
  TB_UNUSED_PARAM(tag)
  TB_UNUSED_PARAM(line)
#endif

	// firmware stops here
	for (;;);
}

/*************************** EOF **************************************/
