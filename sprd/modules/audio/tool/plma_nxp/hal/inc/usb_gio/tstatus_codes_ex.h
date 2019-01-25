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
 *  Module:       tstatus_codes_ex.h
 *  Description:
 *    Project-specific status code definitions used by both 
 *    kernel-mode and user-mode software layers.                
 ************************************************************************/

//
// This file must be included by tstatus_codes.h only.
//
#ifndef __inside_tstatus_codes_h__
#error This file is an extension to tstatus_codes.h and must not be included otherwise.
#endif


///////////////////////////////////////////////////////////////////
//
// Project-specific status codes
//
// A set of project-specific status codes may be defined per project.
// Use TSTATUS_BASE_EX+x, x ranges from 0x000..0xFFF.
//
///////////////////////////////////////////////////////////////////

#define       TSTATUS_INCOMPATIBLE_PROTOCOL_VERSION         TSTATUS_CODE(TSTATUS_BASE_EX+0x001)
case_TSTATUS_(TSTATUS_INCOMPATIBLE_PROTOCOL_VERSION, "The protocol version of the device is incompatible.")

#define       TSTATUS_INVALID_REPORT_LENGTH                 TSTATUS_CODE(TSTATUS_BASE_EX+0x002)
case_TSTATUS_(TSTATUS_INVALID_REPORT_LENGTH, "The length of a report defined by the device is not support.")

#define       TSTATUS_PIN_ID_MISMATCH                       TSTATUS_CODE(TSTATUS_BASE_EX+0x003)
case_TSTATUS_(TSTATUS_PIN_ID_MISMATCH, "The pin ID received in the response does not match the pin ID specified in the command.")

#define       TSTATUS_UNSUPPORTED_PIN_STATE                 TSTATUS_CODE(TSTATUS_BASE_EX+0x004)
case_TSTATUS_(TSTATUS_UNSUPPORTED_PIN_STATE, "An unsupported pin state was received.")

#define       TSTATUS_UNSUPPORTED_PIN_MODE                 TSTATUS_CODE(TSTATUS_BASE_EX+0x005)
case_TSTATUS_(TSTATUS_UNSUPPORTED_PIN_MODE, "An unsupported pin mode was received.")


#define       TSTATUS_RESPONSE_STATUS_BASE                  TSTATUS_CODE(TSTATUS_BASE_EX+0x100)
case_TSTATUS_(TSTATUS_RESPONSE_STATUS_BASE, "Base value for the status code generated from a 1 byte response status.")

///////////////////////////////////////////////////////////////////
//
// Application-specific status codes
//
// A set of application-specific status codes may be defined per project or application.
// Use TSTATUS_BASE_APP+x, x ranges from 0x000..0xFFF.
//
///////////////////////////////////////////////////////////////////

#define       TSTATUS_APP_ERROR1          TSTATUS_CODE(TSTATUS_BASE_APP+0x001)
case_TSTATUS_(TSTATUS_APP_ERROR1, "")

#define       TSTATUS_APP_ERROR2          TSTATUS_CODE(TSTATUS_BASE_APP+0x002)
case_TSTATUS_(TSTATUS_APP_ERROR2, "")

/*************************** EOF **************************************/
