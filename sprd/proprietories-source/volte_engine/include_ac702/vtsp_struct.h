/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29711 $ $Date: 2014-11-06 12:42:22 +0800 (Thu, 06 Nov 2014) $
 *
 */


#ifndef _VTSP_STRUCT_H_
#define _VTSP_STRUCT_H_

#include "osal.h"

typedef void *VTSP_Context;

typedef struct {
    uint8   _internal[276];
} VTSP_CIDData;

typedef struct {
    uvint volume;
    uvint duration;
    uvint endPackets;
    uvint eventNum;
} VTSP_Rfc4733Event;

typedef struct {
    uvint volume;
    uvint duration;
    uvint endPackets;
    uvint modFreq;
    uvint freq1;
    uvint freq2;
    uvint freq3;
    uvint freq4;
    uvint divMod3;
} VTSP_Rfc4733Tone;

typedef struct { 
    OSAL_NetAddress localAddr;
    OSAL_NetAddress remoteAddr;
    uvint   infc;
    uvint   streamId;
    uvint   pktSize;
    char    payload[VTSP_STUN_PAYLOAD_SZ]; /* Bug 2684 comment 2 */
} VTSP_Stun;

typedef struct { 
    vint    profileId;
    uvint   major;
    uvint   minor;
    char    string_ptr[80];
} VTSP_ProfileHeader;

typedef struct {
    vint                    vtspAddStackSize;
    vint                    rtcpAddStackSize;
    uint16                  rtcpInternalPort;
} VTSP_TaskConfig;

typedef struct { 
    vint    templateCode;
    uint32  templateSize;
    vint    templateId;
} VTSP_TemplateHeader;

typedef struct { 
    struct { 
        uvint   types;
        uvint   silenceComp;
        uvint   dtmfRelay;
        uvint   pktRate;
        uvint   bitRate;
        uvint   extension;
    } encoder;
    struct { 
        uvint   types;
        uvint   silenceComp;
        uvint   dtmfRelay;
        uvint   rate;
        uvint   extension;
    } decoder;
    struct { 
        uvint   numStreams;
        uvint   numPerInfc;
        uvint   srtpEnabled;
    } stream;
    struct { 
        uvint   types;
    } detect;
    struct { 
        uvint   coder;
    } faxModem;
    struct { 
        uvint   numFxs;
        uvint   numFxo;
        uvint   numAudio;
        uvint   numInfc;
        vint    fxsFirst;
        vint    fxsLast;
        vint    fxoFirst;
        vint    fxoLast;
        vint    audioFirst;
        vint    audioLast;
        uvint   fxoFlashTime;
        uvint   relay;
        uvint   numTemplateIds;
    } hw;
    struct {
        uvint   numTemplateIds;
        uvint   maxSequenceLen;
    } tone;
    struct {
        uvint   numTemplateIds;
    } ring;
    struct {
        uvint payloadMaxSz;
        uvint keyMask;
    } flow;
} VTSP_QueryData;


typedef struct {
    uvint   cadences;
    uvint   make1;
    uvint   break1;
    uvint   make2;
    uvint   break2;
    uvint   make3;
    uvint   break3;
    uvint   cidBreakNum;
} VTSP_RingTemplate;


typedef struct {
    vint    freq1;
    vint    freq2;
    vint    power1;
    vint    power2;
    vint    cadences;
    vint    make1;
    vint    break1;
    vint    repeat1;
    vint    make2;
    vint    break2;
    vint    repeat2;
    vint    make3;
    vint    break3;
    vint    repeat3;
} VTSP_ToneTemplate;

typedef struct {
    VTSP_TemplateControlUtd  control;
    VTSP_TemplateUtdMask     mask;
    VTSP_TemplateUtdToneType type;
    union { 
        struct { 
            VTSP_TemplateUtdToneControlDual control;
            vint                            cadences;
            vint                            freq1;
            vint                            freqDev1;
            vint                            freq2;
            vint                            freqDev2;
            vint                            minMake1;
            vint                            maxMake1;
            vint                            minBreak1;
            vint                            maxBreak1;
            vint                            minMake2;
            vint                            maxMake2;
            vint                            minBreak2;
            vint                            maxBreak2;
            vint                            minMake3;
            vint                            maxMake3;
            vint                            minBreak3;
            vint                            maxBreak3;
            vint                            power;
        } dual;
        struct {
            VTSP_TemplateUtdToneControlSIT control;
            vint                           freq1;
            vint                           freqDev1;
            vint                           freq2;
            vint                           freqDev2;
            vint                           freq3;
            vint                           freqDev3;
            vint                           freq4;
            vint                           freqDev4;
            vint                           freq5;
            vint                           freqDev5;
            vint                           shortMin;
            vint                           shortMax;
            vint                           longMin;
            vint                           longMax;
            vint                           power;
        } sit;
    } u;
} VTSP_UtdTemplate;

typedef struct { 
    vint    control;
    union {
        struct {
            uvint  freq1;
            uvint  freq2;
            uvint  freq3;
            uvint  freq4;
            vint   power1;
            vint   power2;
            vint   power3;
            vint   power4;
        } quad;
        struct {
            vint   carrier;
            vint   signal;
            vint   power;
            vint   index;
        } modulate;
    } tone;
    vint    cadences;
    vint    make1;
    vint    break1;
    vint    repeat1;
    vint    make2;
    vint    break2;
    vint    repeat2;
    vint    make3;
    vint    break3;
    vint    repeat3;
    vint    delta;
    vint    decay;
} VTSP_ToneQuadTemplate;

typedef struct {
    VTSP_TemplateCidControl control;
    VTSP_TemplateCidFormat  format;
    uvint                   mdidTimeout; 
    vint                    fskPower;
    vint                    sasPower;
    vint                    casPower;
} VTSP_CidTemplate;

typedef struct { 
    uvint   powerLo;
    uvint   powerHi;
    uvint   mak;
    uvint   brk;
    uvint   pauseLen;
} VTSP_DtmfDigitTemplate;

typedef struct { 
    VTSP_TemplateControlEchoCanc control;
    uvint                        silenceTime;
    uvint                        faxSilTxDB;
    uvint                        faxSilRxDB;
    uvint                        tailLength;
} VTSP_EchoCancTemplate;

typedef struct { 
    VTSP_TemplateControlAec control;
    vint                    tailLen;
} VTSP_AecTemplate;

typedef struct { 
    VTSP_TemplateControlCn  control;
    vint                    cnPwrAttenDb;
} VTSP_CnTemplate;

typedef struct { 
    VTSP_TemplateControlFr38         control;
    VTSP_TemplateControlFr38RateMax  maxRate;
    VTSP_TemplateControlFr38RateMgmt rateMgmt;
    VTSP_TemplateControlFr38EcmMode  ecmMode;
    VTSP_TemplateControlFr38EccMode  eccMode;
    vint                             numEccT30;
    vint                             numEccData;
    vint                             cedLen;
    vint                             maxJitter;
} VTSP_Fr38Template;

typedef struct { 
    VTSP_TemplateControlJb  control;
    uvint                   streamId;
    uvint                   maxLevel;
    uvint                   initLevel;
} VTSP_JbTemplate;

typedef struct { 
    VTSP_TemplateControlRtcp control;
    uvint                    streamId;
    uvint                    mask;
    uvint                    tos;
    uint16                   interval;
} VTSP_RtcpTemplate;

typedef struct { 
    VTSP_TemplateControlRtp  control;
    uvint                    streamId;
    uvint                    tos;
    vint                     randomEnable;
    vint                     rdnLevel;
    vint                     rdnHop;
} VTSP_RtpTemplate;

typedef struct { 
    VTSP_TemplateControlFmtd  control;
    vint                      powerMin;
    vint                      detectMask;
} VTSP_FmtdTemplate;

typedef struct { 
    VTSP_TemplateControlTic control;
    vint                    flashTimeMin;
    vint                    flashTimeMax;
    vint                    releaseTimeMin;
    vint                    pddMakeMin;
    vint                    pddMakeMax;
    vint                    pddBreakMin;
    vint                    pddBreakMax;
    vint                    pddInterDigitMin;
} VTSP_TicTemplate;


typedef struct { 
    VTSP_TemplateControlDtmf control;
    vint                     dtmfPower;
    uvint                    dtmfDur;
    uvint                    dtmfSil;
    uvint                    dtmfHiTwist;
    uvint                    dtmfLoTwist;
    uvint                    dtmfDev;
    uvint                    dtmfFrames;
} VTSP_DtmfTemplate;

typedef struct {
    VTSP_TemplateControlDebug control;
    vint                      debugEnable;
    uint32                    debugRemoteIP;
} VTSP_DebugTemplate;

typedef struct {
    char    string_ptr[60];
    char    type;
    uint32  major;
    uint32  minor;
    uint32  point;
} VTSP_Version;

typedef struct { 
    VTSP_EventMsgCodes  code;
    uvint               infc;
    uvint               handset;
    uint32              tick;
    union { 
        struct { 
            uvint   numFxs;
            uvint   numFxo;
            uvint   numAudio;
            uvint   numInfc;
            uvint   fxsFirst;
            uvint   fxsLast;
            uvint   fxoFirst;
            uvint   fxoLast;
            uvint   audioFirst;
            uvint   audioLast;
        } hwConfig;
        struct { 
            uvint   reason;
            uvint   ringNum;     
            uvint   edgeType;   
        } ringDetect;
        struct { 
            uvint   reason;
            uvint   ringNum;
            uvint   edgeType;
        } ringGenerate;
        struct { 
            uvint   reason;
            uvint   edgeType;
            uvint   digit;
        } hook;
        struct { 
            uvint   streamId;
            uvint   reason;
            uvint   tone;
            uvint   edgeType;
            uvint   detect;
            uvint   early;
            uvint   direction;
        } toneDetect;
        struct { 
            uvint   key;
        } keyDetect;
        struct { 
            uvint   reason;
            uvint   type;
            uvint   direction;
            uvint   streamId;
        } toneGenerate;
        struct { 
            uvint       reason;
            uvint       edgeType;
            uvint       index;
        } digitGenerate;
        struct { 
            uvint   reason;
            uint32  period;
            uint32  tickAvg;
            uint32  tickHi;
            uint32  dbRx;
            uint32  dbTx;
            uint32  bitrate;
        } time;
        struct { 
            uvint   reason;
            uvint   type;
        } cidGenerate;
        struct { 
            uvint   reason;
            uvint   type;
        } cidDetect;
        struct { 
            uvint   reason;
            uvint   edgeType;
            uvint   type;
        } polarity;
        struct { 
            uvint   reason;
            uint32  status;
        } error;
        struct {
            uvint   reason;
            uvint   flowId;
            uvint   key;
        } flow;
        struct { 
            uvint   reason;
            uint32  status;
        } shutdown;
        struct { 
            uvint   reason;
            uvint   streamId;
            uint32  ssrc;
            uint32  arg1;
            uint32  arg2;
            uint32  arg3;
            uint32  arg4;
            uint32  arg5;
            uint32  arg6;
        } rtcp;
        struct { 
            uvint   reason;
            uint32  status;
            uint32  streamId;
            uint32  timestamp;
            uint32  ssrc;
            uint32  arg1;
            uint32  arg2;
            uint32  arg3;
            uint32  arg4;
            uint32  arg5;
            uint32  arg6;
            uint32  arg7;
            uint32  arg8;
            uint32  arg9;
        } rtp;
        struct { 
            uvint   reason;
            uvint   streamId;
            uint32  level;
            uint32  drop;
            uint32  plc;
            uint32  jbSize;
            uint32  leaks;
            uint32  accms;
            uint32  lastTimeStamp;
            uint32  lastTimeArrival;
        } jb;
        struct { 
            uvint   reason;
            uvint   streamId;
            uvint   state;
            uvint   pages;
            uvint   trainDown;
            uvint   evt1;
            uvint   evt2;
        } t38;
        struct { 
            uint32  arg1;
            uint32  arg2;
            uint32  arg3;
            uint32  arg4;
        } debug;
        struct { 
            uint32  timeStampArrival;
            uint32  timeStampDecoding;
        } sync;
        struct { 
            uint32  reason;
            uint32  status;
            uint32  rerl;
        } echoCanc;
        struct { 
            uint32  vtspr;
            uint32  g729Encode0;
            uint32  g729Decode0;
            uint32  g729Encode1;
            uint32  g729Decode1;
            uint32  g711p1Encode0;
            uint32  g711p1Decode0;
            uint32  g711p1Encode1;
            uint32  g711p1Decode1;
            uint32  ilbcEncode0;
            uint32  ilbcDecode0;
            uint32  ilbcEncode1;
            uint32  ilbcDecode1;  
            uint32  g722p1Encode0;
            uint32  g722p1Decode0;
            uint32  g722p1Encode1;
            uint32  g722p1Decode1;  
            uint32  g723Encode0;
            uint32  g723Decode0;
            uint32  g723Encode1;
            uint32  g723Decode1;            
            uint32  g722Encode0;
            uint32  g722Decode0;
            uint32  g722Encode1;
            uint32  g722Decode1;
            uint32  g726Encode0;
            uint32  g726Decode0;
            uint32  g726Encode1;
            uint32  g726Decode1;
            uint32  gamrnbEncode0;
            uint32  gamrnbDecode0;
            uint32  gamrnbEncode1;
            uint32  gamrnbDecode1;
            uint32  gamrwbEncode0;
            uint32  gamrwbDecode0;
            uint32  gamrwbEncode1;
            uint32  gamrwbDecode1;
            uint32  aecComputeRout;
            uint32  aecComputeSout;
            uint32  ecsr;
            uint32  fmtd;
            uint32  dtmf;
            uint32  t38;
            uint32  tic;
            uint32  nfe;
            uint32  dcrmNear;
            uint32  dcrmPeer;
            uint32  cids;
            uint32  jbPut;
            uint32  jbGet;
            uint32  events;
            uint32  cmd;
            uint32  rtpXmit;
            uint32  rtpRecv;
            uint32  total;
        } benchmark;
        struct { 
            uvint   reason;
            uvint   streamId;
        } resource;
        struct { 
            uvint   reason;
            uvint   status;
        } gr909;
        struct {
            uvint   reason;
        } hw;
        struct {
            uvint   streamId;
            vint    width;
            vint    height;
        } dim;
        struct {
            uvint   reason;
            uvint   streamId;
            uint32  rtcpNtp; /* RTCP SR - NTP time in milliseconds. */
            uint32  rtcpRtp; /* RTCP SR - RTP timestamp in milliseconds. */
            uint32  rtpTs;   /* RTP timestamp in milliseconds. */
        } syncEngine;
    } msg;
} VTSP_EventMsg;

typedef struct {
    uint16  infc;         
    uint16  streamId;     
    uint32  dynamicCoder;
    uint32  localCoder;
    uint32  extension;
} VTSP_BlockHeader;

/* NTP Timestamps are a fixed-point fraction of seconds since 00:00 UTC Jan 1, 1900. The fixed
 * point fraction has units of seconds in the Most Significant Word side of the fixed-point. This
 * fraction is often represented as either a 64 bit or 32 bit. This 32 bit representation has
 * the fixed point 16 bits in. This means the last 16 bits are in units of 2^-16 seconds. */
typedef uint32  VTSP_NtpTime;

typedef struct { 
    vint            streamId;
    VTSP_StreamDir  dir;
    uint16          peer;
    vint            encoder;
    vint            encodeTime[VTSP_ENCODER_NUM];
    vint            encodeType[VTSP_ENCODER_NUM];
    vint            decodeType[VTSP_DECODER_NUM];
    uint32          extension;
    uint32          dtmfRelay;
    uint32          silenceComp;
    uint32          confMask;
    OSAL_NetAddress remoteAddr;
    uint16          remoteControlPort; 
    OSAL_NetAddress localAddr;
    uint16          localControlPort; 
    uint8           rdnDynType;
    uint16          srtpSecurityType;
    char            srtpSendKey[VTSP_SRTP_KEY_STRING_MAX_LEN];
    char            srtpRecvKey[VTSP_SRTP_KEY_STRING_MAX_LEN];
} VTSP_Stream;

typedef struct { 
    vint            streamId;
    VTSP_StreamDir  dir;
    uint16          peer;
    vint            encoder;
    vint            encodeTime[VTSP_ENCODER_VIDEO_NUM];
    vint            encodeMaxBps[VTSP_ENCODER_VIDEO_NUM];
    vint            encodePacketMode[VTSP_ENCODER_VIDEO_NUM];
    vint            encodeProfileLevelId[VTSP_ENCODER_VIDEO_NUM];
    vint            encodeType[VTSP_ENCODER_VIDEO_NUM];
    vint            decodeType[VTSP_DECODER_VIDEO_NUM];
    uint32          extension;
    vint            extmapId;
    char            extmapUri[VTSP_EXTMAP_URI_SZ];
    uint32          confMask;
    uint8           rdnDynType;
    OSAL_NetAddress remoteAddr;
    uint16          remoteControlPort; 
    OSAL_NetAddress localAddr;
    uint16          localControlPort; 
    /* Local - Video RTP Session bandwidth in kbps - AS bandwidth parameter. */
    uint32          localVideoAsBwKbps;
    /* Remote - Video RTP Session bandwidth in kbps - AS bandwidth parameter. */
    uint32          remoteVideoAsBwKbps;
    uint16          srtpSecurityType;
    char            srtpSendKey[VTSP_SRTP_KEY_STRING_MAX_LEN];
    char            srtpRecvKey[VTSP_SRTP_KEY_STRING_MAX_LEN];
} VTSP_StreamVideo;

#endif

