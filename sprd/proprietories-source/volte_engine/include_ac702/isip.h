/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30168 $ $Date: 2014-12-02 16:40:06 +0800 (Tue, 02 Dec 2014) $
 *
 */

#ifndef _ISIP_H_
#define _ISIP_H_

/*
 * Protocol enumerations.
 * Each individual number represents a protocol/module type
 * Warning: These hard associations are just for reference, and are not used
 * internally by ISI. These associations hold as long as protocol application
 * uses them.
 */
#define ISI_PROTOCOL_VTSP      (0)
#define ISI_PROTOCOL_SIP       (1)
#define ISI_PROTOCOL_XMPP      (2)
#define ISI_PROTOCOL_GSM       (3)
#define ISI_PROTOCOL_YAHOO     (4)
#define ISI_PROTOCOL_SAMETIME  (5)
#define ISI_PROTOCOL_SIP_RCS   (6)
#define ISI_PROTOCOL_LTE       (7) /* MAPP */

#ifdef VPORT_4G_PLUS_APROC
#define ISIP_PROTOCOL_PIPE      "aproc.isi.ipc.protocol"
#define ISIP_APPLICATION_PIPE   "aproc.isi.ipc.application"
#else
#define ISIP_PROTOCOL_PIPE      "isi.ipc.protocol"
#define ISIP_APPLICATION_PIPE   "isi.ipc.application"
#endif


/* RTCP masks. */
#define ISIP_MASK_RTCP_SR           (1 << 0)
#define ISIP_MASK_RTCP_XR           (1 << 1)
#define ISIP_MASK_RTCP_RR           (1 << 2)
#define ISIP_MASK_RTCP_BYE          (1 << 3)
#define ISIP_MASK_RTCP_FB_NACK      (1 << 4)
#define ISIP_MASK_RTCP_FB_PLI       (1 << 5)
#define ISIP_MASK_RTCP_FB_FIR       (1 << 6)
#define ISIP_MASK_RTCP_FB_TMMBR     (1 << 7)
#define ISIP_MASK_RTCP_FB_TMMBN     (1 << 8)

/*
 * What type of message this is.
 */
typedef enum {
    ISIP_CODE_INVALID = 0,
    ISIP_CODE_SERVICE,       /* Service setup / modify */
    ISIP_CODE_CALL,          /* Call setup / modify */
    ISIP_CODE_MESSAGE,       /* Instant message */
    ISIP_CODE_PRESENCE,      /* Presence */
    ISIP_CODE_TEL_EVENT,     /* Event */
    ISIP_CODE_MEDIA,         /* Media messages */
    ISIP_CODE_SYSTEM,        /* System */
    ISIP_CODE_CHAT,          /* Chat */
    ISIP_CODE_FILE,          /* File Transfer */
    ISIP_CODE_DIAGNOSTIC,    /* Diagnostic */
    ISIP_CODE_USSD,          /* USSD */
    ISIP_CODE_LAST
} ISIP_Code;

/*
 * Reason for system level config/control messages.
 */
typedef enum {
    ISIP_SYSTEM_REASON_START,
    ISIP_SYSTEM_REASON_SHUTDOWN,
    ISIP_SYSTEM_REASON_NOT_SUPPORTED,
    ISIP_SYSTEM_REASON_LAST,
} ISIP_SystemReason;

/*
 * Reason for service message.
 */
typedef enum {
    ISIP_SERVICE_REASON_INVALID = 0,
    ISIP_SERVICE_REASON_CREATE,         /* Create a service */
    ISIP_SERVICE_REASON_DESTROY,        /* Destroy a service */
    ISIP_SERVICE_REASON_ACTIVATE,       /* Activate a service / register etc */
    ISIP_SERVICE_REASON_DEACTIVATE,     /* Deactivate a service */
    ISIP_SERVICE_REASON_HANDOFF,        /* VCC related reason used for 'handoff' */
    ISIP_SERVICE_REASON_BLOCKUSER,      /* Block a remote user */
    ISIP_SERVICE_REASON_IDENTITY,       /* Hide / show local call ID */
    ISIP_SERVICE_REASON_SERVER,         /* Set / change server */
    ISIP_SERVICE_REASON_CODERS,         /* Coder list update */
    ISIP_SERVICE_REASON_AUTH,           /* Credential update */
    ISIP_SERVICE_REASON_URI,            /* Set/change the URI for the service */
    ISIP_SERVICE_REASON_BSID,           /* Set/change the BSID for the service */
    ISIP_SERVICE_REASON_NET,            /* Some network problem */
    ISIP_SERVICE_REASON_FILE,           /* Set / change download file path */
    ISIP_SERVICE_REASON_AUTH_AKA_CHALLENGE, /* AKA authentication challenge */
    ISIP_SERVICE_REASON_AUTH_AKA_RESPONSE,  /* AKA authentication response */
    ISIP_SERVICE_REASON_CAPABILITIES,
    ISIP_SERVICE_REASON_PORT,           /* Set or change port and pool size */
    ISIP_SERVICE_REASON_EMERGENCY,      /* Set emergency */
    ISIP_SERVICE_REASON_IMEI_URI,       /* Set IMEI URI*/
    ISIP_SERVICE_REASON_IPSEC,          /* Set / CHANGE IPSec ports and SPI pool */
    ISIP_SERVICE_REASON_INSTANCEID,
    ISIP_SERVICE_REASON_SET_PROVISIONING_DATA, /* Set provisioning data */
    ISIP_SERVICE_REASON_LAST
} ISIP_ServiceReason;

typedef enum {
    ISIP_CHAT_REASON_INVALID = 0,
    ISIP_CHAT_REASON_INITIATE,
    ISIP_CHAT_REASON_TRYING,
    ISIP_CHAT_REASON_ACK,
    ISIP_CHAT_REASON_ACCEPT,
    ISIP_CHAT_REASON_TERMINATE,
    ISIP_CHAT_REASON_REJECT,
    ISIP_CHAT_REASON_FAILED,
    ISIP_GROUP_CHAT_REASON_INITIATE,
    ISIP_GROUP_CHAT_REASON_JOIN,
    ISIP_GROUP_CHAT_REASON_INVITE,
    ISIP_GROUP_CHAT_REASON_KICK,
    ISIP_GROUP_CHAT_REASON_DESTROY,
    ISIP_GROUP_CHAT_REASON_NET,
    ISIP_GROUP_CHAT_REASON_AUTH,
    ISIP_DEFERRED_CHAT_REASON_INITIATE,
    ISIP_CHAT_REASON_LAST
} ISIP_ChatReason;

/*
 * Reason for a call message.
 */
typedef enum {
    ISIP_CALL_REASON_INVALID = 0,
    ISIP_CALL_REASON_INITIATE,          /* Call init */
    ISIP_CALL_REASON_TERMINATE,         /* Call terminate */
    ISIP_CALL_REASON_ACCEPT,            /* Call accept */
    ISIP_CALL_REASON_ACK,               /* Call acknowledge */
    ISIP_CALL_REASON_REJECT,            /* Call reject with code */
    ISIP_CALL_REASON_TRYING,            /* Call is being tried */
    ISIP_CALL_REASON_ERROR,             /* Error in call */
    ISIP_CALL_REASON_FAILED,            /* the call failed, for example the user was not found */
    ISIP_CALL_REASON_CREDENTIALS,       /* Error with call credentials */
    ISIP_CALL_REASON_HOLD,              /* Call hold */
    ISIP_CALL_REASON_RESUME,            /* Call resumed */
    ISIP_CALL_REASON_FORWARD,           /* Forward call */
    ISIP_CALL_REASON_TRANSFER_PROGRESS, /* We have successfully made a transfer request */
    ISIP_CALL_REASON_TRANSFER_ATTEMPT,  /* Call Transfer Attempt */
    ISIP_CALL_REASON_TRANSFER_BLIND,    /* Transfer blind */
    ISIP_CALL_REASON_TRANSFER_COMPLETED,/* Transfer completed */
    ISIP_CALL_REASON_TRANSFER_FAILED,   /* Transfer failed */
    ISIP_CALL_REASON_TRANSFER_ATTENDED, /* Transfer attended */
    ISIP_CALL_REASON_TRANSFER_CONSULT,  /* Transfer after consultation */
    ISIP_CALL_REASON_MODIFY,            /* Call modify */
    ISIP_CALL_REASON_CANCEL_MODIFY,     /* Cancel Call modify */
    ISIP_CALL_REASON_ACCEPT_MODIFY,     /* Accept call modify offer */
    ISIP_CALL_REASON_REJECT_MODIFY,     /* Reject call modify offer */
    ISIP_CALL_REASON_VDX,               /* VCC Domain Transfer Number/URI (i.e. VDN or VDI) */
    ISIP_CALL_REASON_HANDOFF,           /* A VCC "Handoff" message from the underlying protocol */
    ISIP_CALL_REASON_BEING_FORWARDED,   /* Call is being forwarded */
    ISIP_CALL_REASON_HANDOFF_SUCCESS,   /* A VCC "handoff" is successful message from the underlying protocol */
    ISIP_CALL_REASON_HANDOFF_FAILED,    /* A VCC "handoff" is failed message from the underlying protocol */
    ISIP_CALL_REASON_ACCEPT_ACK,        /* Acknowledge for ACCEPT(200 OK) */
    ISIP_CALL_REASON_CONF_KICK,         /* Kick out a participant from a conference call. */
    ISIP_CALL_REASON_RTP_INACTIVE_TMR_DISABLE, /* disable rtp inactivity timer */
    ISIP_CALL_REASON_RTP_INACTIVE_TMR_ENABLE, /* enable rtp inactivity timer */
    ISIP_CALL_REASON_TRANSFER_CONF,     /* Transfer with Rsrc List */
    ISIP_CALL_REASON_INITIATE_CONF,     /* Call init with Rsrc List */
    ISIP_CALL_REASON_VIDEO_REQUEST_KEY,     /* Request a key frame */
    ISIP_CALL_REASON_EARLY_MEDIA,       /* early media state */
    ISIP_CALL_REASON_LAST
} ISIP_CallReason;

/*
 * Reason for a text message.
 */
typedef enum {
    ISIP_TEXT_REASON_INVALID = 0,
    ISIP_TEXT_REASON_NEW,              /* init or new text message */
    ISIP_TEXT_REASON_ERROR,            /* Error with sending a text message */
    ISIP_TEXT_REASON_COMPLETE,         /* completed sending a text message */
    ISIP_TEXT_REASON_REPORT,           /* A display report for a text message */
    ISIP_TEXT_REASON_COMPOSING_ACTIVE, /* Indicate user is composing message */
    ISIP_TEXT_REASON_COMPOSING_IDLE,   /* Indicate user is idle(i.e. not composing) */
    ISIP_TEXT_REASON_NEW_NO_EVENT,  /* A new message but DO NOT send an event to the application */
} ISIP_TextReason;

/*
 * Reason for a file transfer.
 */
typedef enum {
    ISIP_FILE_REASON_INVALID = 0,
    ISIP_FILE_REASON_NEW,
    ISIP_FILE_REASON_ACKNOWLEDGE,
    ISIP_FILE_REASON_ACCEPT,
    ISIP_FILE_REASON_START_SEND,
    ISIP_FILE_REASON_REJECT,
    ISIP_FILE_REASON_CANCEL,
    ISIP_FILE_REASON_PROGRESS,
    ISIP_FILE_REASON_ERROR,
    ISIP_FILE_REASON_COMPLETE,
    ISIP_FILE_REASON_SHUTDOWN,
    ISIP_FILE_REASON_TRYING,
} ISIP_FileReason;

/*
 * Reason for a tel event message.
 */
typedef enum {
    ISIP_TEL_EVENT_REASON_INVALID = 0,
    ISIP_TEL_EVENT_REASON_NEW,       /* init or new tel event message */
    ISIP_TEL_EVENT_REASON_ERROR,     /* Error with sending the tel event message */
    ISIP_TEL_EVENT_REASON_COMPLETE,  /* completed sending a tel event message */
} ISIP_TelEvtReason;

/*
 * Reason for a presence event message.
 */
typedef enum {
    ISIP_PRES_REASON_INVALID = 0,
    ISIP_PRES_REASON_PRESENCE,
    ISIP_PRES_REASON_CONTACT_ADD,
    ISIP_PRES_REASON_CONTACT_RM,
    ISIP_PRES_REASON_SUB_TO_PRES,
    ISIP_PRES_REASON_UNSUB_TO_PRES,
    ISIP_PRES_REASON_SUB_ALLOWED,
    ISIP_PRES_REASON_SUB_REFUSED,
    ISIP_PRES_REASON_COMPLETE,
    ISIP_PRES_REASON_CAPABILITIES,
    ISIP_PRES_REASON_CAPABILITIES_REQUEST,
    ISIP_PRES_REASON_ERROR,
} ISIP_PresReason;

/*
 * Audio reason codes.
 */
typedef enum {
    ISIP_MEDIA_REASON_INVALID = 0,
    ISIP_MEDIA_REASON_RINGSTART,     /* Start ring */
    ISIP_MEDIA_REASON_RINGSTOP,      /* Stop ring */
    ISIP_MEDIA_REASON_TONESTART,     /* Start tone, commanded from ISI SM */
    ISIP_MEDIA_REASON_TONESTOP,      /* Stop tone, commanded from ISI SM */
    ISIP_MEDIA_REASON_TONESTART_CMD, /* Start tone, commanded from ISI API */
    ISIP_MEDIA_REASON_TONESTOP_CMD,  /* Stop tone, commanded from ISI API */
    ISIP_MEDIA_REASON_STREAMSTART,   /* Start stream */
    ISIP_MEDIA_REASON_STREAMSTOP,    /* Stop stream */
    ISIP_MEDIA_REASON_STREAMMODIFY,  /* Modify stream */
    ISIP_MEDIA_REASON_CONFSTART,     /* Start conf */
    ISIP_MEDIA_REASON_CONFSTOP,      /* Stop conf */
    ISIP_MEDIA_REASON_TIMER,         /* A timer event */
    ISIP_MEDIA_REASON_PKT_RECV,      /* A received STUN packet */
    ISIP_MEDIA_REASON_PKT_SEND,      /* A STUN packet to send */
    ISIP_MEDIA_REASON_SPEAKER,       /* Indicates speaker is on, ocntrols aec */
    ISIP_MEDIA_REASON_RTP_RTCP_INACTIVE, /* RTP/RTCP inactive timeout */
    ISIP_MEDIA_REASON_RTP_INACTIVE_TMR_DISABLE, /* disable rtp inactivity tmr */
    ISIP_MEDIA_REASON_RTP_INACTIVE_TMR_ENABLE,  /* enable rtp inactivity tmr */
    ISIP_MEDIA_REASON_AEC_ENABLE,    /* Bypass AEC or not */
    ISIP_MEDIA_REASON_GAIN_CONTROL,  /* Software gain control */
    ISIP_MEDIA_REASON_CN_GAIN_CONTROL,  /* Software cn gain control */
    ISIP_MEDIA_REASON_LAST
} ISIP_MediaReason;

/*
 * General status codes for update of a pending action.
 */
typedef enum {
    ISIP_STATUS_INVALID = 0,
    ISIP_STATUS_TRYING,
    ISIP_STATUS_DONE,
    ISIP_STATUS_FAILED,
    ISIP_STATUS_LAST
} ISIP_Status;

typedef struct {
    char             szCoderName[ISI_CODER_STRING_SZ + 1];
    char             description[ISI_CODER_DESCRIPTION_STRING_SZ + 1];
    ISI_SessionType  relates;
} ISIP_Coder;

typedef struct {
    ISIP_SystemReason reason;
    ISIP_Status       status;
    char              protocolIpc[ISI_ADDRESS_STRING_SZ];
    char              mediaIpc[ISI_ADDRESS_STRING_SZ];
    char              streamIpc[ISI_ADDRESS_STRING_SZ];
    /* Future fields can be added here. */
} ISIP_System;

/*
 * General diagnostic codes for specific test requirement.
 */
typedef enum {
    ISIP_DIAG_REASON_INVALID = 0,
    ISIP_DIAG_REASON_AUDIO_RECORD,
    ISIP_DIAG_REASON_AUDIO_PLAY,
    ISIP_DIAG_REASON_LAST
} ISIP_DiagReason;

/*
 * Reason for ussd message.
 */
typedef enum {
    ISIP_USSD_REASON_INVALID = 0,
    ISIP_USSD_REASON_SEND,
    ISIP_USSD_REASON_REPLY,
    ISIP_USSD_REASON_REQUEST,
    ISIP_USSD_REASON_NOTIFY,
    ISIP_USSD_REASON_DISCONNECT,
    ISIP_USSD_REASON_SEND_OK,
    ISIP_USSD_REASON_SEND_ERROR,
    ISIP_USSD_REASON_LAST
} ISIP_UssdReason;

typedef struct {
    ISIP_ServiceReason reason;
    ISIP_Status        status;
    ISI_ServerType     server;
    char               reasonDesc[ISI_EVENT_DESC_STRING_SZ + 1];
    union {
        char           country[ISI_COUNTRY_STRING_SZ + 1];
        char           uri[ISI_ADDRESS_STRING_SZ + 1];
        char           instanceId[ISI_INSTANCE_STRING_SZ + 1];
        struct {
            vint       type;
            char       szBsId[ISI_BSID_STRING_SZ + 1];
        } bsId;
        char           capabilities[ISI_PRESENCE_STRING_SZ + 1];
        vint           isEmergency;
        char           imeiUri[ISI_ADDRESS_STRING_SZ + 1];
        struct {
            char       filePath[ISI_FILE_PATH_STRING_SZ + 1];
            char       filePrepend[ISI_ADDRESS_STRING_SZ + 1];
        } file;
        char           server[ISI_LONG_ADDRESS_STRING_SZ + 1];
        int            identityHide;
        struct {
            char username[ISI_ADDRESS_STRING_SZ + 1];
            char password[ISI_ADDRESS_STRING_SZ + 1];
            char realm[ISI_ADDRESS_STRING_SZ + 1];
        } credentials;
        ISIP_Coder coders[ISI_CODER_NUM];
        struct {
            char name[ISI_ADDRESS_STRING_SZ + 1];
            OSAL_NetAddress address;
        } interface;
        struct {
            vint result;
            char resp[ISI_AKA_AUTH_RESP_STRING_SZ];
            vint resLength;
            char auts[ISI_AKA_AUTH_AUTS_STRING_SZ];
            char ck[ISI_AKA_AUTH_CK_STRING_SZ];
            char ik[ISI_AKA_AUTH_IK_STRING_SZ];
        } akaAuthResp;
        struct {
            char rand[ISI_AKA_AUTH_RAND_STRING_SZ];
            char autn[ISI_AKA_AUTH_AUTN_STRING_SZ];
        } akaAuthChallenge;
        struct {
            ISI_PortType portType;
            vint         portNum;
            vint         poolSize;
        } port;
        struct {
            struct {
                vint     protectedPort;
                vint     protectedPortPoolSz;
                vint     spi;
                vint     spiPoolSz;
            } cfg;
            struct {
                vint     portUc; /* UE client port */
                vint     portUs; /* UE server port */
                vint     portPc; /* Proxy client port */
                vint     portPs; /* proxy server port */
                vint     spiUc;  /* SPI for inbound SA to UE client port */
                vint     spiUs;  /* SPI for inbound SA to UE server port */
                vint     spiPc;  /*
                                  * SPI for outbound SA to proxy client
                                  * port.
                                  */
                vint     spiPs;  /*
                                  * SPI for outbound SA to proxy server
                                  * port.
                                  */
            } resp;
        } ipsec;
        char             provisioningData[ISI_PROVISIONING_DATA_STRING_SZ + 1];
    } settings;
} ISIP_Service;

typedef struct {
    int              type;
    char             lclKey[ISI_SECURITY_KEY_STRING_SZ + 1];
    char             rmtKey[ISI_SECURITY_KEY_STRING_SZ + 1];
} ISIP_SecurityKeys;

typedef struct {
    char                 to[ISI_ADDRESS_STRING_SZ + 1];
    char                 from[ISI_ADDRESS_STRING_SZ + 1];
    char                 subject[ISI_SUBJECT_STRING_SZ + 1];
    char                 reasonDesc[ISI_EVENT_DESC_STRING_SZ + 1];
    ISIP_CallReason      reason;
    ISIP_Status          status;
    ISI_Id               transferTargetCallId;
    ISI_Id               serviceId;
    ISI_SessionCidType   cidType;
    /* Fields related to session streams */
    uint16               type;
    ISI_SessionDirection audioDirection;
    ISI_SessionDirection videoDirection;
    /* Bit mask indicating what RTCP-FB msgs should be used for video. */
    uint32               videoRtcpFbMask;
    ISIP_Coder           coders[ISI_CODER_NUM];
    OSAL_NetAddress      lclAudioAddr;
    OSAL_NetAddress      lclVideoAddr;
    uint16               lclAudioCntlPort;
    uint16               lclVideoCntlPort;
    /* Local - Video RTP Session bandwidth in kbps - AS bandwidth parameter. */
    uint32               lclVideoAsBwKbps;
    OSAL_NetAddress      rmtAudioAddr;
    OSAL_NetAddress      rmtVideoAddr;
    uint16               rmtAudioCntlPort;
    uint16               rmtVideoCntlPort;
    uint32               srvccStatus;
    /* Remote party - Video RTP Session bandwidth in kbps - AS bandwidth parameter. */
    uint32               rmtVideoAsBwKbps;
    int                  ringTemplate;
    ISIP_SecurityKeys    audioKeys;
    ISIP_SecurityKeys    videoKeys;
    uint8                rsrcStatus;
    int                  supsrvHfExist;
    char                 historyInfo[ISI_HISTORY_INFO_STRING_SZ+1];
    char                 participants[ISI_ADDRESS_STRING_SZ + 1];
} ISIP_Call;

typedef struct {
    ISIP_TelEvtReason reason;
    char              reasonDesc[ISI_EVENT_DESC_STRING_SZ + 1];
    ISI_Id            serviceId;
    ISI_Id            callId;
    char              to[ISI_ADDRESS_STRING_SZ + 1];
    char              from[ISI_ADDRESS_STRING_SZ + 1];
    char              dateTime[ISI_DATE_TIME_STRING_SZ + 1];
    ISI_TelEvent      evt;
    union {
        struct {
            uint32    arg0;
            uint32    arg1;
            char string[ISI_ADDRESS_STRING_SZ + 1];
        } args;
        struct {
           ISI_FwdCond condition;
           int         enable;
           int         timeout;
        } forward;
        struct {
            ISI_SeviceAttribute cmd;
            char arg1[ISI_SECURITY_KEY_STRING_SZ + 1];
            char arg2[ISI_SECURITY_KEY_STRING_SZ + 1];
        } service;
        struct {
            int         digit;
            int         edgeType;
        } dtmfDetect;
    } settings;
} ISIP_TelEvent;

typedef struct {
    ISIP_TextReason   reason;
    char              reasonDesc[ISI_EVENT_DESC_STRING_SZ + 1];
    char              to[ISI_LONG_ADDRESS_STRING_SZ + 1];
    char              from[ISI_ADDRESS_STRING_SZ + 1];
    char              subject[ISI_SUBJECT_STRING_SZ + 1];
    char              message[ISI_TEXT_STRING_SZ + 1];
    char              messageId[ISI_ID_STRING_SZ + 1];
    char              dateTime[ISI_DATE_TIME_STRING_SZ + 1];
    ISI_MessageReport report;
    ISI_Id            serviceId;
    ISI_Id            chatId;
    ISI_MessageType   type;
    vint              pduLen;
} ISIP_Text;

typedef struct {
    ISI_Id               serviceId;
    ISIP_ChatReason      reason;
    char                 reasonDesc[ISI_EVENT_DESC_STRING_SZ + 1];
    ISIP_Status          status;
    char                 remoteAddress[ISI_ADDRESS_STRING_SZ + 1];
    char                 localAddress[ISI_ADDRESS_STRING_SZ + 1];
    char                 participants[ISI_LONG_ADDRESS_STRING_SZ + 1];
    char                 contributionId[ISI_CONTRIBUTION_ID_STRING_SZ + 1];
    char                 subject[ISI_SUBJECT_STRING_SZ + 1];
    char                 password[ISI_PASSWORD_SZ + 1];
    int                  passwordRequired;
    ISI_SessionType      type;
    /* The below value is used for the first message when initiating the chat. */
    ISIP_Text            text;
} ISIP_Chat;

typedef struct {
    OSAL_NetAddress      rmtAddr;
    uint16               rmtCntlPort;
    OSAL_NetAddress      lclAddr;
    uint16               lclCntlPort;
    /* Local - RTP Session bandwidth in kbps - AS bandwidth parameter. */
    uint32               lclVideoAsBwKbps;
    /* Remote - RTP Session bandwidth in kbps - AS bandwidth parameter. */
    uint32               rmtVideoAsBwKbps;
    char                 rtcpCname[ISI_ADDRESS_STRING_SZ + 1];
    char                 userName[ISI_ADDRESS_STRING_SZ + 1];
    ISIP_SecurityKeys    securityKeys;
    /* Bit mask indicating what RTCP-FB msgs should be used. */
    uint32               rtcpFbMask;
} ISIP_MediaParams;

typedef struct {
    int                  id;
    ISI_SessionDirection audioDirection;
    ISI_SessionDirection videoDirection;
    uint16               type;
    uint32               confMask;
    ISIP_MediaParams     audio;
    ISIP_MediaParams     video;
    ISIP_Coder           coders[ISI_CODER_NUM];
} ISIP_Stream;

typedef struct {
    ISI_AudioTone        toneType;
    /* The rest of these are not currently supported */
    int                  freq1;
    int                  freq2;
    int                  cadences;
    int                  make1;
    int                  break1;
    int                  repeat1;
    int                  make2;
    int                  break2;
    int                  repeat2;
    int                  make3;
    int                  break3;
    int                  repeat3;
} ISIP_Tone;

typedef struct {
    int    ringtemplate;
    /* The rest of these are not currently supported */
    int    cadences;
    int    make1;
    int    break1;
    int    make2;
    int    break2;
    int    make3;
    int    break3;
} ISIP_Ring;

typedef struct {
    int              aStreamId[ISI_CONF_USERS_NUM];
    uint32           aConfMask[ISI_CONF_USERS_NUM];
    ISI_Id           aCall[ISI_CONF_USERS_NUM];
} ISIP_Conf;

typedef struct {
    uint32       lclAddr;
    uint16       lclPort;
    uint32       rmtAddr;
    uint16       rmtPort;
    unsigned int pktSize;
    char         payload[512];
} ISIP_Stun;

typedef struct {
    ISIP_MediaReason reason;
    ISI_Id           serviceId;
    union {
        ISIP_Stream stream;
        ISIP_Ring   ring;
        ISIP_Tone   tone;
        ISIP_Conf   conf;
        ISIP_Stun   stun;
        struct {
            uint32  periodMs;
        } timer;
        uint8       speakerOn;
        uint8       aecOn;
        struct {
            vint    txGain;
            vint    rxGain;
        } gainCtrl;
    } media;
} ISIP_Media;

typedef struct {
    ISIP_PresReason reason;
    char            reasonDesc[ISI_EVENT_DESC_STRING_SZ + 1];
    ISI_Id          serviceId;
    ISI_Id          chatId;
    char            to[ISI_ADDRESS_STRING_SZ + 1];
    char            from[ISI_ADDRESS_STRING_SZ + 1];
    struct {
        char        group[ISI_ADDRESS_STRING_SZ + 1];
        char        name[ISI_ADDRESS_STRING_SZ + 1];
    } roster;
    char            presence[ISI_PRESENCE_STRING_SZ + 1];
} ISIP_Presence;

typedef struct {
    ISIP_FileReason     reason;
    char                reasonDesc[ISI_EVENT_DESC_STRING_SZ + 1];
    ISI_Id              serviceId;
    ISI_Id              chatId;
    char                to[ISI_ADDRESS_STRING_SZ + 1];
    char                from[ISI_ADDRESS_STRING_SZ + 1];
    char                filePath[ISI_FILE_PATH_STRING_SZ + 1];
    char                subject[ISI_SUBJECT_STRING_SZ + 1];
    ISI_FileType        fileType;
    ISI_FileAttribute   fileAttribute;
    vint                fileSize;
    char                fileTransferId[ISI_FILE_TRANSFER_ID_SZ + 1];
    vint                progress;
    ISI_FileReport      report;
} ISIP_File;

typedef struct {
    ISIP_DiagReason   reason;
    char              audioFile[ISI_ADDRESS_STRING_SZ];
    /* Future fields can be added here. */
} ISIP_Diagnostic;

typedef struct {
    ISIP_UssdReason   reason;
    char              reasonDesc[ISI_EVENT_DESC_STRING_SZ + 1];
    char              message[ISI_USSD_STRING_SZ + 1];
    char              to[ISI_LONG_ADDRESS_STRING_SZ + 1];
    ISI_Id            serviceId;
    ISIP_Coder        coders[ISI_CODER_NUM];
} ISIP_Ussd;

/*
 * This is a message exchanged between ISI and protocol user agents.
 * All protocol/audio commands and protocol/audio events are exchanged through
 * this message.
 */
typedef struct {
    ISI_Id        id;       /* This is an ID that underlying protocol uses */
    ISIP_Code     code;     /* Type of message */
    int           protocol; /* The protocol pertaining to this message */
    union {
        ISIP_System     system;
        ISIP_Service    service;
        ISIP_Call       call;
        ISIP_Text       message;
        ISIP_Media      media;
        ISIP_TelEvent   event;
        ISIP_Presence   presence;
        ISIP_Chat       groupchat;
        ISIP_File       file;
        ISIP_Diagnostic diag;
        ISIP_Ussd       ussd;
    } msg;
} ISIP_Message;

#endif
