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
 *  Module:         tusb_spec_30.h
 *  Description:    additional defines introduced by USB 3.0 Specification
 ******************************************************************************/

#ifndef __tusb_spec_30_h__
#define __tusb_spec_30_h__

#include "tbase_platform.h"
#include "tbase_types.h"

// pack the following structures                                                     
#include "tbase_pack1.h"


//////////////////////////////////////////////////////////////////////////
//
// Binary Device Object Store (BOS) descriptor
//
//////////////////////////////////////////////////////////////////////////
/* bcdUSB >= TUSB_VER_210 only */
typedef struct tagT_UsbBinaryDeviceObjectStoreDescriptor
{
  T_UINT8      bLength;
  T_UINT8      bDescriptorType;
  T_LE_UINT16	 wTotalLength;
  T_UINT8      bNumDeviceCaps;
} T_UsbBinaryDeviceObjectStoreDescriptor;

#define TUSB_SIZEOF_BINARY_DEVICE_OBJECT_STORE_DESC 		5
TB_CHECK_SIZEOF(T_UsbBinaryDeviceObjectStoreDescriptor, 5);


//////////////////////////////////////////////////////////////////////////
//
// Device Capability descriptor
//
//////////////////////////////////////////////////////////////////////////
/* bcdUSB >= TUSB_VER_210 only */
typedef struct tagT_UsbDeviceCapabilityDescriptor
{
  T_UINT8      bLength;
  T_UINT8      bDescriptorType;			/*TUSB_DEVICE_CAPABILITY_DESCRIPTOR*/
  T_UINT8      bDevCapabilityType;
} T_UsbDeviceCapabilityDescriptor;

#define TUSB_SIZEOF_DEVICE_CAPABILITY_DESC 			 3
TB_CHECK_SIZEOF(T_UsbDeviceCapabilityDescriptor, 3);

//////////////////////////////////////////////////////////////////////////
// Device Capabilities (bDevCapabilityType)
//////////////////////////////////////////////////////////////////////////
#define TUSB_DEVICE_CAPABILITY_TYPE_WIRELESS_USB					0x01
#define TUSB_DEVICE_CAPABILITY_TYPE_USB_20_EXTENSION			0x02
#define TUSB_DEVICE_CAPABILITY_TYPE_SUPERSPEED_USB				0x03
#define TUSB_DEVICE_CAPABILITY_TYPE_CONTAINER_ID					0x04
#define TUSB_DEVICE_CAPABILITY_TYPE_PLATFORM							0x05
#define TUSB_DEVICE_CAPABILITY_TYPE_POWER_DELIVERY_CAP		0x06
#define TUSB_DEVICE_CAPABILITY_TYPE_BATTERY_INFO_CAP			0x07
#define TUSB_DEVICE_CAPABILITY_TYPE_PD_CONSUMER_PORT_CAP  0x08
#define TUSB_DEVICE_CAPABILITY_TYPE_PD_PROVIDER_PORT_CAP  0x09
#define TUSB_DEVICE_CAPABILITY_TYPE_SUPERSPEED_PLUS				0x0A
#define TUSB_DEVICE_CAPABILITY_TYPE_PRECISION_TIME_MEAS   0x0B
#define TUSB_DEVICE_CAPABILITY_TYPE_WIRELESS_USB_EXT      0x0C


//////////////////////////////////////////////////////////////////////////
//
// USB 2.0 Extension descriptor
//
//////////////////////////////////////////////////////////////////////////
typedef struct tagT_UsbDeviceCapabilityUsb20ExtensionDescriptor
{
  T_UINT8      bLength;
  T_UINT8      bDescriptorType;			/*TUSB_DEVICE_CAPABILITY_DESCRIPTOR*/
  T_UINT8      bDevCapabilityType;	/*TUSB_DEVICE_CAPABILITY_TYPE_USB_20_EXTENSION*/
  T_LE_UINT32  bmAttributes;	/* unaligned */
} T_UsbDeviceCapabilityUsb20ExtensionDescriptor;

#define TUSB_SIZEOF_DEVICE_CAPABILITY_USB_20_EXTENSION_DESC 	 7
TB_CHECK_SIZEOF(T_UsbDeviceCapabilityUsb20ExtensionDescriptor, 7);

#define TUSB_DEVICE_LPM_SUPPORTED             0x00000002


//////////////////////////////////////////////////////////////////////////
//
// SuperSpeed Device Capability descriptor
//
//////////////////////////////////////////////////////////////////////////
typedef struct tagT_UsbDeviceCapabilitySuperSpeedDescriptor
{
  T_UINT8      bLength;
  T_UINT8      bDescriptorType;			/*TUSB_DEVICE_CAPABILITY_DESCRIPTOR*/
  T_UINT8      bDevCapabilityType;	/*TUSB_DEVICE_CAPABILITY_TYPE_SUPERSPEED_USB*/
  T_UINT8      bmAttributes;
  T_LE_UINT16  wSpeedsSupported;
  T_UINT8      bFunctionalitySupport;
  T_UINT8      bU1DevExitLat;
  T_LE_UINT16  wU2DevExitLat;
} T_UsbDeviceCapabilitySuperSpeedDescriptor;

#define TUSB_SIZEOF_DEVICE_CAPABILITY_SUPERSPEED_DESC 		 10
TB_CHECK_SIZEOF(T_UsbDeviceCapabilitySuperSpeedDescriptor, 10);

//////////////////////////////////////////////////////////////////////////
// Supported device speeds when operating in super speed (wSpeedsSupported)
//////////////////////////////////////////////////////////////////////////
#define TUSB_DEVICE_SPEEDS_SUPPORTED_LOW_SPEED       0x01
#define TUSB_DEVICE_SPEEDS_SUPPORTED_FULL_SPEED      0x02
#define TUSB_DEVICE_SPEEDS_SUPPORTED_HIGH_SPEED      0x04
#define TUSB_DEVICE_SPEEDS_SUPPORTED_SUPER_SPEED     0x08

#define TUSB_DEVICE_LTM_SUPPORTED                    0x02


//////////////////////////////////////////////////////////////////////////
//
// Container ID descriptor
//
//////////////////////////////////////////////////////////////////////////
typedef struct tagT_UsbDeviceCapabilityContainerIdDescriptor
{
  T_UINT8      bLength;
  T_UINT8      bDescriptorType;			/*TUSB_DEVICE_CAPABILITY_DESCRIPTOR*/
  T_UINT8      bDevCapabilityType;	/*TUSB_DEVICE_CAPABILITY_TYPE_CONTAINER_ID*/
  T_UINT8      bReserved;
  T_UINT8      ContainerID[16];
} T_UsbDeviceCapabilityContainerIdDescriptor;

#define TUSB_SIZEOF_DEVICE_CAPABILITY_CONTAINER_ID_DESC 	  20
TB_CHECK_SIZEOF(T_UsbDeviceCapabilityContainerIdDescriptor, 20);



//////////////////////////////////////////////////////////////////////////
//
// SuperSpeed Endpoint Companion descriptor
//
//////////////////////////////////////////////////////////////////////////
typedef struct tagT_UsbSuperSpeedEndpointCompanionDescriptor
{
  T_UINT8       bLength;
  T_UINT8       bDescriptorType;			/*TUSB_SUPERSPEED_USB_ENDPOINT_COMPANION_DESCRIPTOR*/
  T_UINT8       bMaxBurst;
  T_UINT8       bmAttributes;
  T_LE_UINT16   wBytesPerInterval;
} T_UsbSuperSpeedEndpointCompanionDescriptor;

#define TUSB_SIZEOF_SUPER_SPEED_ENDPOINT_COMPANION_DESC			6
TB_CHECK_SIZEOF(T_UsbSuperSpeedEndpointCompanionDescriptor, 6);


//////////////////////////////////////////////////////////////////////////
// USB Endpoint MaxBurst (bMaxBurst = 0..15)
//////////////////////////////////////////////////////////////////////////
#define TUSB_EP_MAX_BURST(bMaxBurst)	( ((bMaxBurst) & 0xF) + 1 )


//////////////////////////////////////////////////////////////////////////
// USB Bulk Endpoint MaxStreams (Bit 4..0 of bmAttributes)
//////////////////////////////////////////////////////////////////////////
#define TUSB_BULK_EP_MAX_STREAMS_FROM_ATTR(bmAttributes)    ( 0x1U << ((bmAttributes) & 0x1f) )


//////////////////////////////////////////////////////////////////////////
// USB ISO Endpoint Mult (Bit 1..0 of bmAttributes)
//////////////////////////////////////////////////////////////////////////
#define TUSB_ISO_EP_MULT_FROM_ATTR(bmAttributes)	( ((bmAttributes) & 0x3) + 1 )
#define TUSB_ISO_EP_ATTR_FROM_MULT(Mult)					( ((Mult)-1) & 0x3 )

//////////////////////////////////////////////////////////////////////////
// USB ISO Endpoint SSP ISO Companion (Bit 7 of bmAttributes)
//////////////////////////////////////////////////////////////////////////
#define TUSB_ISO_EP_SSP_ISO_COMPANION_AVAILABLE(bmAttributes)	( 0 != ((bmAttributes) & 0x80) )


// restore packing
#include "tbase_packrestore.h"

#endif // __tusb_spec_30_h__

/****************************** EOF ****************************************/
