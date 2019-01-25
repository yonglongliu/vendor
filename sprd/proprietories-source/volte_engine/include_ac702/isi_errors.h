/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 26722 $ $Date: 2014-06-03 17:20:29 +0800 (Tue, 03 Jun 2014) $
 *
 */

#ifndef _ISI_ERRORS_H_
#define _ISI_ERRORS_H_

/*
 * These are D2 specific error codes and descriptive strings that
 * underlying protocols and apps can use to communicate local
 * D2 errors that are not HTTP server errors.  HTTP error codes
 * are values between 1xx-6xx.  D2 error codes are within the 0-99 range.
 *
 * Note if vport is used with Csi then these values must match the
 * one defined in com.d2tech.csi.client.utils.ErrorCodes.java
 */

#define ISI_NO_ERROR (-1)
#define ISI_NO_ERROR_STR ("")

/** Error code: No network connectivity is available. */
#define ISI_NO_NETWORK_AVAILABLE (0)
#define ISI_NO_NETWORK_AVAILABLE_STR ("No Network is available")

/** Error code: No IMS network can be contacted at this time. */
#define ISI_IMS_NETWORK_NOT_REACHABLE (2)
#define ISI_IMS_NETWORK_NOT_REACHABLE_STR ("IMS network is not reachable")

/** Error code: No IMS account is initialized. */
#define ISI_IMS_ACCOUNT_NOT_INITIALIZED (3)
#define ISI_IMS_ACCOUNT_NOT_INITIALIZED_STR ("The IMS account is not initialized")

/** Error code: Protocol is dead. */
#define ISI_PROTOCOL_DEAD (4)
#define ISI_PROTOCOL_DEAD_STR ("The protocol is dead")

/** Error code: Account was destroyed. */
#define ISI_ACCOUNT_DESTROYED (5)
#define ISI_ACCOUNT_DESTROYED_STR ("The Account was destroyed")

/** Error code: Not allowed at this time. */
#define ISI_NOT_ALLOWED (6)
#define ISI_NOT_ALLOWED_STR ("Not allowed at this time")

/** Error code: Message was empty. */
#define ISI_MESSAGE_NULL (7)
#define ISI_MESSAGE_NULL_STR ("The sent message was empty")

/** Error code: Non-specified circuit-switch messaging failure. */
#define ISI_GENERIC_CS_MESSAGING_FAILURE (8)
#define ISI_GENERIC_CS_MESSAGING_FAILURE_STR ("Unspecified circuit-switch messaging error")

/** Error code: You are not authorized. */
#define ISI_UNAUTHORIZED (9)
#define ISI_UNAUTHORIZED_STR ("You are not authorized to access this resource")

/** Error code: Invalid address. */
#define ISI_INVALID_ADDRESS (10)
#define ISI_INVALID_ADDRESS_STR ("Invalid address")

/** Error code: indicating that a Pipe used to share content was terminate
 * before the content 'completed' or 'finished' playing out.
 */
#define ISI_EARLY_TERMINATION (11)
#define ISI_EARLY_TERMINATION_STR ("Session was prematurely terminated")

/** Error code: indicating that the media stream in a pipe has 'broken' due to some generic network error. */
#define ISI_BROKEN_MEDIA_CONNECTION (12)
#define ISI_BROKEN_MEDIA_CONNECTION_STR ("Media Connection Broke")

/** Error code: indicating that a new pipe could not be initiated due to the protocol running out of resources. */
#define ISI_NO_AVAILABLE_RESOURCES (13)
#define ISI_NO_AVAILABLE_RESOURCES_STR ("No resources available to complete operation")

/** Error code: indicating that a request timed out. */
#define ISI_REQUEST_TIMED_OUT (14)
#define ISI_REQUEST_TIMED_OUT_STR ("Request timed out")

/** Error code: indicating that a ACK receipt timed out. */
#define ISI_ACK_RECEIPT_TIMED_OUT (15)
#define ISI_ACK_RECEIPT_TIMED_OUT_STR ("ACK receipt timed out")

/** Error code: RTP-RTCP Timeout */
#define ISI_RTP_RTCP_TIMEOUT  (16)
#define ISI_RTP_RTCP_TIMEOUT_STR    ("RTP_RTCP_TIMEOUT")

/**
 * Error code: indicating sip transport initialization failire.
 * I.e. failed to bind the sip socket.
 */
#define ISI_IMS_XPORT_INIT_FAILURE (17)
#define ISI_IMS_XPORT_INIT_FAILURE_STR ("IMS transport initialization failure")

#endif
