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
 *  Module:         tusb_spec.h
 *  Description:    USB Specification
 ******************************************************************************/

#ifndef __tusb_spec_h__
#define __tusb_spec_h__

#include "tbase_platform.h"
#include "tbase_types.h"

// pack the following structures                                                     
#include "tbase_pack1.h"


//////////////////////////////////////////////////////////////////////////
//
// version number of the USB Spec. used for bcdUSB
//
// NOTE: these defines assume the little endian byte order
//
//////////////////////////////////////////////////////////////////////////

#define TUSB_VER_100     0x0100		/* v1.0 */
#define TUSB_VER_110     0x0110		/* v1.1 */
#define TUSB_VER_200     0x0200		/* v2.0 */
#define TUSB_VER_210     0x0210		/* v2.1 used if USB3 devices runs at HS, equal or greater versions supports the BOS descriptor */
#define TUSB_VER_300     0x0300		/* v3.0 */
#define TUSB_VER_310     0x0310		/* v3.1 */



//////////////////////////////////////////////////////////////////////////
//
// USB speeds (not really defined by the spec but useful)
//
//////////////////////////////////////////////////////////////////////////

#define TUSB_UNKNOWN_SPEED		0
#define TUSB_LOW_SPEED				1		/* 1.5 Mbps */
#define TUSB_FULL_SPEED				2		/*  12 Mbps */
#define TUSB_HIGH_SPEED				3		/* 480 Mbps */
#define TUSB_SUPER_SPEED			4		/*   5 Gbps */



//////////////////////////////////////////////////////////////////////////
//
// USB framing
//
//////////////////////////////////////////////////////////////////////////

// USB frame frequency
#define TUSB_FRAMES_PER_SECOND						1000
#define TUSB_MICRO_FRAMES_PER_SECOND			8000
#define TUSB_MICRO_FRAMES_PER_FRAME				8



//////////////////////////////////////////////////////////////////////////
//
// descriptor types
//
//////////////////////////////////////////////////////////////////////////

#define TUSB_DEVICE_DESCRIPTOR														0x01
#define TUSB_CONFIGURATION_DESCRIPTOR											0x02
#define TUSB_STRING_DESCRIPTOR														0x03
#define TUSB_INTERFACE_DESCRIPTOR													0x04
#define TUSB_ENDPOINT_DESCRIPTOR													0x05
/* high speed only */
#define TUSB_DEVICE_QUALIFIER_DESCRIPTOR									0x06
#define TUSB_OTHER_SPEED_CONFIGURATION_DESCRIPTOR					0x07
/* all speeds */
#define TUSB_INTERFACE_ASSOCIATION_DESCRIPTOR							0x0B

/* super speed only */
#define TUSB_BINARY_DEVICE_OBJECT_STORE_DESCRIPTOR        0x0F
#define TUSB_DEVICE_CAPABILITY_DESCRIPTOR                 0x10
#define TUSB_SUPERSPEED_USB_ENDPOINT_COMPANION_DESCRIPTOR 0x30

/* wireless USB */
#define TUSB_WIRELESS_SECURITY_DESCRIPTOR                 0x0C
#define TUSB_WIRELESS_KEY_DESCRIPTOR                      0x0C
#define TUSB_WIRELESS_ENCRYPTION_TYPE_DESCRIPTOR          0x0E
#define TUSB_WIRELESS_ENDPOINT_COMPANION_DESCRIPTOR       0x11

/* class specific descriptors, used by various classes, e.g. CDC, MIDI */
#define TUSB_CS_INTERFACE_DESCRIPTOR											0x24
#define TUSB_CS_ENDPOINT_DESCRIPTOR												0x25



//////////////////////////////////////////////////////////////////////////
//
// USB requests codes
//
//////////////////////////////////////////////////////////////////////////

#define TUSB_REQ_GET_STATUS           0x00
#define TUSB_REQ_CLEAR_FEATURE        0x01
#define TUSB_REQ_SET_FEATURE          0x03
#define TUSB_REQ_SET_ADDRESS          0x05
#define TUSB_REQ_GET_DESCRIPTOR       0x06
#define TUSB_REQ_SET_DESCRIPTOR       0x07
#define TUSB_REQ_GET_CONFIGURATION    0x08
#define TUSB_REQ_SET_CONFIGURATION    0x09
#define TUSB_REQ_GET_INTERFACE        0x0A
#define TUSB_REQ_SET_INTERFACE        0x0B
#define TUSB_REQ_SYNCH_FRAME          0x0C
// USB 3.0 extension
#define TUSB_REQ_SET_SEL							0x30
#define TUSB_REQ_SET_ISOCH_DELAY			0x31


//////////////////////////////////////////////////////////////////////////
//
// direction bit
//
//////////////////////////////////////////////////////////////////////////

#define TUSB_DIRECTION_IN					0x80
#define TUSB_DIRECTION_OUT				0x00


//////////////////////////////////////////////////////////////////////////
//
// USB feature selector (for SET_FEATURE request)
//
//////////////////////////////////////////////////////////////////////////

/* recipient == device */
#define TUSB_STD_FEATURE_SELECTOR_DEVICE_REMOTE_WAKEUP	0x01
#define TUSB_STD_FEATURE_SELECTOR_DEVICE_TEST_MODE			0x02
// USB 3.0 extension
#define TUSB_STD_FEATURE_SELECTOR_DEVICE_U1_ENABLE			0x30
#define TUSB_STD_FEATURE_SELECTOR_DEVICE_U2_ENABLE			0x31
#define TUSB_STD_FEATURE_SELECTOR_DEVICE_LTM_ENABLE			0x32
#define TUSB_STD_FEATURE_SELECTOR_DEVICE_LDM_ENABLE			0x35

/* recipient == interface */
#define TUSB_STD_FEATURE_SELECTOR_IF_FUNCTION_SUSPEND		0x00

/* recipient == endpoint */
#define TUSB_STD_FEATURE_SELECTOR_ENDPOINT_HALT					0x00


//////////////////////////////////////////////////////////////////////////
//
// USB status information (for GET_STATUS request)
//
//////////////////////////////////////////////////////////////////////////

/* status type */
#define TUSB_STATUS_TYPE_STANDARD							0x00
#define TUSB_STATUS_TYPE_PTM									0x01

/* recipient == device */
#define TUSB_STATUS_DEVICE_SELF_POWERED				0x01
#define TUSB_STATUS_DEVICE_REMOTE_WAKEUP			0x02
/* USB 3.0 extension */
#define TUSB_STATUS_DEVICE_U1_ENABLE					0x04
#define TUSB_STATUS_DEVICE_U2_ENABLE					0x08
#define TUSB_STATUS_DEVICE_LTM_ENABLE					0x10

/* recipient == interface */
/* USB 3.0 extension */
#define TUSB_STATUS_IF_FUNC_REMOTE_WAKE_CAP		0x01
#define TUSB_STATUS_IF_FUNC_REMOTE_WAKEUP			0x02

/* recipient == endpoint */
#define TUSB_STATUS_ENDPOINT_HALT							0x01



//////////////////////////////////////////////////////////////////////////
//
// USB class codes
//
//////////////////////////////////////////////////////////////////////////

#define TUSB_CLASS_CODE_RESERVED              0x00
#define TUSB_CLASS_CODE_AUDIO                 0x01
#define TUSB_CLASS_CODE_COMMUNICATION         0x02
#define TUSB_CLASS_CODE_HUMAN_INTERFACE       0x03
#define TUSB_CLASS_CODE_PHYSICAL              0x05
#define TUSB_CLASS_CODE_IMAGING               0x06
#define TUSB_CLASS_CODE_PRINTER               0x07
#define TUSB_CLASS_CODE_MASS_STORAGE          0x08
#define TUSB_CLASS_CODE_HUB                   0x09
#define TUSB_CLASS_CODE_CDC_DATA              0x0A
#define TUSB_CLASS_CODE_SMART_CARD            0x0B
#define TUSB_CLASS_CODE_CONTENT_SECURITY      0x0D
#define TUSB_CLASS_CODE_VIDEO                 0x0E
#define TUSB_CLASS_CODE_PERSONAL_HEALTHCARE   0x0F
#define TUSB_CLASS_CODE_DIAGNOSTIC_DEVICE     0xDC
#define TUSB_CLASS_CODE_WIRELESS_CONTROLLER   0xE0
#define TUSB_CLASS_CODE_MISCELLANEOUS         0xEF
#define TUSB_CLASS_CODE_APPLICATION_SPECIFIC  0xFE
#define TUSB_CLASS_CODE_VENDOR_SPECIFIC       0xFF

//////////////////////////////////////////////////////////////////////////
//
// Miscellaneous class specific sub class codes
//
//////////////////////////////////////////////////////////////////////////
#define TUSB_CLS_MISC_SUB_COMMON_CLASS				0x02


//////////////////////////////////////////////////////////////////////////
//
// Miscellaneous class specific protocol codes
//
//////////////////////////////////////////////////////////////////////////
#define TUSB_CLS_MISC_PROT_IAD								0x01



//////////////////////////////////////////////////////////////////////////
//
// USB max. String Descriptor length
//
//////////////////////////////////////////////////////////////////////////

#define TUSB_MAX_STRING_DESC_LENGTH          256                                      // max bytes for the whole string descriptor
#define TUSB_MAX_STRING_DESC_STRING_LENGTH   (TUSB_MAX_STRING_DESC_LENGTH - 2)        // max bytes for the bString element
#define TUSB_MAX_STRING_DESC_CHARS           (TUSB_MAX_STRING_DESC_STRING_LENGTH / 2) // max chars for the bString element

//////////////////////////////////////////////////////////////////////////
//
// standard USB structs (e.g. setup packet)
//
// NOTE: all structs assume the little endian byte order
//
//////////////////////////////////////////////////////////////////////////

typedef struct tagT_UsbSetupPacket
{
  T_UINT8      bmRequestType;
  T_UINT8      bRequest;
  T_LE_UINT16  wValue;
  T_LE_UINT16  wIndex;
  T_LE_UINT16  wLength;  
} T_UsbSetupPacket;

#define TUSB_SIZEOF_SETUP_PACKET	8
TB_CHECK_SIZEOF(T_UsbSetupPacket, 8);

//////////////////////////////////////////////////////////////////////////
// USB setup request recipient (Bit 4..0 of bmRequestType field)
//////////////////////////////////////////////////////////////////////////
#define TUSB_SETUP_RECIPIENT_FROM_REQUEST(bmRequestType)  ( (bmRequestType) & 0x1f )
#define TUSB_REQUEST_FROM_SETUP_RECIPIENT(val)        ( (val) & 0x1f )
//values
#define TUSB_SETUP_RECIPIENT_DEVICE    0
#define TUSB_SETUP_RECIPIENT_INTERFACE 1
#define TUSB_SETUP_RECIPIENT_ENDPOINT  2
#define TUSB_SETUP_RECIPIENT_OTHER     3

//////////////////////////////////////////////////////////////////////////
// USB setup request types (Bit 6..5 in bmRequestType field)
//////////////////////////////////////////////////////////////////////////
#define TUSB_SETUP_TYPE_FROM_REQUEST(bmRequestType) ( ((bmRequestType) >> 5) & 0x3 )
#define TUSB_REQUEST_FROM_SETUP_TYPE(val)       ( ((val) & 0x3) << 5 )
// values
#define TUSB_SETUP_TYPE_STANDARD  0
#define TUSB_SETUP_TYPE_CLASS     1
#define TUSB_SETUP_TYPE_VENDOR    2
#define TUSB_SETUP_TYPE_RESERVED  3

//////////////////////////////////////////////////////////////////////////
// USB setup request direction bit checks (Bit 7 of bmRequestType field)
//////////////////////////////////////////////////////////////////////////
#define TUSB_IS_IN_SETUP_REQUEST(bmRequestType)   ( 0 != ((bmRequestType) & TUSB_DIRECTION_IN) )
#define TUSB_IS_OUT_SETUP_REQUEST(bmRequestType)  ( 0 == ((bmRequestType) & TUSB_DIRECTION_IN) )

//////////////////////////////////////////////////////////////////////////
// create value for bmRequestType field
// recipient = TUSB_SETUP_RECIPIENT_xxx
// type = TUSB_SETUP_TYPE_xxx
// direction = TUSB_DIRECTION_IN or TUSB_DIRECTION_OUT
//////////////////////////////////////////////////////////////////////////
#define TUSB_REQUEST_TYPE_CODE(recipient, type, direction )\
	( TUSB_REQUEST_FROM_SETUP_RECIPIENT(recipient) | TUSB_REQUEST_FROM_SETUP_TYPE(type) | ((direction)&TUSB_DIRECTION_IN) )



//////////////////////////////////////////////////////////////////////////
// NOTE: Windows limits the length of a SETUP data phase to 4KB
//////////////////////////////////////////////////////////////////////////
#define TUSB_WINDOWS_MAX_EP0_DATAPHASE_LENGTH		4096

//////////////////////////////////////////////////////////////////////////
// timeout intervals, in ms, for standard device requests (see USB 2 spec, chapter 9.2.6.4)
//////////////////////////////////////////////////////////////////////////
#define TUSB_STANDARD_REQUEST_TIMEOUT_NO_DATA   50
#define TUSB_STANDARD_REQUEST_TIMEOUT_DATA_IN   500
#define TUSB_STANDARD_REQUEST_TIMEOUT_DATA_OUT  5000



//////////////////////////////////////////////////////////////////////////
//
// standard descriptor structs (USB 1.1 and USB 2.0)
//
// NOTE: all structs assume the little endian byte order
//
//////////////////////////////////////////////////////////////////////////

/* descriptor header (common descriptor) */
typedef struct tagT_UsbDescriptorHeader
{
  T_UINT8 bLength;
  T_UINT8 bDescriptorType;
} T_UsbDescriptorHeader;

#define TUSB_SIZEOF_DESC_HEADER				 2
TB_CHECK_SIZEOF(T_UsbDescriptorHeader, 2);


//////////////////////////////////////////////////////////////////////////
//
// USB Device descriptor
//
//////////////////////////////////////////////////////////////////////////
typedef struct tagT_UsbDeviceDescriptor
{
  T_UINT8      bLength;
  T_UINT8      bDescriptorType;
  T_LE_UINT16  bcdUSB;
  T_UINT8      bDeviceClass;
  T_UINT8      bDeviceSubClass;
  T_UINT8      bDeviceProtocol;
  T_UINT8      bMaxPacketSize0;
  T_LE_UINT16  idVendor;
  T_LE_UINT16  idProduct;
  T_LE_UINT16  bcdDevice;
  T_UINT8      iManufacturer;
  T_UINT8      iProduct;
  T_UINT8      iSerialNumber;
  T_UINT8      bNumConfigurations;
} T_UsbDeviceDescriptor;

#define TUSB_SIZEOF_DEVICE_DESC				 18
TB_CHECK_SIZEOF(T_UsbDeviceDescriptor, 18);


//////////////////////////////////////////////////////////////////////////
//
// USB Configuration descriptor
//
//////////////////////////////////////////////////////////////////////////
typedef struct tagT_UsbConfigurationDescriptor
{
  T_UINT8      bLength;
  T_UINT8      bDescriptorType;
  T_LE_UINT16	 wTotalLength;
  T_UINT8      bNumInterfaces;
  T_UINT8      bConfigurationValue;
  T_UINT8      iConfiguration;
  T_UINT8      bmAttributes;
  T_UINT8      bMaxPower;
} T_UsbConfigurationDescriptor;

#define TUSB_SIZEOF_CONFIGURATION_DESC 			  9
TB_CHECK_SIZEOF(T_UsbConfigurationDescriptor, 9);


//////////////////////////////////////////////////////////////////////////
// USB Configuration attributes (bmAttributes)
//////////////////////////////////////////////////////////////////////////

#define TUSB_CONFIG_ATTRIBUTE_BUS_POWER			0x80
#define TUSB_CONFIG_ATTRIBUTE_SELF_POWER		0x40
#define TUSB_CONFIG_ATTRIBUTE_REMOTE_WAKEUP	0x20


//////////////////////////////////////////////////////////////////////////
//
// USB Interface descriptor
//
//////////////////////////////////////////////////////////////////////////
typedef struct tagT_UsbInterfaceDescriptor
{
  T_UINT8   bLength;
  T_UINT8   bDescriptorType;
  T_UINT8   bInterfaceNumber;
  T_UINT8   bAlternateSetting;
  T_UINT8   bNumEndpoints;
  T_UINT8   bInterfaceClass;
  T_UINT8   bInterfaceSubClass;
  T_UINT8   bInterfaceProtocol;
  T_UINT8   iInterface;
} T_UsbInterfaceDescriptor;

#define TUSB_SIZEOF_INTERFACE_DESC				9
TB_CHECK_SIZEOF(T_UsbInterfaceDescriptor, 9);


//////////////////////////////////////////////////////////////////////////
//
// USB Endpoint descriptor
//
//////////////////////////////////////////////////////////////////////////
typedef struct tagT_UsbEndpointDescriptor
{
  T_UINT8      bLength;
  T_UINT8      bDescriptorType;
  T_UINT8      bEndpointAddress;
  T_UINT8      bmAttributes;
  T_LE_UINT16  wMaxPacketSize;	/* unaligned */
  T_UINT8      bInterval;
} T_UsbEndpointDescriptor;

#define TUSB_SIZEOF_ENDPOINT_DESC				 7
TB_CHECK_SIZEOF(T_UsbEndpointDescriptor, 7);


//////////////////////////////////////////////////////////////////////////
// USB Endpoint direction
//////////////////////////////////////////////////////////////////////////

#define TUSB_IS_IN_DIRECTION_EP(bEndpointAddress)     ( 0 != ((bEndpointAddress) & TUSB_DIRECTION_IN) )
#define TUSB_IS_OUT_DIRECTION_EP(bEndpointAddress)    ( 0 == ((bEndpointAddress) & TUSB_DIRECTION_IN) )

//////////////////////////////////////////////////////////////////////////
// USB Endpoint Transfer types (Bit 1..0 of bmAttributes)
//////////////////////////////////////////////////////////////////////////

#define TUSB_ATTR_FROM_TRANSFER_TYPE(val)						( (val) & 0x3 )
#define TUSB_TRANSFER_TYPE_FROM_ATTR(bmAttributes)	( (bmAttributes) & 0x3 )
// values
#define TUSB_TRANSFER_TYPE_CONTROL      0
#define TUSB_TRANSFER_TYPE_ISOCHRONOUS  1
#define TUSB_TRANSFER_TYPE_BULK         2
#define TUSB_TRANSFER_TYPE_INTERRUPT    3

//////////////////////////////////////////////////////////////////////////
// USB Endpoint Synchronization Type (Bit 3..2 of bmAttributes)
// (only defined for Isochronous endpoints)
//////////////////////////////////////////////////////////////////////////

#define TUSB_ATTR_FROM_SYNCH(val)						( ((val)&0x3)<<2 )
#define TUSB_SYNCH_FROM_ATTR(bmAttributes)	( ((bmAttributes)>>2)&0x3 )
// values
#define TUSB_SYNCHRONISATION_NONE            0
#define TUSB_SYNCHRONISATION_ASYNCHRONOUS    1
#define TUSB_SYNCHRONISATION_ADAPTIVE        2
#define TUSB_SYNCHRONISATION_SYNCHRONOUS     3

//////////////////////////////////////////////////////////////////////////
// USB Endpoint Usage Type (Bit 5..4 of bmAttributes)
// defined for Isochronous and Interrupt endpoints
//////////////////////////////////////////////////////////////////////////

#define TUSB_ATTR_FROM_USAGE_TYPE(val)					( ((val)&0x3)<<4 )
#define TUSB_USAGE_TYPE_FROM_ATTR(bmAttributes)	( ((bmAttributes)>>4)&0x3 )
// values for ISO endpoints 
#define TUSB_USAGE_TYPE_DATA       0
#define TUSB_USAGE_TYPE_FEEDBACK   1
#define TUSB_USAGE_TYPE_IMPLICIT   2
#define TUSB_USAGE_TYPE_RESERVED   3
// values for Interrupt endpoints 
#define TUSB_USAGE_TYPE_PERIODIC       0
#define TUSB_USAGE_TYPE_NOTIFICATION   1

//////////////////////////////////////////////////////////////////////////
// Max Packet Size (interpretation depends on endpoint type and speed)
//////////////////////////////////////////////////////////////////////////
// HS Isoch and Interrupt endpoints. USE THIS MACRO WITH native CPU type T_UINT16.
#define TUSB_SINGLE_PACKET_SIZE_ISO_INT_HS(wMaxPacketSize)	( ((wMaxPacketSize) & 0x7ff) )
#define TUSB_PACKET_COUNT_ISO_INT_HS(wMaxPacketSize)				( (1+(((wMaxPacketSize)>>11)&0x03)) )
#define TUSB_MAX_PACKET_SIZE_ISO_INT_HS(wMaxPacketSize)			( TUSB_SINGLE_PACKET_SIZE_ISO_INT_HS(wMaxPacketSize) * TUSB_PACKET_COUNT_ISO_INT_HS(wMaxPacketSize) )
// all other FS and HS endpoints. USE THIS MACRO WITH native CPU type T_UINT16.
#define TUSB_MAX_PACKET_SIZE(wMaxPacketSize)								( (wMaxPacketSize) & 0x7ff )

// For HS Isoch and Interrupt EPs only:
// transaction count from max packet size (1..3072)
#define TUSB_XACT_COUNT_FROM_PACKET_SIZE(packetSize)				( ((packetSize) + 1023) / 1024 )
// transaction size from max packet size (1..3072)
#define TUSB_XACT_SIZE_FROM_PACKET_SIZE(packetSize)					( (packetSize) / TUSB_XACT_COUNT_FROM_PACKET_SIZE(packetSize) )
// wMaxPacketSize field from XACT count and size
#define TUSB_wMaxPacketSize_FROM_XACT(xactCount, xactSize)	( ((((xactCount)-1)&0x3)<<11) | ((xactSize)&0x7ff) )
// wMaxPacketSize field from max packet size (1..3072)
#define TUSB_wMaxPacketSize_FROM_PACKET_SIZE(packetSize)		TUSB_wMaxPacketSize_FROM_XACT( TUSB_XACT_COUNT_FROM_PACKET_SIZE(packetSize), TUSB_XACT_SIZE_FROM_PACKET_SIZE(packetSize) )


//////////////////////////////////////////////////////////////////////////
// Interval (interpretation depends on endpoint type and speed)
//////////////////////////////////////////////////////////////////////////
// FS/HS Isoch endpoints: interval = 2^(n-1) frames/microframes
// HS Interrupt endpoints: interval = 2^(n-1) microframes
#define TUSB_EP_INTERVAL_POWER2(bInterval)		( 0x1U << ((bInterval)-1) )
// all other FS and HS endpoints: bInterval specifies frames/microframes
#define TUSB_EP_INTERVAL(bInterval)						(bInterval)



/* string descriptor */
typedef struct tagT_UsbStringDescriptor
{
  T_UINT8      bLength;
  T_UINT8      bDescriptorType;
  T_LE_UINT16  bString[1];       /* variable size */
} T_UsbStringDescriptor;

/* interface association descriptor */
typedef struct tagT_UsbInterfaceAssociationDescriptor 
{
  T_UINT8   bLength;
  T_UINT8   bDescriptorType;
  T_UINT8   bFirstInterface;
  T_UINT8   bInterfaceCount;
  T_UINT8   bFunctionClass;
  T_UINT8   bFunctionSubClass;
  T_UINT8   bFunctionProtocol;
  T_UINT8   iFunction;
} T_UsbInterfaceAssociationDescriptor;

#define TUSB_SIZEOF_INTERFACE_ASSOCIATION_DESC			 8
TB_CHECK_SIZEOF(T_UsbInterfaceAssociationDescriptor, 8);


/*** high speed only ***/

/* device qualifier descriptor */
typedef struct tagT_UsbDeviceQualifierDescriptor
{
  T_UINT8      bLength;
  T_UINT8      bDescriptorType;
  T_LE_UINT16  bcdUSB;
  T_UINT8      bDeviceClass;
  T_UINT8      bDeviceSubClass;
  T_UINT8      bDeviceProtocol;
  T_UINT8      bMaxPacketSize0;
  T_UINT8      bNumConfigurations;
  T_UINT8      bReserved;
} T_UsbDeviceQualifierDescriptor;

#define TUSB_SIZEOF_DEVICE_QUALIFIER_DESC				10
TB_CHECK_SIZEOF(T_UsbDeviceQualifierDescriptor, 10);
 
/* other speed configuration descriptor */
typedef struct tagT_UsbOtherSpeedConfigurationDescriptor
{
  T_UINT8      bLength;
  T_UINT8      bDescriptorType;
  T_LE_UINT16  wTotalLength;
  T_UINT8      bNumInterfaces;
  T_UINT8      bConfigurationValue;
  T_UINT8      iConfiguration;
  T_UINT8      bmAttributes;
  T_UINT8      bMaxPower;
} T_UsbOtherSpeedConfigurationDescriptor;

#define TUSB_SIZEOF_OTHERSPEED_CONFIGURATION_DESC				9
TB_CHECK_SIZEOF(T_UsbOtherSpeedConfigurationDescriptor, 9);


/* Standard TUSB_REQ_GET_STATUS request returned data */
typedef struct tagT_UsbStatusStandardRequest
{
  T_LE_UINT16  wStatus;
} T_UsbStatusStandardRequest;

#define TUSB_SIZEOF_STATUS_STANDARD_REQ		  2
TB_CHECK_SIZEOF(T_UsbStatusStandardRequest, 2);

/* wStatus fields*/
#define TUSB_STD_DEVICE_STATUS_SELF_POWERED  0x0001
#define TUSB_STD_DEVICE_STATUS_REMOTE_WAKEUP 0x0002
#define TUSB_STD_ENDPOINT_STATUS_HALT				 0x0001


// restore packing
#include "tbase_packrestore.h"

#endif // __tusb_spec_h__

/****************************** EOF ****************************************/
