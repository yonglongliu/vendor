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

/*******************************************************************************
 *  Module:         tusb_cls_hid.h
 *  Description:    USB Device Class Specification for Human Interface Devcies
 ******************************************************************************/

#ifndef __tusb_cls_hid_h__
#define __tusb_cls_hid_h__

#include "tusb_spec.h"


/* pack the following structures  */                                                   
#include "tbase_pack1.h"


/*////////////////////////////////////////////////////////////////////////
//
// HID specific subclasses
//
//////////////////////////////////////////////////////////////////////// */

#define TUSB_CLS_HID_SUB_NONE								0x00
#define TUSB_CLS_HID_SUB_BOOT_INTERFACE     0x01

/*////////////////////////////////////////////////////////////////////////
//
// HID specific protocols
//
//////////////////////////////////////////////////////////////////////// */

#define TUSB_CLS_HID_PROT_NONE							0x00
#define TUSB_CLS_HID_PROT_KEYBOARD					0x01
#define TUSB_CLS_HID_PROT_MOUSE							0x02


/*////////////////////////////////////////////////////////////////////////
//
// HID specific descriptor types
//
//////////////////////////////////////////////////////////////////////// */

#define TUSB_CLS_HID_DESCRIPTOR             0x21
#define TUSB_CLS_HID_REPORT_DESCRIPTOR      0x22
#define TUSB_CLS_HID_PHYSICAL_DESCRIPTOR    0x23


/* ////////////////////////////////////////////////////////////////////////
//
// HID specific report types
//
//////////////////////////////////////////////////////////////////////// */

#define TUSB_CLS_HID_REPORT_TYPE_INPUT			0x01
#define TUSB_CLS_HID_REPORT_TYPE_OUTPUT			0x02
#define TUSB_CLS_HID_REPORT_TYPE_FEATURE		0x03

/*////////////////////////////////////////////////////////////////////////
//
// HID specific requests
//
//////////////////////////////////////////////////////////////////////// */
#define TUSB_CLS_HID_REQ_GET_REPORT         0x01
#define TUSB_CLS_HID_REQ_GET_IDLE		        0x02
#define TUSB_CLS_HID_REQ_GET_PROTOCOL       0x03
#define TUSB_CLS_HID_REQ_SET_REPORT         0x09
#define TUSB_CLS_HID_REQ_SET_IDLE						0x0A
#define TUSB_CLS_HID_REQ_SET_PROTOCOL				0x0B

/*////////////////////////////////////////////////////////////////////////
//
// HID specific protocol constants
//
//////////////////////////////////////////////////////////////////////// */
#define TUSB_CLS_HID_BOOT_PROTOCOL					0x00
#define TUSB_CLS_HID_REPORT_PROTOCOL				0x01

/*////////////////////////////////////////////////////////////////////////
//
// HID descriptor bCountryCode constants
// bCountryCode identifies which country the hardware is localized for
// devices are not required to place a value other than zero in this field,
//  but some operating environments may require this information
// keyboards may use the field to indicate the language of the key caps
//
//////////////////////////////////////////////////////////////////////// */
#define TUSB_CLS_HID_COUNTRY_CODE_NOT_SUPPORTED	0
#define TUSB_CLS_HID_COUNTRY_CODE_GERMAN				9
#define TUSB_CLS_HID_COUNTRY_CODE_ISO						13
#define TUSB_CLS_HID_COUNTRY_CODE_UK						32
#define TUSB_CLS_HID_COUNTRY_CODE_US						33


/*////////////////////////////////////////////////////////////////////////
//
// HID keyboard input report modifier byte constants
//
//////////////////////////////////////////////////////////////////////// */
#define TUSB_CLS_HID_KBD_MODIFIER_LEFTCTRL			(0x1U<<0)
#define TUSB_CLS_HID_KBD_MODIFIER_LEFTSHIFT			(0x1U<<1)
#define TUSB_CLS_HID_KBD_MODIFIER_LEFTALT				(0x1U<<2)
#define TUSB_CLS_HID_KBD_MODIFIER_LEFTGUI				(0x1U<<3)
#define TUSB_CLS_HID_KBD_MODIFIER_RIGHTCTRL			(0x1U<<4)
#define TUSB_CLS_HID_KBD_MODIFIER_RIGHTSHIFT		(0x1U<<5)
#define TUSB_CLS_HID_KBD_MODIFIER_RIGHTALT			(0x1U<<6)
#define TUSB_CLS_HID_KBD_MODIFIER_RIGHTGUI			(0x1U<<7)


/*////////////////////////////////////////////////////////////////////////
//
// HID keyboard output report byte constants
//
//////////////////////////////////////////////////////////////////////// */
#define TUSB_CLS_HID_KBD_LED_NUMLOCK						(0x1U<<0)
#define TUSB_CLS_HID_KBD_LED_CAPSLOCK						(0x1U<<1)
#define TUSB_CLS_HID_KBD_LED_SCROLLLOCK					(0x1U<<2)
#define TUSB_CLS_HID_KBD_LED_COMPOSE						(0x1U<<3)
#define TUSB_CLS_HID_KBD_LED_KANA								(0x1U<<4)


/*////////////////////////////////////////////////////////////////////////
//
// Usage page for HID keyboard/keypad
// see Universal Serial Bus HID Usage Tables document, section 10
//
//////////////////////////////////////////////////////////////////////// */
// returns the HID keyboard code for characters a to z
#define TUSB_CLS_HID_KBD_USAGE_GET_KEY_CODE_a_TO_z(lowerCaseChar) (4 + (lowerCaseChar) - 'a')

#define TUSB_CLS_HID_KBD_USAGE_NUMLOCKANDCLEAR	0x53
// to be extended


/*////////////////////////////////////////////////////////////////////////
//
// HID descriptor structures
//
//////////////////////////////////////////////////////////////////////// */

typedef struct tagT_UsbClsHidDescriptorType {
	T_UINT8 bDescriptorType;
	T_LE_UINT16 wDescriptorLength;	/* unaligned */
} T_UsbClsHidDescriptorType;

#define TUSB_SIZEOF_HID_DESC_TYPE					 3
TB_CHECK_SIZEOF(T_UsbClsHidDescriptorType, 3);

typedef struct tagT_UsbClsHidDescriptor
{
	T_UINT8 bDescLength;
	T_UINT8 bDescriptorType;
	T_LE_UINT16 bcdHID;							/* unaligned */
	T_UINT8 bCountryCode;
	T_UINT8 bNumDescriptors;
	T_UINT8 bDescriptorTypeCls;
	T_LE_UINT16 wDescriptorLength;	/* unaligned */
} T_UsbClsHidDescriptor;
/* may be followed by further bDescriptorType/wDescriptorLength values
   for optional descriptors */

#define TUSB_SIZEOF_HID_DESC					 9
TB_CHECK_SIZEOF(T_UsbClsHidDescriptor, 9);

//
// HID Report Descriptor
//

// short items (bType)
#define TUSB_CLS_HID_TYPE_MAIN						0x0
#define TUSB_CLS_HID_TYPE_GLOBAL					0x1
#define TUSB_CLS_HID_TYPE_LOCAL						0x2
// long item
#define TUSB_CLS_HID_TYPE_RESERVED				0x3

// short item sizes (bSize)
#define TUSB_CLS_HID_SIZE_0								0x0
#define TUSB_CLS_HID_SIZE_1								0x1
#define TUSB_CLS_HID_SIZE_2								0x2
#define TUSB_CLS_HID_SIZE_4								0x3

// main item tags (bTag)
#define TUSB_CLS_HID_MAIN_INPUT						0x8
#define TUSB_CLS_HID_MAIN_OUTPUT					0x9
#define TUSB_CLS_HID_MAIN_FEATURE					0xB
#define TUSB_CLS_HID_MAIN_COLLECTION			0xA
#define TUSB_CLS_HID_MAIN_END_COLLECTION	0xC

// global item tags (bTag)
#define TUSB_CLS_HID_GLOBAL_USAGE_PAGE		0x0
#define TUSB_CLS_HID_GLOBAL_LOGICAL_MIN		0x1
#define TUSB_CLS_HID_GLOBAL_LOGICAL_MAX		0x2
#define TUSB_CLS_HID_GLOBAL_PHYSICAL_MIN	0x3
#define TUSB_CLS_HID_GLOBAL_PHYSICAL_MAX	0x4
#define TUSB_CLS_HID_GLOBAL_UNIT_EXPONENT	0x5
#define TUSB_CLS_HID_GLOBAL_UNIT					0x6
#define TUSB_CLS_HID_GLOBAL_REPORT_SIZE		0x7
#define TUSB_CLS_HID_GLOBAL_REPORT_ID			0x8
#define TUSB_CLS_HID_GLOBAL_REPORT_COUNT	0x9
#define TUSB_CLS_HID_GLOBAL_PUSH					0xA
#define TUSB_CLS_HID_GLOBAL_POP						0xB

// local item tags (bTag)
#define TUSB_CLS_HID_LOCAL_USAGE					0x0
#define TUSB_CLS_HID_LOCAL_USAGE_MIN			0x1
#define TUSB_CLS_HID_LOCAL_USAGE_MAX			0x2
#define TUSB_CLS_HID_LOCAL_DESIGNATOR_IDX	0x3
#define TUSB_CLS_HID_LOCAL_DESIGNATOR_MIN	0x4
#define TUSB_CLS_HID_LOCAL_DESIGNATOR_MAX	0x5
#define TUSB_CLS_HID_LOCAL_STRING_IDX			0x7
#define TUSB_CLS_HID_LOCAL_STRING_MIN			0x8
#define TUSB_CLS_HID_LOCAL_STRING_MAX			0x9
#define TUSB_CLS_HID_LOCAL_DELIMITER			0xA


#define TUSB_CLS_HID_SHORT_ITEM(bSize, bType, bTag) (((bSize) & 0x3) | (((bType) & 0x3) << 2) | (((bTag) & 0xF) << 4))
#define TUSB_CLS_HID_LONG_ITEM	0xFE

/* restore packing */
#include "tbase_packrestore.h"


#endif /* __tusb_cls_hid_h__ */

/****************************** EOF ****************************************/
