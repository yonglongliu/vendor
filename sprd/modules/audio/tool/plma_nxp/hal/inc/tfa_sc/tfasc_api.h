/*******************************************************************************
 *
 *  Module:         tfasc_api.h 
 *  Description:    
 *		application programming interface exported by the TFA side channel DLL
 *
 *  Author(s):      Reinhard Huck
 *  Company:        Thesycon GmbH, Ilmenau
 ******************************************************************************/

#ifndef __tfasc_api_h__
#define __tfasc_api_h__

// Tell doxygen that we want to document this file.
/*! \file */


// basic types
#include "tbase_platform.h"
#include "tbase_types.h"
// status codes
#include "tstatus_codes.h"

// defs
#include "TfascApi_defs.h"


/*!
  \defgroup api_version_constants Interface version constants
  @{

  \brief Version number of the programming interface exported by the DLL
  
  If changes are made to the interface that are compatible with previous versions
  of the interface then the minor version number will be incremented.
  If changes are made to the interface that cause an incompatibility with previous versions
  of the interface then the major version number will be incremented.
  
  An application should use DLLTEMPL_CheckApiVersion() to check if it can work with the present version of this DLL.
  It can use DLLTEMPL_GetApiVersion() to retrieve the API version implemented by the DLL.
*/

//! Major version number of the programming interface.
#define TFASC_API_VERSION_MJ   1
//! Minor version number of the programming interface.
#define TFASC_API_VERSION_MN   0

//! Version number of the programming interface as DWORD.
#define TFASC_API_VERSION  \
  ( ((unsigned int)TFASC_API_VERSION_MN)|(((unsigned int)TFASC_API_VERSION_MJ)<<16) )

/*!
  @}
*/


// function decoration
#if defined(TFASC_API_DLL_EXPORTS)
// API function declaration used if the .h file is included by the DLL implementation itself.
#define TFASC_DECL
#else
// API function declaration used if the .h file is included (imported) by client code.
#define TFASC_DECL  __declspec(dllimport)
#endif

// Calling convention of exported API functions.
#define TFASC_CALL  __stdcall


//
// Functions exported by the DLL
//

#if defined (__cplusplus)
extern "C" {
#endif


/*!
  \defgroup api_functions Interface functions 
  @{

  \brief Functions exported by the DLL and called by the application.
*/



/*!
  \brief Returns the API version implemented by the DLL.
  
  \return
    This function returns the version number of the programming interface (API) implemented by the DLL. 
    The number returned is the #TFASC_API_VERSION constant the DLL has been compiled with.
*/
TFASC_DECL
unsigned int
TFASC_CALL
TFASC_GetApiVersion(void);


/*!
  \brief Check if the given API version number is compatible with this DLL.
  
  An application should call this function before it uses any other function exported by the DLL
  in order to verify its compatibility with the DLL at runtime.
  If this function fails then the function interface exported by the DLL is not compatible
  with the .h file the application has been compiled with and the application should 
  reject working with this DLL version.
  
  The DLL interface is considered compatible if its major version number is equal to \p majorVersion 
  and its minor version number is greater than or equal to \p minorVersion.
  
  \param[in] majorVersion
    The major version number the application is expecting.
    An application should set this parameter to the #TFASC_API_VERSION_MJ constant.

  \param[in] minorVersion
    The minor version number the application is expecting.
    An application should set this parameter to the #TFASC_API_VERSION_MN constant.
  
  \return
    The function returns 1 if the specified version constants are compatible with this DLL.
    The function returns 0 if the specified version constants are not compatible with this DLL.
*/
TFASC_DECL
int
TFASC_CALL
TFASC_CheckApiVersion(
  unsigned int majorVersion,
  unsigned int minorVersion
  );


/*!
  \brief Open TFA side channel driver.
  
  \return
    The function returns \c TSTATUS_SUCCESS if successful, an error code otherwise.
*/
TFASC_DECL
TSTATUS
TFASC_CALL
TFASC_Open(void);


/*!
  \brief Close TFA side channel driver.
*/
TFASC_DECL
void
TFASC_CALL
TFASC_Close(void);


/*!
  \brief Get TFA device count.

  \param[out] count
    Caller-provided variable which will be set to the number of TFA devices.

  \return
    The function returns \c TSTATUS_SUCCESS if successful, an error code otherwise.
*/
TFASC_DECL
TSTATUS
TFASC_CALL
TFASC_GetTfaDeviceCount(
  unsigned int* count
  );


/*!
  \brief Get TFA device ids.

  \param[out] ids
    Caller-provided array which will be fill with TFA device ids.

  \param[in] maxIds
    Maximum number of elements that can be copied into the ids array.

  \param[out] validIds
    Caller-provided variable which will be set to the number of valid TFA devices that are copied into ids array.

  \return
    The function returns \c TSTATUS_SUCCESS if successful, an error code otherwise.
*/
TFASC_DECL
TSTATUS
TFASC_CALL
TFASC_GetTfaDeviceIds(
	TfaDeviceId* ids,
	unsigned int maxIds,
	unsigned int* validIds
	);


/*!
  \brief Get information about a TFA device.

  \param[in] deviceId
    TFA device id.

  \param[out] info
    Caller-provided variable which will be receive the information of the TFA device specified by deviceId.

  \return
    The function returns \c TSTATUS_SUCCESS if successful, an error code otherwise.
*/
TFASC_DECL
TSTATUS
TFASC_CALL
TFASC_GetTfaDeviceInfo(
	TfaDeviceId deviceId,
	TfaDeviceInfo* info
	);


/*!
  \brief Get information about a specific TFA chip of the TFA device.

  \param[in] deviceId
    TFA device id.

  \param[in] chipIndex
    Zero based chip index.

  \param[out] info
    Caller-provided variable which will be receive the information of the TFA chip specified by deviceId and chipIndex.

  \return
    The function returns \c TSTATUS_SUCCESS if successful, an error code otherwise.
*/
TFASC_DECL
TSTATUS
TFASC_CALL
TFASC_GetTfaChipInfo(
  TfaDeviceId deviceId,
	unsigned int chipIndex,		// 0..numTfaChips-1
  TfaChipInfo* info
  );


/*!
  \brief Issue I2C transaction.

  \param[in] deviceId
    TFA device id.

  \param[in] busNumber
    I2C bus number.

  \param[in] slaveAddress
    I2C slave address.

  \param[in] writeData
    Data that should be written to the I2C bus.

  \param[in] numWriteBytes
    Number of bytes that should be written to the I2C bus.

  \param[out] readData
    Caller-provided array which will receive the read data from the I2C bus.

  \param[out] numReadBytes
    Number of bytes that should be read from the I2C bus.

  \return
    The function returns \c TSTATUS_SUCCESS if successful, an error code otherwise.
*/
TFASC_DECL
TSTATUS
TFASC_CALL
TFASC_I2cTransaction(
	TfaDeviceId deviceId,
	unsigned int busNumber,
	unsigned char slaveAddress,
	const unsigned char* writeData,
	unsigned int numWriteBytes,
	unsigned char* readData,
	unsigned int numReadBytes
	);


/*!
  @}
*/


#if defined (__cplusplus)
}
#endif


#endif

/******************************** EOF ****************************************/
