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
 *  Module:       tstatus_codes.h
 *  Description:
 *    Common status code definitions used by both 
 *    kernel-mode and user-mode software layers.
 ************************************************************************/

#if !defined(__tstatus_codes_h__) || defined(__inside_tstatus_codes_str_h__)
#define __tstatus_codes_h__
#define __inside_tstatus_codes_h__

// in case of RC this file is empty
#ifndef RC_INVOKED

// pull in header file with compiler detection
#include "tbase_platform.h"


///////////////////////////////////////////////////////////////////
// types
///////////////////////////////////////////////////////////////////

#ifndef TBASE_COMPILER_DETECTED
#error Compiler was not detected.
#endif

#ifdef TBASE_COMPILER_MICROSOFT
// Windows

#if defined(TBASE_COMPILER_MICROSOFT_KERNEL_MODE)
	// Windows kernel mode
	typedef long TSTATUS;
#elif defined(TBASE_COMPILER_MICROSOFT_USER_MODE)
	// Windows user mode
	typedef unsigned long TSTATUS;
#elif defined(TBASE_COMPILER_MICROSOFT_UNDER_CE)
	// Windows CE user mode
	typedef unsigned long TSTATUS;
#else
	#error One of TBASE_COMPILER_MICROSOFT_xxx must be defined.
#endif


// NTSTATUS custom bit
#define TSTATUS_NTSTATUS_CUSTOM_BIT	0x20000000
// NTSTATUS severity=error
#define TSTATUS_NTSTATUS_SEV_ERROR	0xC0000000

// facility codes
#define TSTATUS_PRIV_STATUS_FACILITY_CODE	0x0E000000
#define TSTATUS_USBD_STATUS_FACILITY_CODE	0x0F000000

//
// Generate a status code from x.
//
// Allowed range for x: 0x0001..0xFFFF
// We use severity=error and set the C bit
// Examples:
// 0xEE000001  TSTATUS_FAILED
// 0xEE000003  TSTATUS_TIMEOUT
//
#define TSTATUS_CODE(x) ( (TSTATUS)( TSTATUS_NTSTATUS_SEV_ERROR | TSTATUS_NTSTATUS_CUSTOM_BIT | TSTATUS_PRIV_STATUS_FACILITY_CODE | ((x) & 0xFFFF) ))

// status code range base values
#define TSTATUS_BASE_EX				0x1000		// project-specific extensions
#define TSTATUS_BASE_APP			0x2000		// application-specific codes


//
// NTSTATUS to TSTATUS mapping, see ntstatus.h
//
// We add the C bit (bit 29) to each NTSTATUS code. 
// So the upper bits 31..30 still contain the severity code.
// Examples:
// 0xC0000001L (STATUS_UNSUCCESSFUL) maps to 0xE0000001L
// 0x40000000L (STATUS_OBJECT_NAME_EXISTS) maps to 0x60000000L
//
#define TSTATUS_FROM_NTSTATUS(x) ( (TSTATUS)((x) | TSTATUS_NTSTATUS_CUSTOM_BIT) )
#if defined(TBASE_COMPILER_MICROSOFT_KERNEL_MODE) && !defined(__inside_tstatus_codes_str_h__)
TB_INLINE
TSTATUS
TStatusFromNTSTATUS(long ntstatus)	// should be NTSTATUS ntstatus but this creates a dependency on ntddk.h
{
	return ( (0 == ntstatus) ? 0 : TSTATUS_FROM_NTSTATUS(ntstatus) );
}
#endif

//
// USBD_STATUS to TSTATUS mapping, see usb.h
//
// For USBD status codes (from URB completion) we add the C bit and use a specific facility code.
// Example:
// 0xC0000004L (USBD_STATUS_STALL_PID) maps to 0xEF000004 (TSTATUS_USBD_STATUS_STALL_PID)
//
#define TSTATUS_FROM_USBD_STATUS(x) ( (TSTATUS)((x) | TSTATUS_NTSTATUS_CUSTOM_BIT | TSTATUS_USBD_STATUS_FACILITY_CODE) )
#if defined(TBASE_COMPILER_MICROSOFT_KERNEL_MODE) && !defined(__inside_tstatus_codes_str_h__)
TB_INLINE
TSTATUS
TStatusFromUSBDSTATUS(long st)
{
	return ( (0 == st) ? 0 : TSTATUS_FROM_USBD_STATUS(st) );
}
#endif


#elif defined(TBASE_COMPILER_APPLE)
// Apple
#include <IOKit/IOReturn.h>

typedef IOReturn TSTATUS;

//
// iokit_vendor_specific_err x -14bit (0 - 0x3fff)
//

#define TSTATUS_CODE(x) iokit_vendor_specific_err(x)

// for  status codes from errno
#define TSTATUS_CODE_ERRNO(x) iokit_vendor_specific_err(x + 0x3000)

// status code range base values
#define TSTATUS_BASE_EX				0x1000		// project-specific extensions
#define TSTATUS_BASE_APP			0x2000		// application-specific codes


#else
// default, e.g. Linux
typedef int TSTATUS;

// for private status codes we use the range 0xE0000001 ... 0xE0003FFF.
#define TSTATUS_CODE(x) ((TSTATUS)( 0xE0000000L | ((x) & 0x3FFF) ))

// for  status codes from errno with prefix 0xE0010000
#define TSTATUS_CODE_ERRNO(x) ((TSTATUS)( 0xE0010000L | ((x) & 0xFFFF) ))

// status code range base values
#define TSTATUS_BASE_EX				0x1000		// project-specific extensions
#define TSTATUS_BASE_APP			0x2000		// application-specific codes

// map errno, e.g. EINVAL, to TSTATUS code
#define TSTATUS_FROM_ERRNO(err) ( -(err) )

#endif



// define empty macro, if not included by tstatus_codes_str.h
#if !defined(__inside_tstatus_codes_str_h__)
#define case_TSTATUS_(val,txt)
#endif


///////////////////////////////////////////////////////////////////
// common status codes
// This set of generic status codes may be used in all kernel mode and
// user mode modules. Don't add project-specific codes to this set.
// Add specific codes to tstatus_codes_ex.h instead.
// The generic status codes values have to use the range from
// 0x0001 .. 0x0FFF.
// Under Windows this is mapped to 
// 0xE0000001 ... 0xE0000FFF
///////////////////////////////////////////////////////////////////

#define				TSTATUS_SUCCESS										0
case_TSTATUS_(TSTATUS_SUCCESS, "The operation completed successfully.")

#define				TSTATUS_FAILED										TSTATUS_CODE(0x001)
case_TSTATUS_(TSTATUS_FAILED, "The operation failed.")

#define				TSTATUS_INTERNAL_ERROR						TSTATUS_CODE(0x002)		
case_TSTATUS_(TSTATUS_INTERNAL_ERROR, "An internal error occurred.")

#define				TSTATUS_TIMEOUT										TSTATUS_CODE(0x003)
case_TSTATUS_(TSTATUS_TIMEOUT, "The operation timed out.")

#define				TSTATUS_REJECTED									TSTATUS_CODE(0x004)
case_TSTATUS_(TSTATUS_REJECTED, "The operation cannot be executed.")

#define				TSTATUS_ABORTED										TSTATUS_CODE(0x005)
case_TSTATUS_(TSTATUS_ABORTED, "The operation was aborted.")

#define				TSTATUS_IN_USE										TSTATUS_CODE(0x006)
case_TSTATUS_(TSTATUS_IN_USE, "The requested resource is currently in use.")

#define				TSTATUS_BUSY											TSTATUS_CODE(0x007)
case_TSTATUS_(TSTATUS_BUSY, "The device is currently busy.")

#define				TSTATUS_ALREADY_DONE							TSTATUS_CODE(0x008)
case_TSTATUS_(TSTATUS_ALREADY_DONE, "The operation was already executed and cannot be executed again.")

#define				TSTATUS_PENDING										TSTATUS_CODE(0x009)
case_TSTATUS_(TSTATUS_PENDING, "The operation is pending and will be completed at a later time.")



#define				TSTATUS_NO_MEMORY									TSTATUS_CODE(0x020)
case_TSTATUS_(TSTATUS_NO_MEMORY, "Not enough memory available to complete the operation.")

#define				TSTATUS_NO_RESOURCES							TSTATUS_CODE(0x021)
case_TSTATUS_(TSTATUS_NO_RESOURCES, "Not enough resources available to complete the operation.")

#define				TSTATUS_NO_MORE_ITEMS							TSTATUS_CODE(0x022)
case_TSTATUS_(TSTATUS_NO_MORE_ITEMS, "There is no more data available.")

#define				TSTATUS_NO_DEVICES								TSTATUS_CODE(0x023)
case_TSTATUS_(TSTATUS_NO_DEVICES, "There are no devices available in the system.")

#define				TSTATUS_NO_DATA										TSTATUS_CODE(0x024)
case_TSTATUS_(TSTATUS_NO_DATA, "There is no data available.")



#define				TSTATUS_NOT_SUPPORTED							TSTATUS_CODE(0x030)
case_TSTATUS_(TSTATUS_NOT_SUPPORTED, "The operation is not supported.")

#define				TSTATUS_NOT_POSSIBLE							TSTATUS_CODE(0x031)
case_TSTATUS_(TSTATUS_NOT_POSSIBLE, "The operation cannot be executed.")

#define				TSTATUS_NOT_ALLOWED								TSTATUS_CODE(0x032)
case_TSTATUS_(TSTATUS_NOT_ALLOWED, "There is no permission to execute the operation.")

#define				TSTATUS_NOT_OPENED								TSTATUS_CODE(0x033)
case_TSTATUS_(TSTATUS_NOT_OPENED, "The device or object was not opened.")

#define				TSTATUS_NOT_AVAILABLE							TSTATUS_CODE(0x034)
case_TSTATUS_(TSTATUS_NOT_AVAILABLE, "The specified resource or object is not available.")

#define				TSTATUS_INSTANCE_NOT_AVAILABLE		TSTATUS_CODE(0x035)
case_TSTATUS_(TSTATUS_INSTANCE_NOT_AVAILABLE, "The specified instance is not available.")

#define				TSTATUS_NOT_INITIALIZED						TSTATUS_CODE(0x036)
case_TSTATUS_(TSTATUS_NOT_INITIALIZED, "The module or object was not initialized.")



#define				TSTATUS_INVALID_IOCTL							TSTATUS_CODE(0x040)
case_TSTATUS_(TSTATUS_INVALID_IOCTL, "The specified IOCTL code is invalid.")

#define				TSTATUS_INVALID_PARAMETER					TSTATUS_CODE(0x041)
case_TSTATUS_(TSTATUS_INVALID_PARAMETER, "An invalid parameter was passed to the function.")

#define				TSTATUS_INVALID_LENGTH						TSTATUS_CODE(0x042)
case_TSTATUS_(TSTATUS_INVALID_LENGTH, "An invalid length was specified.")

#define				TSTATUS_INVALID_BUFFER_SIZE				TSTATUS_CODE(0x043)
case_TSTATUS_(TSTATUS_INVALID_BUFFER_SIZE, "An invalid buffer size was specified.")

#define				TSTATUS_INVALID_INBUF_SIZE				TSTATUS_CODE(0x044)
case_TSTATUS_(TSTATUS_INVALID_INBUF_SIZE, "An invalid input buffer size was specified.")

#define				TSTATUS_INVALID_OUTBUF_SIZE				TSTATUS_CODE(0x045)
case_TSTATUS_(TSTATUS_INVALID_OUTBUF_SIZE, "An invalid output buffer size was specified.")

#define				TSTATUS_INVALID_TYPE							TSTATUS_CODE(0x046)
case_TSTATUS_(TSTATUS_INVALID_TYPE, "An invalid type was specified.")

#define				TSTATUS_INVALID_INDEX							TSTATUS_CODE(0x047)
case_TSTATUS_(TSTATUS_INVALID_INDEX, "An invalid index was specified.")

#define				TSTATUS_INVALID_HANDLE						TSTATUS_CODE(0x048)
case_TSTATUS_(TSTATUS_INVALID_HANDLE, "An invalid handle was specified.")

#define				TSTATUS_INVALID_DEVICE_STATE			TSTATUS_CODE(0x049)
case_TSTATUS_(TSTATUS_INVALID_DEVICE_STATE, "The operation cannot be performed in the current state of the device.")

#define				TSTATUS_INVALID_DEVICE_CONFIG			TSTATUS_CODE(0x04A)
case_TSTATUS_(TSTATUS_INVALID_DEVICE_CONFIG, "An invalid device configuration was detected.")



#define				TSTATUS_INVALID_DESCRIPTOR				TSTATUS_CODE(0x050)
case_TSTATUS_(TSTATUS_INVALID_DESCRIPTOR, "An invalid descriptor was found.")

#define				TSTATUS_INVALID_FORMAT						TSTATUS_CODE(0x051)
case_TSTATUS_(TSTATUS_INVALID_FORMAT, "An invalid format was specified.")

#define				TSTATUS_INVALID_CONFIGURATION			TSTATUS_CODE(0x052)
case_TSTATUS_(TSTATUS_INVALID_CONFIGURATION, "An invalid configuration was specified.")

#define				TSTATUS_INVALID_MODE							TSTATUS_CODE(0x053)
case_TSTATUS_(TSTATUS_INVALID_MODE, "An invalid mode was specified.")

#define				TSTATUS_INVALID_FILE							TSTATUS_CODE(0x054)
case_TSTATUS_(TSTATUS_INVALID_FILE, "An invalid file was specified.")



#define				TSTATUS_VERSION_MISMATCH					TSTATUS_CODE(0x060)
case_TSTATUS_(TSTATUS_VERSION_MISMATCH, "The version numbers of software modules do not match.")

#define				TSTATUS_LENGTH_MISMATCH						TSTATUS_CODE(0x061)
case_TSTATUS_(TSTATUS_LENGTH_MISMATCH, "An unexpected length was detected.")

#define				TSTATUS_MAGIC_MISMATCH						TSTATUS_CODE(0x062)
case_TSTATUS_(TSTATUS_MAGIC_MISMATCH, "An unexpected marker was detected.")

#define				TSTATUS_VALUE_UNKNOWN							TSTATUS_CODE(0x063)
case_TSTATUS_(TSTATUS_VALUE_UNKNOWN, "The specified value is unknown.")

#define				TSTATUS_UNEXPECTED_DEVICE_RESPONSE	TSTATUS_CODE(0x064)
case_TSTATUS_(TSTATUS_UNEXPECTED_DEVICE_RESPONSE, "An unexpected response was received from the device.")



#define				TSTATUS_ENUM_REQUIRED							TSTATUS_CODE(0x070)
case_TSTATUS_(TSTATUS_ENUM_REQUIRED, "Device enumeration must be performed again.")

#define				TSTATUS_DEVICE_REMOVED						TSTATUS_CODE(0x071)
case_TSTATUS_(TSTATUS_DEVICE_REMOVED, "The device has been removed from the system.")

#define				TSTATUS_DEVICES_EXIST							TSTATUS_CODE(0x072)
case_TSTATUS_(TSTATUS_DEVICES_EXIST, "One or more device instances exist in the system.")

#define				TSTATUS_WRONG_DEVICE_STATE				TSTATUS_CODE(0x073)
case_TSTATUS_(TSTATUS_WRONG_DEVICE_STATE, "The device is not in the expected state.")

#define				TSTATUS_BUFFER_TOO_SMALL					TSTATUS_CODE(0x074)
case_TSTATUS_(TSTATUS_BUFFER_TOO_SMALL, "The specified buffer is too small.")


#define				TSTATUS_REGISTRY_ACCESS_FAILED		TSTATUS_CODE(0x080)
case_TSTATUS_(TSTATUS_REGISTRY_ACCESS_FAILED, "Unable to access the system registry.")

#define				TSTATUS_OBJECT_NOT_FOUND					TSTATUS_CODE(0x081)
case_TSTATUS_(TSTATUS_OBJECT_NOT_FOUND, "The specified object was not found.")

#define				TSTATUS_NOT_IN_HIGH_SPEED_MODE		TSTATUS_CODE(0x082)
case_TSTATUS_(TSTATUS_NOT_IN_HIGH_SPEED_MODE, "The device is not operating in USB high-speed mode.")

#define				TSTATUS_WRONG_DIRECTION						TSTATUS_CODE(0x083)
case_TSTATUS_(TSTATUS_WRONG_DIRECTION, "An unexpected direction argument was specified.")

#define				TSTATUS_PARAMETER_REJECTED				TSTATUS_CODE(0x084)
case_TSTATUS_(TSTATUS_PARAMETER_REJECTED, "A parameter cannot be accepted.")

#define				TSTATUS_ALREADY_OPEN							TSTATUS_CODE(0x085)
case_TSTATUS_(TSTATUS_ALREADY_OPEN, "The device or object is already opened.")



#define				TSTATUS_OPEN_FILE_FAILED					TSTATUS_CODE(0x090)
case_TSTATUS_(TSTATUS_OPEN_FILE_FAILED, "Unable to open the specified file.")

#define				TSTATUS_READ_FILE_FAILED					TSTATUS_CODE(0x091)
case_TSTATUS_(TSTATUS_READ_FILE_FAILED, "A file read operation failed.")

#define				TSTATUS_WRITE_FILE_FAILED					TSTATUS_CODE(0x092)
case_TSTATUS_(TSTATUS_WRITE_FILE_FAILED, "A file write operation failed.")



#ifdef TBASE_COMPILER_MICROSOFT
//
// Windows USB bus driver status codes, see also usb.h in the WDK
//
// mapped to 0xEF000001 ... 0xEFFFFFFF, see also TSTATUS_FROM_USBD_STATUS
//
#define				TSTATUS_USBD_STATUS_CRC                      					TSTATUS_FROM_USBD_STATUS(0xC0000001L)
case_TSTATUS_(TSTATUS_USBD_STATUS_CRC, "USB CRC error.")
#define				TSTATUS_USBD_STATUS_BTSTUFF                  					TSTATUS_FROM_USBD_STATUS(0xC0000002L)
case_TSTATUS_(TSTATUS_USBD_STATUS_BTSTUFF, "USB bit stuff error.")
#define				TSTATUS_USBD_STATUS_DATA_TOGGLE_MISMATCH     					TSTATUS_FROM_USBD_STATUS(0xC0000003L)
case_TSTATUS_(TSTATUS_USBD_STATUS_DATA_TOGGLE_MISMATCH, "USB data toggle mismatch.")
#define				TSTATUS_USBD_STATUS_STALL_PID                					TSTATUS_FROM_USBD_STATUS(0xC0000004L)
case_TSTATUS_(TSTATUS_USBD_STATUS_STALL_PID, "USB stall error.")
#define				TSTATUS_USBD_STATUS_DEV_NOT_RESPONDING       					TSTATUS_FROM_USBD_STATUS(0xC0000005L)
case_TSTATUS_(TSTATUS_USBD_STATUS_DEV_NOT_RESPONDING, "USB device not responding.")
#define				TSTATUS_USBD_STATUS_PID_CHECK_FAILURE        					TSTATUS_FROM_USBD_STATUS(0xC0000006L)
case_TSTATUS_(TSTATUS_USBD_STATUS_PID_CHECK_FAILURE, "USB PID check failure.")
#define				TSTATUS_USBD_STATUS_UNEXPECTED_PID           					TSTATUS_FROM_USBD_STATUS(0xC0000007L)
case_TSTATUS_(TSTATUS_USBD_STATUS_UNEXPECTED_PID, "USB PID unexpected.")
#define				TSTATUS_USBD_STATUS_DATA_OVERRUN             					TSTATUS_FROM_USBD_STATUS(0xC0000008L)
case_TSTATUS_(TSTATUS_USBD_STATUS_DATA_OVERRUN, "USB data overrun.")
#define				TSTATUS_USBD_STATUS_DATA_UNDERRUN            					TSTATUS_FROM_USBD_STATUS(0xC0000009L)
case_TSTATUS_(TSTATUS_USBD_STATUS_DATA_UNDERRUN, "USB data underrun.")
#define				TSTATUS_USBD_STATUS_RESERVED1                					TSTATUS_FROM_USBD_STATUS(0xC000000AL)
case_TSTATUS_(TSTATUS_USBD_STATUS_RESERVED1, "USB reserved 1")
#define				TSTATUS_USBD_STATUS_RESERVED2                					TSTATUS_FROM_USBD_STATUS(0xC000000BL)
case_TSTATUS_(TSTATUS_USBD_STATUS_RESERVED2, "USB reserved 2")
#define				TSTATUS_USBD_STATUS_BUFFER_OVERRUN           					TSTATUS_FROM_USBD_STATUS(0xC000000CL)
case_TSTATUS_(TSTATUS_USBD_STATUS_BUFFER_OVERRUN, "USB buffer overrun.")
#define				TSTATUS_USBD_STATUS_BUFFER_UNDERRUN          					TSTATUS_FROM_USBD_STATUS(0xC000000DL)
case_TSTATUS_(TSTATUS_USBD_STATUS_BUFFER_UNDERRUN, "USB buffer underrun.")
#define				TSTATUS_USBD_STATUS_NOT_ACCESSED             					TSTATUS_FROM_USBD_STATUS(0xC000000FL)
case_TSTATUS_(TSTATUS_USBD_STATUS_NOT_ACCESSED, "USB not accessed error.")
#define				TSTATUS_USBD_STATUS_FIFO                     					TSTATUS_FROM_USBD_STATUS(0xC0000010L)
case_TSTATUS_(TSTATUS_USBD_STATUS_FIFO, "USB FIFO error.")
#define				TSTATUS_USBD_STATUS_XACT_ERROR               					TSTATUS_FROM_USBD_STATUS(0xC0000011L)
case_TSTATUS_(TSTATUS_USBD_STATUS_XACT_ERROR, "USB XACT error.")
#define				TSTATUS_USBD_STATUS_BABBLE_DETECTED          					TSTATUS_FROM_USBD_STATUS(0xC0000012L)
case_TSTATUS_(TSTATUS_USBD_STATUS_BABBLE_DETECTED, "USB babble detected.")
#define				TSTATUS_USBD_STATUS_DATA_BUFFER_ERROR        					TSTATUS_FROM_USBD_STATUS(0xC0000013L)
case_TSTATUS_(TSTATUS_USBD_STATUS_DATA_BUFFER_ERROR, "USB data buffer error.")
#define				TSTATUS_USBD_STATUS_ENDPOINT_HALTED          					TSTATUS_FROM_USBD_STATUS(0xC0000030L)
case_TSTATUS_(TSTATUS_USBD_STATUS_ENDPOINT_HALTED, "USB endpoint halted.")
#define				TSTATUS_USBD_STATUS_INVALID_URB_FUNCTION     					TSTATUS_FROM_USBD_STATUS(0x80000200L)
case_TSTATUS_(TSTATUS_USBD_STATUS_INVALID_URB_FUNCTION, "USB: invalid URB function.")
#define				TSTATUS_USBD_STATUS_INVALID_PARAMETER        					TSTATUS_FROM_USBD_STATUS(0x80000300L)
case_TSTATUS_(TSTATUS_USBD_STATUS_INVALID_PARAMETER, "USB: invalid parameter.")
#define				TSTATUS_USBD_STATUS_ERROR_BUSY               					TSTATUS_FROM_USBD_STATUS(0x80000400L)
case_TSTATUS_(TSTATUS_USBD_STATUS_ERROR_BUSY, "USB: busy error.")
#define				TSTATUS_USBD_STATUS_INVALID_PIPE_HANDLE      					TSTATUS_FROM_USBD_STATUS(0x80000600L)
case_TSTATUS_(TSTATUS_USBD_STATUS_INVALID_PIPE_HANDLE, "USB: invalid pipe handle.")
#define				TSTATUS_USBD_STATUS_NO_BANDWIDTH             					TSTATUS_FROM_USBD_STATUS(0x80000700L)
case_TSTATUS_(TSTATUS_USBD_STATUS_NO_BANDWIDTH, "USB: no bandwidth.")
#define				TSTATUS_USBD_STATUS_INTERNAL_HC_ERROR        					TSTATUS_FROM_USBD_STATUS(0x80000800L)
case_TSTATUS_(TSTATUS_USBD_STATUS_INTERNAL_HC_ERROR, "USB: internal HC error.")
#define				TSTATUS_USBD_STATUS_ERROR_SHORT_TRANSFER     					TSTATUS_FROM_USBD_STATUS(0x80000900L)
case_TSTATUS_(TSTATUS_USBD_STATUS_ERROR_SHORT_TRANSFER, "USB short transfer error.")
#define				TSTATUS_USBD_STATUS_BAD_START_FRAME          					TSTATUS_FROM_USBD_STATUS(0xC0000A00L)
case_TSTATUS_(TSTATUS_USBD_STATUS_BAD_START_FRAME, "USB: bad start frame.")
#define				TSTATUS_USBD_STATUS_ISOCH_REQUEST_FAILED     					TSTATUS_FROM_USBD_STATUS(0xC0000B00L)
case_TSTATUS_(TSTATUS_USBD_STATUS_ISOCH_REQUEST_FAILED, "USB isochronous request failed.")
#define				TSTATUS_USBD_STATUS_FRAME_CONTROL_OWNED      					TSTATUS_FROM_USBD_STATUS(0xC0000C00L)
case_TSTATUS_(TSTATUS_USBD_STATUS_FRAME_CONTROL_OWNED, "USB frame control owned.")
#define				TSTATUS_USBD_STATUS_FRAME_CONTROL_NOT_OWNED  					TSTATUS_FROM_USBD_STATUS(0xC0000D00L)
case_TSTATUS_(TSTATUS_USBD_STATUS_FRAME_CONTROL_NOT_OWNED, "USB frame control owned.")
#define				TSTATUS_USBD_STATUS_NOT_SUPPORTED            					TSTATUS_FROM_USBD_STATUS(0xC0000E00L)
case_TSTATUS_(TSTATUS_USBD_STATUS_NOT_SUPPORTED, "USB request not supported.")
#define				TSTATUS_USBD_STATUS_INAVLID_CONFIGURATION_DESCRIPTOR	TSTATUS_FROM_USBD_STATUS(0xC0000F00L)
case_TSTATUS_(TSTATUS_USBD_STATUS_INAVLID_CONFIGURATION_DESCRIPTOR, "USB configuration descriptor invalid.")
#define				TSTATUS_USBD_STATUS_INSUFFICIENT_RESOURCES   					TSTATUS_FROM_USBD_STATUS(0xC0001000L)
case_TSTATUS_(TSTATUS_USBD_STATUS_INSUFFICIENT_RESOURCES, "USB: insufficient resources.")
#define				TSTATUS_USBD_STATUS_SET_CONFIG_FAILED        					TSTATUS_FROM_USBD_STATUS(0xC0002000L)
case_TSTATUS_(TSTATUS_USBD_STATUS_SET_CONFIG_FAILED, "USB set configuration failed.")
#define				TSTATUS_USBD_STATUS_BUFFER_TOO_SMALL         					TSTATUS_FROM_USBD_STATUS(0xC0003000L)
case_TSTATUS_(TSTATUS_USBD_STATUS_BUFFER_TOO_SMALL, "USB buffer too small.")
#define				TSTATUS_USBD_STATUS_INTERFACE_NOT_FOUND      					TSTATUS_FROM_USBD_STATUS(0xC0004000L)
case_TSTATUS_(TSTATUS_USBD_STATUS_INTERFACE_NOT_FOUND, "USB interface not found.")
#define				TSTATUS_USBD_STATUS_INAVLID_PIPE_FLAGS       					TSTATUS_FROM_USBD_STATUS(0xC0005000L)
case_TSTATUS_(TSTATUS_USBD_STATUS_INAVLID_PIPE_FLAGS, "USB: invalid pipe flags.")
#define				TSTATUS_USBD_STATUS_TIMEOUT                  					TSTATUS_FROM_USBD_STATUS(0xC0006000L)
case_TSTATUS_(TSTATUS_USBD_STATUS_TIMEOUT, "USB request timed out.")
#define				TSTATUS_USBD_STATUS_DEVICE_GONE              					TSTATUS_FROM_USBD_STATUS(0xC0007000L)
case_TSTATUS_(TSTATUS_USBD_STATUS_DEVICE_GONE, "USB device gone.")
#define				TSTATUS_USBD_STATUS_STATUS_NOT_MAPPED        					TSTATUS_FROM_USBD_STATUS(0xC0008000L)
case_TSTATUS_(TSTATUS_USBD_STATUS_STATUS_NOT_MAPPED, "USB: not mapped error.")
#define				TSTATUS_USBD_STATUS_HUB_INTERNAL_ERROR       					TSTATUS_FROM_USBD_STATUS(0xC0009000L)
case_TSTATUS_(TSTATUS_USBD_STATUS_HUB_INTERNAL_ERROR, "USB hub internal error.")
#define				TSTATUS_USBD_STATUS_CANCELED                 					TSTATUS_FROM_USBD_STATUS(0xC0010000L)
case_TSTATUS_(TSTATUS_USBD_STATUS_CANCELED, "USB request canceled.")
#define				TSTATUS_USBD_STATUS_ISO_NOT_ACCESSED_BY_HW   					TSTATUS_FROM_USBD_STATUS(0xC0020000L)
case_TSTATUS_(TSTATUS_USBD_STATUS_ISO_NOT_ACCESSED_BY_HW, "USB isochronous request not accessed by hardware.")
#define				TSTATUS_USBD_STATUS_ISO_TD_ERROR             					TSTATUS_FROM_USBD_STATUS(0xC0030000L)
case_TSTATUS_(TSTATUS_USBD_STATUS_ISO_TD_ERROR, "USB isochronous TD error.")
#define				TSTATUS_USBD_STATUS_ISO_NA_LATE_USBPORT      					TSTATUS_FROM_USBD_STATUS(0xC0040000L)
case_TSTATUS_(TSTATUS_USBD_STATUS_ISO_NA_LATE_USBPORT, "USB isochronous request not accessed late (USB port).")
#define				TSTATUS_USBD_STATUS_ISO_NOT_ACCESSED_LATE    					TSTATUS_FROM_USBD_STATUS(0xC0050000L)
case_TSTATUS_(TSTATUS_USBD_STATUS_ISO_NOT_ACCESSED_LATE, "USB isochronous request not accessed late.")
#define				TSTATUS_USBD_STATUS_BAD_DESCRIPTOR                    TSTATUS_FROM_USBD_STATUS(0xC0100000L)
case_TSTATUS_(TSTATUS_USBD_STATUS_BAD_DESCRIPTOR, "USB: bad descriptor.")
#define				TSTATUS_USBD_STATUS_BAD_DESCRIPTOR_BLEN               TSTATUS_FROM_USBD_STATUS(0xC0100001L)
case_TSTATUS_(TSTATUS_USBD_STATUS_BAD_DESCRIPTOR_BLEN, "USB: bad descriptor bLength.")
#define				TSTATUS_USBD_STATUS_BAD_DESCRIPTOR_TYPE               TSTATUS_FROM_USBD_STATUS(0xC0100002L)
case_TSTATUS_(TSTATUS_USBD_STATUS_BAD_DESCRIPTOR_TYPE, "USB: bad descriptor type.")
#define				TSTATUS_USBD_STATUS_BAD_INTERFACE_DESCRIPTOR          TSTATUS_FROM_USBD_STATUS(0xC0100003L)
case_TSTATUS_(TSTATUS_USBD_STATUS_BAD_INTERFACE_DESCRIPTOR, "USB: bad interface descriptor.")
#define				TSTATUS_USBD_STATUS_BAD_ENDPOINT_DESCRIPTOR           TSTATUS_FROM_USBD_STATUS(0xC0100004L)
case_TSTATUS_(TSTATUS_USBD_STATUS_BAD_ENDPOINT_DESCRIPTOR, "USB: bad endpoint descriptor.")
#define				TSTATUS_USBD_STATUS_BAD_INTERFACE_ASSOC_DESCRIPTOR    TSTATUS_FROM_USBD_STATUS(0xC0100005L)
case_TSTATUS_(TSTATUS_USBD_STATUS_BAD_INTERFACE_ASSOC_DESCRIPTOR, "USB: bad interface association descriptor.")
#define				TSTATUS_USBD_STATUS_BAD_CONFIG_DESC_LENGTH            TSTATUS_FROM_USBD_STATUS(0xC0100006L)
case_TSTATUS_(TSTATUS_USBD_STATUS_BAD_CONFIG_DESC_LENGTH, "USB: bad config descriptor length.")
#define				TSTATUS_USBD_STATUS_BAD_NUMBER_OF_INTERFACES          TSTATUS_FROM_USBD_STATUS(0xC0100007L)
case_TSTATUS_(TSTATUS_USBD_STATUS_BAD_NUMBER_OF_INTERFACES, "USB: bad number of interfaces.")
#define				TSTATUS_USBD_STATUS_BAD_NUMBER_OF_ENDPOINTS           TSTATUS_FROM_USBD_STATUS(0xC0100008L)
case_TSTATUS_(TSTATUS_USBD_STATUS_BAD_NUMBER_OF_ENDPOINTS, "USB: bad number of endpoints.")
#define				TSTATUS_USBD_STATUS_BAD_ENDPOINT_ADDRESS              TSTATUS_FROM_USBD_STATUS(0xC0100009L)
case_TSTATUS_(TSTATUS_USBD_STATUS_BAD_ENDPOINT_ADDRESS, "USB: bad endpoint address.")

#endif // TBASE_COMPILER_MICROSOFT


#if defined(TBASE_COMPILER_GNU_LINUX) | defined(TBASE_COMPILER_APPLE)
// the error numbers from Linux
#include <errno.h>
#define				TSTATUS_LINUX_EPERM              TSTATUS_CODE_ERRNO(EPERM)
case_TSTATUS_(TSTATUS_LINUX_EPERM, "Operation not permitted.")
#define				TSTATUS_LINUX_ENOENT              TSTATUS_CODE_ERRNO(ENOENT)
case_TSTATUS_(TSTATUS_LINUX_ENOENT, "No such file or directory.")
#define				TSTATUS_LINUX_ESRCH              TSTATUS_CODE_ERRNO(ESRCH)
case_TSTATUS_(TSTATUS_LINUX_ESRCH, "No such process.")
#define				TSTATUS_LINUX_EINTR             TSTATUS_CODE_ERRNO(EINTR)
case_TSTATUS_(TSTATUS_LINUX_EINTR, "Interrupted system call.")
#define				TSTATUS_LINUX_EIO             TSTATUS_CODE_ERRNO(EIO)
case_TSTATUS_(TSTATUS_LINUX_EIO, "I/O error.")
#define				TSTATUS_LINUX_ENXIO              TSTATUS_CODE_ERRNO(ENXIO)
case_TSTATUS_(TSTATUS_LINUX_ENXIO, "No such device or address.")
#define				TSTATUS_LINUX_E2BIG              TSTATUS_CODE_ERRNO(E2BIG)
case_TSTATUS_(TSTATUS_LINUX_E2BIG, "Argument list too long.")
#define				TSTATUS_LINUX_ENOEXEC              TSTATUS_CODE_ERRNO(ENOEXEC)
case_TSTATUS_(TSTATUS_LINUX_ENOEXEC, "Exec format error.")
#define				TSTATUS_LINUX_EBADF              TSTATUS_CODE_ERRNO(EBADF)
case_TSTATUS_(TSTATUS_LINUX_EBADF, "Bad file number.")
#define				TSTATUS_LINUX_ECHILD              TSTATUS_CODE_ERRNO(ECHILD)
case_TSTATUS_(TSTATUS_LINUX_ECHILD, "No child processes.")
#define				TSTATUS_LINUX_EAGAIN              TSTATUS_CODE_ERRNO(EAGAIN)
case_TSTATUS_(TSTATUS_LINUX_EAGAIN, "Try again.")
#define				TSTATUS_LINUX_ENOMEM              TSTATUS_CODE_ERRNO(ENOMEM)
case_TSTATUS_(TSTATUS_LINUX_ENOMEM, "Out of memory.")
#define				TSTATUS_LINUX_EACCES              TSTATUS_CODE_ERRNO(EACCES)
case_TSTATUS_(TSTATUS_LINUX_EACCES, "Permission denied.")
#define				TSTATUS_LINUX_EFAULT              TSTATUS_CODE_ERRNO(EFAULT)
case_TSTATUS_(TSTATUS_LINUX_EFAULT, "Bad address.")
#define				TSTATUS_LINUX_ENOTBLK              TSTATUS_CODE_ERRNO(ENOTBLK)
case_TSTATUS_(TSTATUS_LINUX_ENOTBLK, "Block device required.")
#define				TSTATUS_LINUX_EBUSY              TSTATUS_CODE_ERRNO(EBUSY)
case_TSTATUS_(TSTATUS_LINUX_EBUSY, "Device or resource busy.")
#define				TSTATUS_LINUX_EEXIST              TSTATUS_CODE_ERRNO(EEXIST)
case_TSTATUS_(TSTATUS_LINUX_EEXIST, "File exists.")
#define				TSTATUS_LINUX_EXDEV              TSTATUS_CODE_ERRNO(EXDEV)
case_TSTATUS_(TSTATUS_LINUX_EXDEV, "Cross-device link.")
#define				TSTATUS_LINUX_ENODEV              TSTATUS_CODE_ERRNO(ENODEV)
case_TSTATUS_(TSTATUS_LINUX_ENODEV, "No such device.")
#define				TSTATUS_LINUX_ENOTDIR              TSTATUS_CODE_ERRNO(ENOTDIR)
case_TSTATUS_(TSTATUS_LINUX_ENOTDIR, "Not a directory.")
#define				TSTATUS_LINUX_EISDIR              TSTATUS_CODE_ERRNO(EISDIR)
case_TSTATUS_(TSTATUS_LINUX_EISDIR, "Is a directory.")
#define				TSTATUS_LINUX_EINVAL              TSTATUS_CODE_ERRNO(EINVAL)
case_TSTATUS_(TSTATUS_LINUX_EINVAL, "Invalid argument.")
#define				TSTATUS_LINUX_ENFILE              TSTATUS_CODE_ERRNO(ENFILE)
case_TSTATUS_(TSTATUS_LINUX_ENFILE, "File table overflow.")
#define				TSTATUS_LINUX_EMFILE              TSTATUS_CODE_ERRNO(EMFILE)
case_TSTATUS_(TSTATUS_LINUX_EMFILE, "Too many open files.")
#define				TSTATUS_LINUX_ENOTTY              TSTATUS_CODE_ERRNO(ENOTTY)
case_TSTATUS_(TSTATUS_LINUX_ENOTTY, "Not a typewriter.")
#define				TSTATUS_LINUX_ETXTBSY              TSTATUS_CODE_ERRNO(ETXTBSY)
case_TSTATUS_(TSTATUS_LINUX_ETXTBSY, "Text file busy.")
#define				TSTATUS_LINUX_EFBIG              TSTATUS_CODE_ERRNO(EFBIG)
case_TSTATUS_(TSTATUS_LINUX_EFBIG, "File too large.")
#define				TSTATUS_LINUX_ENOSPC              TSTATUS_CODE_ERRNO(ENOSPC)
case_TSTATUS_(TSTATUS_LINUX_ENOSPC, "No space left on device.")
#define				TSTATUS_LINUX_ESPIPE              TSTATUS_CODE_ERRNO(ESPIPE)
case_TSTATUS_(TSTATUS_LINUX_ESPIPE, "Illegal seek.")
#define				TSTATUS_LINUX_EROFS              TSTATUS_CODE_ERRNO(EROFS)
case_TSTATUS_(TSTATUS_LINUX_EROFS, "Read-only file system.")
#define				TSTATUS_LINUX_EMLINK              TSTATUS_CODE_ERRNO(EMLINK)
case_TSTATUS_(TSTATUS_LINUX_EMLINK, "Too many links.")
#define				TSTATUS_LINUX_EPIPE              TSTATUS_CODE_ERRNO(EPIPE)
case_TSTATUS_(TSTATUS_LINUX_EPIPE, "Broken pipe.")
#define				TSTATUS_LINUX_EDOM              TSTATUS_CODE_ERRNO(EDOM)
case_TSTATUS_(TSTATUS_LINUX_EDOM, "Math argument out of domain of func.")
#define				TSTATUS_LINUX_ERANGE              TSTATUS_CODE_ERRNO(ERANGE)
case_TSTATUS_(TSTATUS_LINUX_ERANGE, "Math result not representable.")

#endif // defined(TBASE_COMPILER_GNU_LINUX) | defined(TBASE_COMPILER_APPLE)


// project-specific status codes
#include "tstatus_codes_ex.h"


#if !defined(__inside_tstatus_codes_str_h__)
#undef case_TSTATUS_
#endif


#undef __inside_tstatus_codes_h__


#endif	//#ifndef RC_INVOKED

#endif  // __tstatus_codes_h__

/*************************** EOF **************************************/
