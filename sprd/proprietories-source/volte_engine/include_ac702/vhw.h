/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2005 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29023 $ $Date: 2014-09-25 17:28:52 +0800 (Thu, 25 Sep 2014) $
 */

#ifndef _VHW_H_
#define _VHW_H_

#include <osal_types.h>
#include <osal.h>

/*
 * Define the channel buffer size in samples per processing period
 */
#define VHW_PCM_BUF_SZ          (80)

/*
 * Number of PCM channels supported.
 */ 
#define VHW_NUM_PCM_CH          (1)

/*
 * Number of SPI channels supported
 */
#define VHW_NUM_SPI_CH          (0)
 
/*
 * VHW Error codes
 */

/*
 * Set physical interfaces audio characteristics
 */
#define VHW_AUDIO_SHIFT         (1)
#undef  VHW_AUDIO_MULAW

/*
 * This macro returns start address of buffer for a channel 'ch' given data_ptr 
 * points to the start address of buffer for ch0.
 * Returns NULL if buffer is not available.
 */
#define VHW_GETBUF(data_ptr, ch)      \
        (ch > (VHW_NUM_PCM_CH - 1)) ? \
         NULL : (((vint *)data_ptr) + (VHW_PCM_BUF_SZ * ch))

/* 
 * Function prototypes
 */

#define VHW_AMP_DISABLE
#define VHW_AMP_ENABLE

int VHW_init(
    void);

void VHW_start(
    void);

void VHW_shutdown(
    void);

void VHW_exchange(
    int **tx_ptr,
    int **rx_ptr);

void VHW_attach(
    void);

void VHW_detach(
    void);

int VHW_isSleeping(
    void);

#endif
