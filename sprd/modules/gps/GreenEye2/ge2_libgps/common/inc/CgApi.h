#ifndef _CGAPI_H_
#define _CGAPI_H_

#include "agps_type.h"

#ifdef __cplusplus
extern "C" {
#endif

unsigned short CG_deliverAssistData_RefTime(TCgRxNTime * apTime);
unsigned short CG_deliverAssistData_RefLoc(void * apPos, TCgRxNTime * apTime);
unsigned short CG_deliverAssistData_NavMdl(TCgAgpsEphemerisCollection_t * apEphemerisCollection);
unsigned short CG_deliverAssistData_AcquisAssist(TCgAgpsAcquisAssist_t *apAcquisAssist);
unsigned short CG_deliverAssistData_End(void);

unsigned short CG_deliverMSAPosition(TCgAgpsPosition *msaPosition, TcgVelocity  *agpsVelocity);  // TCgAgpsPosition in CgTypes.h
unsigned short CG_getPosition(TcgTimeStamp *pTime, TCgAgpsPosition *pLastPos, TcgVelocity *pVelocity); // TcgVelocity in CgTypes.h

void CG_suplReadyReceivePosInfo(int sid, char rspType);

/*get Historical Report from engine*/
unsigned short CG_readyRecvHistoricReport(unsigned char sid,
                                         CgHistoricReporting_t *histRpt, 
                                         TCgSetQoP_t *qop);

unsigned short CG_deliverLocOfAnotherSet(TCgAgpsPosition *msaPosition,
                                         TcgVelocity  *agpsVelocity
                                                );
/**
	Put necessary initialization code here, will be called on SUPL Client start.

	\retval 0 on success
*/
CG_SO_SETAPI unsigned long CG_initPhoneItf(void);

/**
	Put necessary de-initialization code here, will be called on SUPL Client end.

	\retval 0 on success
*/
CG_SO_SETAPI unsigned long CG_closePhoneItf(void);

/*
 The following 6 functions will all return a pointer to a piece of static memory, 
 in the calling thread do NOT try to free the pointer.
 In case of need, copy the content to your own memory instead.

 msisdn, mdn and imsi are a BCD (Binary Coded Decimal) string
 represent digits from 0 through 9,
 two digits per octet, each digit encoded 0000 to 1001 (0 to 9)
 bits 8765 of octet n encoding digit 2n
 bits 4321 of octet n encoding digit 2(n-1) +1
 not used digits in the string shall be filled with 1111

 MSISDN and IMSI are used more often.
 ==============================================================================
 From Wikipedia, the free encyclopedia

 MSISDN is a number uniquely identifying a subscription in a GSM or a UMTS mobile network. 
 Simply put, it is the telephone number of the SIM card in a mobile/cellular phone. 
 This abbreviation has several interpretations, the most common one being "Mobile Subscriber 
 Integrated Services Digital Network Number".[1]

 The MSISDN together with IMSI are two important numbers used for identifying a mobile subscriber. 
 The latter identifies the SIM, i.e. the card inserted in to the mobile phone, while the former is 
 used for routing calls to the subscriber. IMSI is often used as a key in the HLR ("subscriber 
 database") and MSISDN is the number normally dialed to connect a call to the mobile phone. 
 A SIM is uniquely associated to an IMSI, while the MSISDN can change in time (e.g. due to number 
 portability), i.e. different MSISDNs can be associated to the SIM.

 The MSISDN follows the numbering plan defined in the ITU-T recommendation E.164.
 ==============================================================================
 According the choice of SET ID, implement one of the following functions
 ==============================================================================
*/

/**
	pBuf will be filled with BCD encoded MSISDN.

	\retval 0 on success
*/
CG_SO_SETAPI unsigned long CG_getMSISDN(unsigned char* pBuf, unsigned short usLenBuf);

/**
	pBuf will be filled with BCD encoded IMSI.

	\retval 0 on success
*/
CG_SO_SETAPI unsigned long CG_getIMSI(unsigned char* pBuf, unsigned short usLenBuf);

/**
	pBuf will be filled with BCD encoded MDN.

	\retval 0 on success
*/
CG_SO_SETAPI unsigned long CG_getMDN(unsigned char* pBuf, unsigned short usLenBuf);

/**
	pBuf will be filled with MIN. The 5th byte's 6 Least Significant bits are not used. 

	\retval 0 on success
*/
CG_SO_SETAPI unsigned long CG_getMIN(unsigned char* pBuf, unsigned short usLenBuf);

/**
	pBuf will be filled with NAI.

	\retval 0 on success
*/
CG_SO_SETAPI unsigned long CG_getNAI(unsigned char* pBuf, unsigned short usLenBuf);

/*
	Use SET's IP address as SET's ID. In some cases,  SET's IP address isn't global network address nor is fixed. For 
	example some mobile phone's IP address in GRPS network is dynamically allocated and will change from time to 
	time. So it's not recommended to use SET's IP address as its ID. 
	Only IPV4 address is supported by now. 
	pBuf will be filled with IP address. 
*/
// unsigned long CG_getSET_IPAddress(unsigned char* pBuf, unsigned short usLenBuf);

/**
	Select the SET ID type

	\retval the SET ID type
*/
CG_SO_SETAPI CG_SETId_PR CG_getSETID_Type(void);	// This will return the type of SET ID, implemented by the user


/*
	In SET (SUPL Enabled Terminal) LID (Location ID) must be known.
	LID describles the gloabally unique cell identification of the most current
	serving cell.
	This module will interface with the phone's protocol stack to get the LID.

	SUPL Client needs to know the Mobile Device's Location Identifier (in case of GSM network, it 
	includes MCC-MNC-LAC-Cell_Id). Normally this information could only be got from the Mobile Device's 
	Wireless (GSM/CDMA/WCDMA/WiMAX, etc.) protocol stack. 
	Most GSM/GPRS modem provides AT command interface through serial port. Using AT command it's 
	easy to get the information. However, in most Windows Mobile platform, the GSM/GPRSM can't be 
	accessed through the serial port. In stead a RIL (Radio Interface Layer) is provided and could be used to 
	get the information. In the EVK, an example of using WM5's RIL is used to get the Location Identifier. 

	unsigned long CG_getLocationId(LocationId_t *pLocationId); 
	Function Description:  
	This method allows SUPL client to get current serving Cell information from SET. Some times when  change a SIM 
	card or SET moving into a new Cell, the Location information will change, and this information could be retrieved 
	from Mobile Communication Protocol Stack.  
	This function will be called in each SUPL session. 
	The SUPL Client protocol stack only doesn't care the system type (GSM, CDMA or WCDMA) 
	except in this function. 
*/

/**
	This function is called directly by the SUPL client, and in turn calls one of the 
	following sub-functions.
	(CG_getGsmCellInformation, CG_getCdmaCellInformation or CG_getWcdmaCellInformation).
	The type of Cell used by the SET, determines which function will be called to retrieve partial LocationId information.
*/
CG_SO_SETAPI unsigned long CG_getLocationId(CgLocationId_t *pLocationId);

/**
	In case of GSM system, CG_getLocationId should provide CgGsmCellInformation_t

	\retval 0 on success
*/
CG_SO_SETAPI unsigned long CG_getGsmCellInformation(CgGsmCellInformation_t * pGsmCellInfo);

/**
	In case of CDMA system, CG_getLocationId should provide CgCdmaCellInformation_t

	\retval 0 on success
*/
CG_SO_SETAPI unsigned long CG_getCdmaCellInformation(CgCdmaCellInformation_t * pCdmaCellInfo);

/**
	In case of WCDMA system, CG_getLocationId should provide CgWcdmaCellInformation_t

	\retval 0 on success
*/
CG_SO_SETAPI unsigned long CG_getWcdmaCellInformation(CgWcdmaCellInformation_t * pWcdmaCellInfo);

/**
	Return the location ID type, currently available on the SET

	\retval 0 on success
*/
CG_SO_SETAPI unsigned long CG_getPresentCellInfo(CgCellInfo_PR * apCellInfo);

/**
	Register the SUPL library, to receive SUPL_INIT messages from tne SUPL network,
	via dedicated SMS or WAP messages.

	\retval 0 on success
*/
CG_SO_SETAPI unsigned long CG_registerWAPandSMS(GpsNiNotification *notification);

/******************************************************************************
This function will show a Message Box (if required in aNotificationType) then wait (if required
in aNotificationType) until user clicks or timer expires.
Blocking.
Expected to be implemented by customer.
Return value:
-- 0 User Accepted
-- 1 User Denied
-- 2 time-out
******************************************************************************/
//CG_SO_SETAPI unsigned char CG_showNotifVerif(CG_e_NotificationType aNotificationType);

CG_SO_SETAPI void CG_SUPL_NI_END(void);

CG_SO_SETAPI int CG_getWlanApInfo(CgWlanCellList_t *pCgWlanCellList);

CG_SO_SETAPI unsigned long CG_getIPAddress(CgIPAddress_t *ipAddress);

#ifdef __cplusplus
}
#endif



#endif
