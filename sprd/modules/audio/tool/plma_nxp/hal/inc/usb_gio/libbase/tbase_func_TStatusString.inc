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
 *  Module:       tbase_func_TStatusString.inc
 *  Description:
 *    TSTATUS codes to string translation                
 ************************************************************************/

/*
 NOTE: This file is to be included in *exactly one* C/C++ file of the conrete project.
 This way you will pull in the implementation of the function below.
*/

#define __inside_tstatus_codes_str_h__

#ifdef IMPLEMENT_TStatusStringImplA

//
// utility function:
// convert status code to string
// The function returns NULL if the provided code is unknown.
//
static
const char*
TStatusStringImplA(
	TSTATUS st
	)
{
#define TSTATUS_STR_impl(s)	s
#define case_TSTATUS_(val,txt) case val : return TSTATUS_STR_impl(#val);

	switch (st) {
// don't complain about negative indentation
//lint -save -e525

#include "tstatus_codes.h"

	default: return NULL;
//lint -restore
	} // switch

#undef case_TSTATUS_
#undef TSTATUS_STR_impl
}

#endif // IMPLEMENT_TStatusStringImplA


#ifdef IMPLEMENT_TStatusStringImplW

//
// utility function:
// convert status code to string
// The function returns NULL if the provided code is unknown.
//
static
const wchar_t*
TStatusStringImplW(
	TSTATUS st
	)
{
// helper macros
#define TSTATUS_STR_impl(s)	L ## s
#define case_TSTATUS_(val,txt) case val : return TSTATUS_STR_impl(#val);

	switch (st) {
// don't complain about negative indentation
//lint -save -e525

#include "tstatus_codes.h"

	default: return NULL;
//lint -restore
	} // switch

#undef case_TSTATUS_
#undef TSTATUS_STR_impl
}

#endif //IMPLEMENT_TStatusStringImplW



#ifdef IMPLEMENT_TStatusTextImplA

//
// utility function:
// return description text for status code
// The function returns NULL if the provided code is unknown.
//
static
const char*
TStatusTextImplA(
	TSTATUS st
	)
{
#define case_TSTATUS_(val,txt) case val : return txt;

	switch (st) {
// don't complain about negative indentation
//lint -save -e525

#include "tstatus_codes.h"

	default: return NULL;
//lint -restore
	} // switch

#undef case_TSTATUS_
}

#endif // IMPLEMENT_TStatusTextImplA



#ifdef IMPLEMENT_TStatusTextImplW

//
// utility function:
// return description text for status code
// The function returns NULL if the provided code is unknown.
//
static
const wchar_t*
TStatusTextImplW(
	TSTATUS st
	)
{
#define case_TSTATUS_(val,txt) case val : return L ## txt;

	switch (st) {
// don't complain about negative indentation
//lint -save -e525

#include "tstatus_codes.h"

	default: return NULL;
//lint -restore
	} // switch

#undef case_TSTATUS_
}

#endif // IMPLEMENT_TStatusTextImplW


#undef __inside_tstatus_codes_str_h__

/*************************** EOF **************************************/
