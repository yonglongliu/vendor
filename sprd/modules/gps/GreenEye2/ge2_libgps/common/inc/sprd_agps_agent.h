/*
 *  Generic aGPS Stack Customer Interface
 *
 *  Copyright (C) 2013 Spreadtrum Communications Inc.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 */

#ifndef __SPRD_AGPS_AGENT_H
#define __SPRD_AGPS_AGENT_H

/* Basic data type define */
typedef int bool_t;

typedef struct
{
    unsigned int    ota_type;      /* over-the-air type (RRLP/RRC/LPP) */
    unsigned int    rrc_msg_type;
    int             isfinal;       /* is final response? */
    unsigned int    msg_size;      /* uplink payload size */
    unsigned char  *msg_data;
    unsigned int    addl_info_len;
    unsigned char  *addl_info;
}agps_ul_pdu_params_t;

typedef struct
{
    unsigned int    ota_type;
    unsigned int    rrc_state;
    unsigned int    session_priority;
    unsigned int    msg_size;
    unsigned char  *msg_data;
    unsigned int    addl_info_len;
    unsigned char  *addl_info;
}agps_dl_pdu_params_t;

/** handle_NotifyMTLR structure */
typedef struct
{
    int    handle_id;
    int    ni_type;
    int    notify_type;
    int    requestorID_len;
    int    requestorID_dcs;
    char  *requestorID;
    int    clientName_len;
    int    clientName_dcs;
    char  *clientName;
}agps_user_notify_params_t;

/* structure of +CMOLR parameter */
typedef struct 
{
    int   enable;
    int   method;
    int   hor_acc_set;
    int   hor_acc;
    int   ver_req;
    int   ver_acc_set;
    int   ver_acc;
    int   vel_req;
    int   rep_mode;
    int   timeout;
    int   interval;
    int   shape_rep;
    int   plane;
    char *NMEA_rep;
    char *third_party_addr;
}agps_molr_params_t;

typedef enum
{
    ELEMENT_LOCATION,
    ELEMENT_GNSS_LOCATION,
    ELEMENT_ASSIST_DATA,
    ELEMENT_POS_MEAS,
    ELEMENT_POS_MEAS_REQ,
    ELEMENT_GPS_MEAS,
    ELEMENT_OTDOA_MEAS,
    ELEMENT_GNSS_MEAS,
    ELEMENT_GPS_ASSIST_REQ,
    ELEMENT_GNSS_ASSIST_REQ,
    ELEMENT_OTDOA_ASSIST_REQ,
    ELEMENT_CAPABILITY_REQ,
    ELEMENT_CAPABILITIES,
    ELEMENT_MSG,
    ELEMENT_POS_ERR,
    ELEMENT_RESET_ASSIST_DATA,
    ELEMENT_GPS_GNSS_ASSIST_DATA,
    ELEMENT_MAXNUM
    
}PosElementPR_e;

typedef struct
{
    int   pdu_len;
    char *pdu;
}AssistReq_t;

typedef struct
{
    bool_t           present_gps_only;
    AssistReq_t      nonlpp_gps_only;
    bool_t           present_gnss_excl_gps;
    AssistReq_t      nonlpp_gnss_excl_gps;
    bool_t           present_lpp;
    AssistReq_t      lpp;
}GnssAssistReq_t;

typedef enum
{
    POS_PROTO_RRLP,
    POS_PROTO_RRC,
    POS_PROTO_LPP
}PosProtoType_e;

/* structure of +CPOS parameter */
typedef struct 
{
    bool_t  enable;
    PosElementPR_e  present;
    union
    {
        GnssAssistReq_t         gnss_assist_req;

        /* todo: Add more */
        
    }choice;

    /** 
     **RESERVED 
    PosProtoType_e protocol;
    int  transaction_id;
    **/
}agps_pos_params_t;

// MTLR request definitions
typedef int (*agps_cppayload)(agps_dl_pdu_params_t *dl_pdu_params);
typedef int (*agps_measureterminate)(void);
typedef int (*agps_rrcstateevent)(unsigned int rrc_state);
typedef int (*agps_e911dial_notify)(void);
typedef int (*agps_cpreset)(short type);
typedef int (*agps_mtlrnotify)(agps_user_notify_params_t *agps_user_notify_params);
typedef int (*agps_molr_resp)(int locationEstimate_len, unsigned char *locationEstimate);
typedef int (*agps_pos_resp)(PosElementPR_e evt_type, int len, char *rsp_param);



/** AGPS C-Plane hook structure. */
typedef struct agps_c_plane_hook 
{
    /** set to sizeof(struct agps_c_plane_hook) */
    size_t size;
    agps_cppayload          cppayload_cb;
    agps_measureterminate   meas_term_cb;
    agps_rrcstateevent      rrc_state_cb;
    agps_e911dial_notify   e911notify_cb;
    agps_cpreset             cp_reset_cb;
    agps_mtlrnotify       mtlr_notify_cb;
    agps_molr_resp          molr_resp_cb;
    agps_pos_resp            pos_resp_cb;
}agps_c_plane_hook_t;

typedef struct agps_c_plane_interface 
{
    /** set to sizeof(struct agps_c_plane_interface) */
    size_t size;
    /**
     * Opens the interface and provides the callback functions.
     */
    int (*init)(agps_c_plane_hook_t *hook);

    /** Closes the interface. */
    void (*exit)(void);

    /** enable/disable agps control plane feature */
    int (*lcs_enable)(bool_t is_enable);
    /** Send uplink payload request to network. */
    int (*network_ulreq)(agps_ul_pdu_params_t *ul_pdu_params);
    /** Send MTLR verification response to network. */
    int (*lcsverificationresp)(unsigned long notifId,     /* Notification ID (unique) */
                               unsigned long present,     /* Indicates if verification response is present */
                               unsigned long verificationResponse); /* User's verification response value */
    int (*lcs_molr)(agps_molr_params_t *agps_molr_params);
    int (*lcs_pos)(agps_pos_params_t *agps_pos_params);

}agps_c_plane_interface_t;

/*---------------------------------------------*
**               FUNCTIONS                     *
**---------------------------------------------*/
const agps_c_plane_interface_t *get_agps_cp_interface(void);

#endif
