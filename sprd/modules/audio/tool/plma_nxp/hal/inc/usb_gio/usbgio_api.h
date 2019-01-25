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
 *  Module:       usbgio_api.h
 *  Description:  
 *    PC-side application programming interface for generic I/O via USB/HID
 ************************************************************************/

#ifndef __usbgio_api_h__
#define __usbgio_api_h__

// Tell Doxygen that we want to document this file.
/*! \file */


/*!
  \mainpage

  \section sec_overview Overview

  This documentation describes the programming interface of the USBGIO (generic I/O) device which
	supports communication through I2C and GPIO lines.
  Device communication is based on USB and HID (Human Interface Device class) and uses
  the HID class driver included in Windows.
  The DLL and its programming interface support communication with multiple device instances in parallel.
  
  The programming interface consists of C-style functions which are exported by the DLL \c usbgio_api.dll.
  The DLL implementation is based on Windows HID driver functions and standard Win32 functions.
  
  The \c usbgio_api.dll DLL is available in two variants: 32 bit and 64 bit.
  The function interface exported by the DLL is identical in both variants.
  The 32 bit DLL is to be used by a 32 bit application.
  A 32 bit application can run on both variants of Windows systems: 32 bit (x86) and 64 bit (x64).
  The 64 bit DLL is to be used by an application that has been compiled as a 64 bit executable.
  Such an application can run on x64 variants of Windows only.


  \section sec_prog_if Programming Interface
  
  The programming interface of the DLL is described in the following sections:
  - \ref api_functions
  - \ref cb_functions
  - \ref constants_types
  - \ref api_version_constants

  The DLL programming interface is declared in usbgio_api.h.


  \section sec_compatibility API Compatibility Check
  
  Before an application calls any other API function, it should use USBGIO_CheckApiVersion() to verify
  that the loaded DLL binary is compatible with the usbgio_api.h file the application has been compiled with. 
  See USBGIO_CheckApiVersion() and \ref api_version_constants for details.


  \section sec_enum_dev Enumerating Devices Attached to the System

  Typically, at startup an application needs to determine how many USB devices are 
  currently connected to the system and to build an internal device list.
  This is called device enumeration.
  Before the enumeration is performed, the application should register a device change notification 
  callback with USBGIO_RegisterDeviceChangeCallback().
  The DLL fires this callback if the device list changes afterwards.
  
  The following pseudo code shows how an application should implement initial device enumeration.
  This is not a complete C++ source code example. 
  Error handling and other details are not shown.

  \code

  // this is our list that holds active devices (open handles)
  MyDeviceContainer deviceContainer;

  // register callback with the DLL (optional)
  USBGIO_RegisterDeviceChangeCallback(MyDeviceChangeCallback, NULL);

  // enumerate attached devices
  unsigned int deviceCount = 0;
  USBGIO_EnumerateDevices(&deviceCount);

  // build device list
  for ( unsigned int i=0; i<deviceCount; i++ ) {
    T_UNICHAR idStr[512];
    st = USBGIO_GetDeviceInstanceId(i, idStr, sizeof(idStr));
    if ( TSTATUS_SUCCESS == st ) {
      USBGIODeviceHandle h;
      st = USBGIO_OpenDevice(idStr, &h);
      if ( TSTATUS_SUCCESS == st ) {
        // device successfully opened, add a new object to our list
        MyDeviceObj* devobj = new MyDeviceObj(idStr, h);
        deviceContainer.Add(devobj);
      }
    }
  }

  \endcode


  \section dyn_add_remove Dynamic Add/Remove Device Handling
  
  Note that handling of dynamic device removal/insertion is not necessary for all types of applications.
  For example, a command line based utility will typically implement initial device enumeration only
  and will ignore subsequent changes of the device list.
  Whether the dynamic add/remove device handling described in this section is required or not, 
  is a decision to be made by the application designer.

  When a device is removed or attached while the application is running, the DLL calls
  the application's device change callback.
  In the device change callback (or in another thread which gets triggered by the callback) 
  the application needs to update its internal device list to reflect the current state of the system.
  Note that more than one device can be removed (or attached) at once, e.g. if a USB hub is disconnected or connected.

  The following pseudo code shows how an application should implement re-enumeration in its device change callback.
  This is not a complete C++ source code example. 
  Error handling and other details are not shown.
  
  \code

  // this is our list that holds active devices (open handles)
  MyDeviceContainer deviceContainer;

  void MyDeviceChangeCallback(void* callbackContext)
  {
    //
    // first step: remove from our list all devices that are disconnected now
    //
    foreach ( MyDeviceObj* devobj in deviceContainer ) {
      if ( TSTATUS_SUCCESS != USBGIO_CheckDeviceConnection(devobj->Handle) ) {
        // device has been disconnected, remove it from our container
        deviceContainer.Remove(devobj);
        USBGIO_CloseDevice(devobj->Handle);
        delete devobj;
      }
    } 

    //
    // next: add new devices to our list, if not already present
    //
    unsigned int deviceCount = 0;
    USBGIO_EnumerateDevices(&deviceCount);
    for ( unsigned int i=0; i<deviceCount; i++ ) {
      T_UNICHAR idStr[512];
      st = USBGIO_GetDeviceInstanceId(i, idStr, sizeof(idstr));
      if ( TSTATUS_SUCCESS == st ) {
        if ( deviceContainer.IsDeviceIdPresent(idStr) ) {
          // we opened this device already
        } else {
          // new device, add to our list
          USBGIODeviceHandle h;
          st = USBGIO_OpenDevice(idStr, &h);
          if ( TSTATUS_SUCCESS == st ) {
            MyDeviceObj* devobj = new MyDeviceObj(idStr, h);
            deviceContainer.Add(devobj);
          }
        }
      }
    }
  }

  \endcode


  \section sec_exclusive_access Exclusive Device Access
  
  Through this DLL communication with a given USB device instance can be controlled by one application at a time only.
  In the USBGIO_OpenDevice() call the DLL blocks concurrent access to a device instance by multiple
  applications, or multiple instances of the same application.
  In other words, any given USB device instance is always opened exclusively.
  
  Note that if there are multiple USB device instances attached to the system then these instances can be 
  opened and used concurrently through the DLL API. 


  \section sec_threading Multi-threading
  
  The DLL and its API is thread-safe.
  API functions can be called by multiple threads in parallel.


  \section sec_unichar UNICODE Characters

	API functions that expect string arguments or return strings use the character type \c T_UNICHAR.
	This type varies depending on the platform this API is running on.
	On Windows a \c T_UNICHAR is 2 bytes in size and corresponds to \c WCHAR or \c wchar_t.
	A UNICODE character is represented by a 16-bit wide character.
	On Linux a \c T_UNICHAR is 1 byte in size and corresponds to \c char.
	A UNICODE character is a UTF-8 encoded multi-byte sequence.

	If this API is used from scripting languages care must be taken to use the correct
	character size when calling API functions. Where required, strings must be converted to meet 
	the native character representation of the scripting language.
	See also USBGIO_GetUniCharSize().

*/


// basic types
#include "tbase_platform.h"
#include "tbase_types.h"
// status codes
#include "tstatus_codes.h"


/*!
  \defgroup api_version_constants Interface version constants
  @{

  \brief Version number of the programming interface exported by the DLL
  
  If changes are made to the interface that are compatible with previous versions
  of the interface then the minor version number will be incremented.
  If changes are made to the interface that cause an incompatibility with previous versions
  of the interface then the major version number will be incremented.
  
  An application should use USBGIO_CheckApiVersion() to check if it can work with the present version of this DLL.
  It can use USBGIO_GetApiVersion() to retrieve the API version implemented by the DLL.
*/

//! Major version number of the programming interface.
#define USBGIO_API_VERSION_MJ   1
//! Minor version number of the programming interface.
#define USBGIO_API_VERSION_MN   0

//! Version number of the programming interface as DWORD.
#define USBGIO_API_VERSION  \
  ( ((unsigned int)USBGIO_API_VERSION_MN)|(((unsigned int)USBGIO_API_VERSION_MJ)<<16) )

/*!
  @}
*/




/*!
  \defgroup constants_types Constants and types
  @{

  \brief Constants and types used in the programming interface.
*/

//! Handle type used by the DLL. The value #USBGIO_INVALID_HANDLE (zero) is an invalid handle.
typedef unsigned int USBGIODeviceHandle;

//! The value zero is an invalid handle.
#define USBGIO_INVALID_HANDLE        0


/*!
  \brief Device event types, see USBGIOCB_DeviceChangeEvent().
*/
typedef enum tagUSBGIODeviceEvent
{
  USBGIOEvent_reserved = 0,    /*!< Reserved for internal use. */

  USBGIOEvent_DeviceAttached,  /*!< A USB device has been attached to the system. */
  USBGIOEvent_DeviceRemoved    /*!< A USB device has been removed from the system. */

} USBGIODeviceEvent;


/*!
  \brief GPIO pin modes.
*/
typedef enum tagUSBGIO_GPIOMode
{
  GPIOMode_Input = 0,			/*!< Input pin. */
  GPIOMode_InputEvent,		/*!< Input pin, initiates event on state change. */
  GPIOMode_Output_Low,		/*!< Output pin. Initial state is LOW. */
  GPIOMode_Output_High		/*!< Output pin. Initial state is HIGH. */

} USBGIO_GPIOMode;


/*!
  \brief GPIO pin states.
*/
typedef enum tagUSBGIO_GPIOState
{
  GPIOState_Low = 0,			/*!< LOW */
  GPIOState_High = 1			/*!< HIGH */

} USBGIO_GPIOState;

/*!
  @}
*/


#if defined(_WIN32)
// Windows

// function decoration
#if defined(USBGIO_API_DLL_EXPORTS)
// if .h file is included by the DLL implementation:

// Use this if a .def file is used to specify the exports.
#define USBGIO_DECL

#else
// if .h file is included (imported) by client code
#define USBGIO_DECL  __declspec(dllimport)
#endif  

// calling convention
#define USBGIO_CALL  __stdcall

#else
// Linux

#define USBGIO_DECL
#define USBGIO_CALL

#endif // defined(_WIN32)



#if defined (__cplusplus)
extern "C" {
#endif

/*!
  \defgroup cb_functions Callback functions 
  @{

  \brief Functions provided by the application and called by the DLL.
*/

/*!
  \brief Callback function which will be called when a new device is attached to the system, or removed from the system.
  
  \param[in] callbackContext
    The pointer value that was passed to USBGIO_RegisterDeviceChangeCallback() when the callback was registered.

  \param[in] eventType
    This value indicates the type of event.

  When receiving a \ref USBGIOEvent_DeviceRemoved event the application should call 
  USBGIO_CheckDeviceConnection() for each device it has currently opened and close, via USBGIO_CloseDevice(),
  any device instance which is not present in the system any longer.
  
  When receiving a \ref USBGIOEvent_DeviceAttached event an application should call USBGIO_EnumerateDevices()
  to determine the list of devices currently connected to the system.
  Then it uses USBGIO_GetDeviceInstanceId() and USBGIO_OpenDevice() to open each (new) device instance 
  and to establish communication.
  
  See also \ref dyn_add_remove.

  The callback function executes in the context of an internal worker thread.
  So the callback is asynchronous with respect to any other thread of the application.

  The callback function should return as quickly as possible and should not block internally waiting
  for external events to occur.
  If the function blocks internally then this prevents subsequent notifications from being delivered.
*/
typedef
void
USBGIO_CALL
USBGIOCB_DeviceChangeEvent(
  void* callbackContext,
  USBGIODeviceEvent eventType
  );


/*!
  \brief Callback function which will be called when the state of a \ref GPIOMode_InputEvent pin changes.

  \param[in] callbackContext
    The pointer value that was passed to USBGIO_RegisterPinEventCallback() when the callback was registered.

  \param[in] pinId
    Identifies the GPIO pin.
    The number of available GPIO pins and their capabilities are application defined.
    The valid pin IDs are defined by a given firmware implementation.

  \param[in] state
		Indicates the new state of the GPIO pin.

	Input pin event callbacks will be fired for pins only which are set to \ref GPIOMode_InputEvent, see USBGIO_SetPinMode().

  The callback function executes in the context of an internal worker thread.
  So the callback is asynchronous with respect to any other thread of the application.

  The callback function should return as quickly as possible and should not block internally waiting
  for external events to occur.
  If the function blocks internally then this prevents subsequent events from being delivered.
*/
typedef
void
USBGIO_CALL
USBGIOCB_InputPinEvent(
  void* callbackContext,
  unsigned int pinId,
  USBGIO_GPIOState state
  );


/*!
  @}
*/



/*!
  \defgroup api_functions Interface functions 
  @{

  \brief Functions exported by the DLL and called by the application.
*/


/*!
  \brief Returns the API version implemented by the DLL.
  
  \return
    This function returns the version number of the programming interface (API) implemented by the DLL. 
    The number returned is the #USBGIO_API_VERSION constant the DLL has been compiled with.
*/
USBGIO_DECL
unsigned int
USBGIO_CALL
USBGIO_GetApiVersion(void);


/*!
  \brief Check if the given API version number is compatible with this DLL.
  
  \param[in] majorVersion
    The major version number the application is expecting.
    An application should set this parameter to the #USBGIO_API_VERSION_MJ constant.

  \param[in] minorVersion
    The minor version number the application is expecting.
    An application should set this parameter to the #USBGIO_API_VERSION_MN constant.
  
  \return
    The function returns 1 if the specified version constants are compatible with this DLL.
    The function returns 0 if the specified version constants are not compatible with this DLL.

  An application should call this function before it uses any other function exported by the DLL
  in order to verify its compatibility with the DLL at runtime.
  If this function fails then the function interface exported by the DLL is not compatible
  with the .h file the application has been compiled with and the application should 
  reject working with this DLL version.
  
  The DLL interface is considered compatible if its major version number is equal to \p majorVersion 
  and its minor version number is greater than or equal to \p minorVersion.
*/
USBGIO_DECL
int
USBGIO_CALL
USBGIO_CheckApiVersion(
  unsigned int majorVersion,
  unsigned int minorVersion
  );


/*!
  \brief Returns the size, in bytes, of the \c T_UNICHAR type.
  
  \return
    The function returns the size, in bytes, of the \c T_UNICHAR type.
	
	The \c T_UNICHAR type is used by API functions that expect string arguments or return strings.
	The size of the \c T_UNICHAR type varies depending on the platform.

	On Windows a \c T_UNICHAR character is 2 bytes in size, i.e. it corresponds to \c WCHAR or \c wchar_t.
	UNICODE characters are represented by 16-bit characters.

	On GNU/Linux a \c T_UNICHAR character is 1 byte in size, i.e. it corresponds to \c char.
	UNICODE characters are encoded as UTF-8.

	Use this function to implement API wrapper functions in platform-independent code 
	such as	Python scripts.
*/
USBGIO_DECL
unsigned int
USBGIO_CALL
USBGIO_GetUniCharSize(void);



/*!
  \brief Register a callback to receive device change notifications.
  
  \param[in] callback
    Address of a caller-provided callback function.
    Set \p callback to NULL to unregister an existing callback.

  \param[in] callbackContext
    Caller-provided pointer which will be passed to the callback function unmodified.
    Can be set to NULL if unused.

  \return
    The function returns \c TSTATUS_SUCCESS if successful, an error code otherwise.

  The function registers an application-provided callback function which will be called
  when a new USB device instance is attached to the system, or removed from the system.
  The function will consider USBGIO devices only.
  Any other USB HID device type such as keyboard or mouse will be ignored.

  The DLL supports registration of a single callback only. 
  A new call to USBGIO_RegisterDeviceChangeCallback() will overwrite a previously registered callback, if any.
  A call to USBGIO_RegisterDeviceChangeCallback() with \p callback set to NULL will unregister any existing callback.

  An application that registered a callback with this function should unregister before
  the application quits or unloads this DLL. 
*/
USBGIO_DECL
TSTATUS
USBGIO_CALL
USBGIO_RegisterDeviceChangeCallback(
  USBGIOCB_DeviceChangeEvent* callback,
  void* callbackContext
  );



/*!
  \brief Enumerate USB devices connected to the system.
  
  \param[out] deviceCount
    Address of a caller-provided variable which will be set to the number of devices
    currently connected to the system.
    This corresponds to the number of items available in the internal device list.
    The list is addressed by means of a zero-based index which ranges from zero to \p deviceCount - 1.
    If no devices are connected, the function sets \p deviceCount to zero and succeeds.
    So in addition to the return value the caller also needs to check \p deviceCount.

  \return
    The function returns \c TSTATUS_SUCCESS if successful, an error code otherwise.

  A call to this function creates an internal list which contains all USBGIO device instances 
  currently attached to the system.
  The application needs to iterate through this list and call USBGIO_GetDeviceInstanceId() for each instance.
  The ID string returned by USBGIO_GetDeviceInstanceId() is then passed to USBGIO_OpenDevice() in order to
  open the device.

  See also \ref sec_enum_dev.

  The function will consider USBGIO devices only.
  Any other USB HID device type such as keyboard or mouse will be ignored.
*/
USBGIO_DECL
TSTATUS
USBGIO_CALL
USBGIO_EnumerateDevices(
  unsigned int* deviceCount
  );


/*!
  \brief Get the ID string for a device instance.
  
  \param[in] deviceIndex
    Specifies the device instance as a zero-based index.
    The index ranges from zero to \p deviceCount - 1.
    The \p deviceCount is returned by USBGIO_EnumerateDevices().

  \param[out] instanceIdString
    Address of a caller-provided buffer which will receive a null-terminated unicode string.
    The function guarantees that the returned string is terminated by a unicode null character.
    This string unambiguously identifies the respective USB device instance.
    The string needs to be passed to USBGIO_OpenDevice() in order to open the device instance.

  \param[in] stringBufferSize
    Specifies the size of the buffer pointed to by \p instanceIdString in bytes.
    The caller should provide space for at least 512 unicode characters.

  \return
    The function returns \c TSTATUS_SUCCESS if successful, an error code otherwise.
    Some specific error codes are described below.

  \retval TSTATUS_ENUM_REQUIRED
    USBGIO_EnumerateDevices() was not called before.
  
  \retval TSTATUS_BUFFER_TOO_SMALL
    Indicates that the provided string buffer (as defined by \p stringBufferSize)
    is not large enough to receive the resulting string.

  The function returns a string that unambiguously identifies a device instance connected to the system.
  The returned string is required to open this device instance and to start communication.

  An application needs to call USBGIO_EnumerateDevices() before this function can be used.
*/
USBGIO_DECL
TSTATUS
USBGIO_CALL
USBGIO_GetDeviceInstanceId(
  unsigned int deviceIndex,
  T_UNICHAR* instanceIdString,
  unsigned int stringBufferSize
  );


/*!
  \brief Open a device.
  
  \param[in] instanceIdString
    Points to a null-terminated unicode string which unambiguously identifies the device instance to be opened.
    This string is returned by USBGIO_GetDeviceInstanceId().

  \param[out] deviceHandle
    Address of a caller-provided variable which will be set to a handle value if the function succeeds.
    The handle represents the connection to the device instance and is required for all subsequent API calls.
    Note that the value #USBGIO_INVALID_HANDLE (zero) is an invalid handle.
    If the function succeeds it will return a valid non-zero handle.

  \return
    The function returns \c TSTATUS_SUCCESS if successful, an error code otherwise.
    Some specific error codes are described below.

  \retval TSTATUS_IN_USE
    Indicates that a handle for the specified device instance is already opened.
    The device is already in use by this application, or is currently in use by another application.

  \retval TSTATUS_OBJECT_NOT_FOUND
    The device instance identified by \p instanceIdString is not present in the system.
    The device has been disconnected after USBGIO_EnumerateDevices() was called.
    The application should call USBGIO_EnumerateDevices() again.

  The function opens the specified device instance.
  If the specified device instance is already opened, the function fails.
  One device instance cannot be opened by multiple applications simultaneously, see also \ref sec_exclusive_access.
  
  If more than one device is currently present in the system then client code can call this 
  function several times to open all available device instances, or a subset of them.
*/
USBGIO_DECL
TSTATUS
USBGIO_CALL
USBGIO_OpenDevice(
  const T_UNICHAR* instanceIdString,
  USBGIODeviceHandle* deviceHandle
  );


/*!
  \brief Close a device.
  
  \param[in] deviceHandle
    Identifies the device instance.
    The specified handle becomes invalid after this call and must not be used in any subsequent API calls.

  \return
    The function returns \c TSTATUS_SUCCESS if successful, an error code otherwise.
    If a valid handle is specified, the function returns always \c TSTATUS_SUCCESS.

  The specified device instance will be closed and the provided handle becomes invalid after this call.
*/
USBGIO_DECL
TSTATUS
USBGIO_CALL
USBGIO_CloseDevice(
  USBGIODeviceHandle deviceHandle
  );


/*!
  \brief Check if the given device instance is still connected to the system.
  
  \param[in] deviceHandle
    Identifies the device instance.

  \retval TSTATUS_SUCCESS
    The device is still connected to the system.

  \retval TSTATUS_DEVICE_REMOVED
    The device has been disconnected.
    No further communication is possible with this device instance.
    The application should close the handle via USBGIO_CloseDevice().

  The function checks if a given device is present in the system (USB cable attached)
  or not present (USB cable detached).
*/
USBGIO_DECL
TSTATUS
USBGIO_CALL
USBGIO_CheckDeviceConnection(
  USBGIODeviceHandle deviceHandle
  );


/*!
  \brief Query USB identifiers.
 
  \param[in] deviceHandle
    Identifies the device instance.

  \param[out] usbVendorId
    Address of a caller-provided variable which will be set to the USB vendor ID.
    If the caller is not interested in the information, this parameter can be set to NULL.

  \param[out] usbProductId
    Address of a caller-provided variable which will be set to the USB product ID.
    If the caller is not interested in the information, this parameter can be set to NULL.

  \param[out] bcdDevice
    Address of a caller-provided variable which will be set to the revision code reported in the USB device descriptor.
    If the caller is not interested in the information, this parameter can be set to NULL.

  \return
    The function returns \c TSTATUS_SUCCESS if successful, an error code otherwise.

  The function returns some USB-specific identifiers which are informational only.
*/
USBGIO_DECL
TSTATUS
USBGIO_CALL
USBGIO_GetDeviceUsbInfo(
  USBGIODeviceHandle deviceHandle,
  unsigned int* usbVendorId,
  unsigned int* usbProductId,
  unsigned int* bcdDevice
  );


/*!
  \brief Query the device's USB serial number.

  \param[in] deviceHandle
    Identifies the device instance.

  \param[out] serialNumber
    Address of a caller-provided buffer which will receive a null-terminated unicode string.
    The function guarantees that the returned string is terminated by a unicode null character.
    This string represents the serial number reported by the USB device via string descriptor.

  \param[in] stringBufferSize
    Specifies the size of the buffer pointed to by \p serialNumber in bytes.
    The caller should provide space for at least 128 unicode characters.

  \return
    The function returns \c TSTATUS_SUCCESS if successful, an error code otherwise.
    Some specific error codes are described below.

  \retval TSTATUS_NOT_SUPPORTED
    The USB device does not support a USB serial number string descriptor.
  
  \retval TSTATUS_BUFFER_TOO_SMALL
    Indicates that the provided string buffer (as defined by \p stringBufferSize)
    is not large enough to receive the resulting string.
  
  The implementation of a serial number string descriptor in a USB device is optional.
*/
USBGIO_DECL
TSTATUS
USBGIO_CALL
USBGIO_GetDeviceSerialNumber(
  USBGIODeviceHandle deviceHandle,
  T_UNICHAR* serialNumber,
  unsigned int stringBufferSize
  );



/*!
  \brief Configure an GPIO pin.

  \param[in] deviceHandle
    Identifies the device instance.

  \param[in] pinId
    Identifies the GPIO pin.
    The number of available GPIO pins and their capabilities are application defined.
    The valid pin IDs are defined by a given firmware implementation.
   
  \param[in] mode
		Specifies the mode of the GPIO pin.

  \return
    The function returns \c TSTATUS_SUCCESS if successful, an error code otherwise.

	All available GPIO pins are configured as input (\ref GPIOMode_Input) by default.
*/
USBGIO_DECL
TSTATUS
USBGIO_CALL
USBGIO_SetPinMode(
  USBGIODeviceHandle deviceHandle,
  unsigned int pinId,
  USBGIO_GPIOMode mode
  );


/*!
  \brief Set the state of an output GPIO pin.

  \param[in] deviceHandle
    Identifies the device instance.

  \param[in] pinId
    Identifies the GPIO pin.
    The number of available GPIO pins and their capabilities are application defined.
    The valid pin IDs are defined by a given firmware implementation.
   
  \param[in] state
		Specifies the new state of the GPIO pin.

  \return
    The function returns \c TSTATUS_SUCCESS if successful, an error code otherwise.

	This call has an effect on the specified pin only if it has been configured for output mode, see USBGIO_SetPinMode().
	It has no effect on input pins.
*/
USBGIO_DECL
TSTATUS
USBGIO_CALL
USBGIO_SetPinState(
  USBGIODeviceHandle deviceHandle,
  unsigned int pinId,
  USBGIO_GPIOState state
  );


/*!
  \brief Read the current state of a GPIO pin.

  \param[in] deviceHandle
    Identifies the device instance.

  \param[in] pinId
    Identifies the GPIO pin.
    The number of available GPIO pins and their capabilities are application defined.
    The valid pin IDs are defined by a given firmware implementation.
   
  \param[out] state
		Points to a caller-provided variable which will be set to the current state of the 
		specified GPIO pin if the function succeeds.

  \return
    The function returns \c TSTATUS_SUCCESS if successful, an error code otherwise.

	This call returns the current state of any supported pin regardless whether the pin
	is in output mode or input mode, see USBGIO_SetPinMode().
*/
USBGIO_DECL
TSTATUS
USBGIO_CALL
USBGIO_GetPinState(
  USBGIODeviceHandle deviceHandle,
  unsigned int pinId,
  USBGIO_GPIOState* state
  );


/*!
  \brief Read the current mode of a GPIO pin.

  \param[in] deviceHandle
    Identifies the device instance.

  \param[in] pinId
    Identifies the GPIO pin.
    The number of available GPIO pins and their capabilities are application defined.
    The valid pin IDs are defined by a given firmware implementation.
  
  \param[out] mode
    Points to a caller-provided variable which will be set to the current mode of the 
    specified GPIO pin if the function succeeds.

  \return
    The function returns \c TSTATUS_SUCCESS if successful, an error code otherwise.
*/
USBGIO_DECL
TSTATUS
USBGIO_CALL
USBGIO_GetPinMode(
  USBGIODeviceHandle deviceHandle,
  unsigned int pinId,
  USBGIO_GPIOMode* mode
  );


/*!
  \brief Register a callback to receive input pin events.
  
  \param[in] deviceHandle
    Identifies the device instance.
  
  \param[in] callback
    Address of a caller-provided callback function.
    Set \p callback to NULL to unregister an existing callback.

  \param[in] callbackContext
    Caller-provided pointer which will be passed to the callback function unmodified.
    Can be set to NULL if unused.

  \return
    The function returns \c TSTATUS_SUCCESS if successful, an error code otherwise.

  The function registers an application-provided callback function which will be called
  if the state of an input pin that has been set to \ref GPIOMode_InputEvent mode changes.

  The DLL supports registration of a single callback only. 
  A new call to USBGIO_RegisterPinEventCallback() will overwrite a previously registered callback, if any.
  A call to USBGIO_RegisterPinEventCallback() with \p callback set to NULL will unregister any existing callback.
  
  An application that registered a callback with this function should unregister before
  the application quits or unloads this DLL. 
*/
USBGIO_DECL
TSTATUS
USBGIO_CALL
USBGIO_RegisterPinEventCallback(
  USBGIODeviceHandle deviceHandle,
  USBGIOCB_InputPinEvent* callback,
  void* callbackContext
  );



/*!
  \brief Execute an I2C transaction.

  \param[in] deviceHandle
    Identifies the device instance.

  \param[in] i2cBus
		Identifies the I2C bus.
    The number of available I2C buses and their capabilities are application defined.

  \param[in] i2cSpeed
		Specifies the I2C bus speed, in units of 1/s, for this transaction.

  \param[in] i2cSlaveAddress
		Specifies the I2C slave address.
		The address is given as a 7-bit value.
		The address is contained in bits 6 to 0 and does not include the R/W bit.
		The firmware adds the R/W bit internally as needed.

  \param[in] writeData
		Points to a buffer that contains the data bytes to be transferred to the slave device 
		during the write phase of the transaction.
		This parameter can be set to NULL if \p bytesToWrite is set to zero.

  \param[in] bytesToWrite
		Specifies the number of bytes to be transferred during the transaction's write phase.
		Set to zero if there is no write phase to be executed.

  \param[out] readBuffer
		Points to a caller-provided buffer which receives the data bytes received from the slave 
		device during the read phase of the transaction.
		This parameter can be set to NULL if \p bytesToRead is set to zero.

  \param[in] bytesToRead
		Specifies the number of bytes to be transferred during the transaction's read phase.
		Set to zero if there is no read phase to be executed.
	
  \param[in] timeoutInterval
		Specifies a timeout interval, in terms of milliseconds, for the entire transaction.
    If the transaction does not complete within the specified interval
    then the function will abort and return the error status \c TSTATUS_TIMEOUT.
    The specified timeout interval can not be 0 and must not exceed the max. timeout of 30 seconds.

  \return
    The function returns \c TSTATUS_SUCCESS if successful, an error code otherwise.
    Some specific error codes are described below.

  \retval TSTATUS_TIMEOUT
    Indicates that the specified timeout interval expired before the transaction completed.


	The function executes a write phase, a read phase, or a combination of both.
	The supported transaction types are described below:
	
	  - <b>Write only</b> (bytesToWrite>0 && bytesToRead==0) \n
	  The I2C transaction sequence is: START - WRITE - STOP

	  - <b>Read only</b> (bytesToWrite==0 && bytesToRead>0) \n
	  The I2C transaction sequence is: START - READ - STOP

	  - <b>Write + Read</b> (bytesToWrite>0 && bytesToRead>0) \n
	  The I2C transaction sequence is: START - WRITE - START - READ - STOP

*/
USBGIO_DECL
TSTATUS
USBGIO_CALL
USBGIO_I2cTransaction(
  USBGIODeviceHandle deviceHandle,
  unsigned int i2cBus,
  unsigned int i2cSpeed,
  unsigned int i2cSlaveAddress,
  const unsigned char* writeData,
  unsigned int bytesToWrite,
  unsigned char* readBuffer,
  unsigned int bytesToRead,
  unsigned int timeoutInterval
  );



/*!
  @}
*/

#if defined (__cplusplus)
}
#endif


#endif  // __usbgio_api_h__

/*************************** EOF **************************************/
