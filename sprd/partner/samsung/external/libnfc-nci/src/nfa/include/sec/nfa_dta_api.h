/******************************************************************************
 *
 *  Copyright (C) 2014 Samsung Electonics Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/


/******************************************************************************
 *
 *  This is the public interface file for NFA, Samsung Electronics's NFC application
 *  layer for mobile phones.
 *
 ******************************************************************************/
#ifndef NFA_DTA_API_H
#define NFA_DTA_API_H

#include "nfc_target.h"
#include "nci_defs.h"
#include "tags_defs.h"
#include "nfc_api.h"
#include "rw_api.h"
#include "nfc_hal_api.h"
#include "gki.h"



/*****************************************************************************
**  Constants and data types
*****************************************************************************/


//[START] *** Temporary data
typedef struct
{
    UINT8   poll;
    UINT8   listen;
}  tNFA_DTA_FL_POLL_LISTEN;

typedef struct
{
    UINT8   mode;
} tNFA_DTA_EMVCO_PCD_MODE;

typedef struct
{
    UINT8   cfg_item;
} tNFA_DTA_CFG_ITEM;

typedef struct
{
    UINT8 cfg;
} tNFA_DTA_CFG;

//[END] *** Temporary data


/* Union of all DM callback structures */
typedef union
{
    tNFA_STATUS         status;
    tNFC_START_DEVT    start;
    void                    *p_vs_evt_data;
} tNFA_DTA_CBACK_DATA;


typedef void (tNFA_DTA_CBACK) (UINT8 event, tNFA_DTA_CBACK_DATA *p_data);

/* DTA states */
enum
{
    NFA_DTA_PATTERN_0 = 0,
    NFA_DTA_PATTERN_1,
    NFA_DTA_PATTERN_2,
    NFA_DTA_PATTERN_3,
    NFA_DTA_PATTERN_4,
    NFA_DTA_PATTERN_5,
    NFA_DTA_PATTERN_6,
    NFA_DTA_PATTERN_7,
    NFA_DTA_PATTERN_8,
    NFA_DTA_PATTERN_9,
    NFA_DTA_PATTERN_A,
    NFA_DTA_PATTERN_B,
    NFA_DTA_PATTERN_C,
};

/* Send data timer value */
#define NFA_DTA_SEND_DATA_TIMEOUT       5000


/* NFA Connection Callback Events */
#define NFA_DTA_ENABLED_EVT         0   // Enable DTA mode
#define NFA_DTA_DISABLED_EVT        1   // Disable DTA mode
#define NFA_DTA_START_EVT           2   // Start DTA mode
#define NFA_DTA_STOP_EVT            3   // Stop DTA mode
#define NFA_DTA_CONFIG_EVT          4   // DTA Config


#define NFA_DTA_DATA_EVT            0


/*****************************************************************************
**  External Function Declarations
*****************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif
/*******************************************************************************
**
** Function         NFA_Init
**
** Description      This function initializes control blocks for NFA
**
**                  p_hal_entry_tbl points to a table of HAL entry points
**
**                  NOTE: the buffer that p_hal_entry_tbl points must be
**                  persistent until NFA is disabled.
**
**
** Returns          none
**
*******************************************************************************/
NFC_API extern void NFA_DTA_Init (tHAL_NFC_ENTRY *p_hal_entry_tbl);

/*******************************************************************************
**
** Function         NFA_Enable
**
** Description      This function enables NFC. Prior to calling NFA_Enable,
**                  the NFCC must be powered up, and ready to receive commands.
**                  This function enables the tasks needed by NFC, opens the NCI
**                  transport, resets the NFC controller, downloads patches to
**                  the NFCC (if necessary), and initializes the NFC subsystems.
**
**                  This function should only be called once - typically when NFC
**                  is enabled during boot-up, or when NFC is enabled from a
**                  settings UI. Subsequent calls to NFA_Enable while NFA is
**                  enabling or enabled will be ignored. When the NFC startup
**                  procedure is completed, an NFA_DM_ENABLE_EVT is returned to the
**                  application using the tNFA_DM_CBACK.
**
**                  The tNFA_CONN_CBACK parameter is used to register a callback
**                  for polling, p2p and card emulation events.
**
**
** Returns          NFA_STATUS_OK if successfully initiated
**                  NFA_STATUS_FAILED otherwise
**
*******************************************************************************/
NFC_API extern tNFA_STATUS NFA_DTA_Enable (BOOLEAN autoStart,
                                       tNFA_DTA_CBACK     *p_dta_cback);

/*******************************************************************************
**
** Function         NFA_Disable
**
** Description      This function is called to shutdown NFC. The tasks for NFC
**                  are terminated, and clean up routines are performed. This
**                  function is typically called during platform shut-down, or
**                  when NFC is disabled from a settings UI. When the NFC
**                  shutdown procedure is completed, an NFA_DM_DISABLE_EVT is
**                  returned to the application using the tNFA_DM_CBACK.
**
**                  The platform should wait until the NFC_DISABLE_REVT is
**                  received before powering down the NFC chip and NCI transport.
**                  This is required to so that NFA can gracefully shut down any
**                  open connections.
**
** Returns          NFA_STATUS_OK if successfully initiated
**                  NFA_STATUS_FAILED otherwise
**
*******************************************************************************/
NFC_API extern tNFA_STATUS NFA_DTA_Disable ();


/*******************************************************************************
**
** Function         NFA_Disable
**
** Description      This function is called to shutdown NFC. The tasks for NFC
**                  are terminated, and clean up routines are performed. This
**                  function is typically called during platform shut-down, or
**                  when NFC is disabled from a settings UI. When the NFC
**                  shutdown procedure is completed, an NFA_DM_DISABLE_EVT is
**                  returned to the application using the tNFA_DM_CBACK.
**
**                  The platform should wait until the NFC_DISABLE_REVT is
**                  received before powering down the NFC chip and NCI transport.
**                  This is required to so that NFA can gracefully shut down any
**                  open connections.
**
** Returns          NFA_STATUS_OK if successfully initiated
**                  NFA_STATUS_FAILED otherwise
**
*******************************************************************************/
NFC_API extern tNFA_STATUS NFA_DTA_Start (UINT16 patternNo, UINT16 tlv_len,  UINT8 *tlv);


/*******************************************************************************
**
** Function         NFA_Disable
**
** Description      This function is called to shutdown NFC. The tasks for NFC
**                  are terminated, and clean up routines are performed. This
**                  function is typically called during platform shut-down, or
**                  when NFC is disabled from a settings UI. When the NFC
**                  shutdown procedure is completed, an NFA_DM_DISABLE_EVT is
**                  returned to the application using the tNFA_DM_CBACK.
**
**                  The platform should wait until the NFC_DISABLE_REVT is
**                  received before powering down the NFC chip and NCI transport.
**                  This is required to so that NFA can gracefully shut down any
**                  open connections.
**
** Returns          NFA_STATUS_OK if successfully initiated
**                  NFA_STATUS_FAILED otherwise
**
*******************************************************************************/
NFC_API extern tNFA_STATUS NFA_DTA_Stop ();



/*******************************************************************************
**
** Function         NFA_Disable
**
** Description      This function is called to shutdown NFC. The tasks for NFC
**                  are terminated, and clean up routines are performed. This
**                  function is typically called during platform shut-down, or
**                  when NFC is disabled from a settings UI. When the NFC
**                  shutdown procedure is completed, an NFA_DM_DISABLE_EVT is
**                  returned to the application using the tNFA_DM_CBACK.
**
**                  The platform should wait until the NFC_DISABLE_REVT is
**                  received before powering down the NFC chip and NCI transport.
**                  This is required to so that NFA can gracefully shut down any
**                  open connections.
**
** Returns          NFA_STATUS_OK if successfully initiated
**                  NFA_STATUS_FAILED otherwise
**
*******************************************************************************/
NFC_API extern tNFA_STATUS NFA_DTA_Config (UINT16 configItem, UINT16 configData);


#ifdef __cplusplus
}
#endif


#endif //NFA_DTA_API_H
