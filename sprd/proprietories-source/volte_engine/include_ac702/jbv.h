/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 5312 $ $Date: 2008-02-18 19:05:41 -0600 (Mon, 18 Feb 2008) $
 */

#ifndef _JBV_H_
#define _JBV_H_

#include <h264.h>
#include <h263.h>

#define LOG_TAG "JBV"

#ifdef VIDEO_DEBUG_LOG
#define JBV_dbgLog(fmt, args...) \
        OSAL_logMsg("%s %d:" fmt, __FUNCTION__, __LINE__, ## args)
#define JBV_wrnLog(fmt, args...) \
        OSAL_logMsg("%s: WARNNING " fmt, __FUNCTION__, ## args)
#define JBV_errLog(fmt, args...) \
        OSAL_logMsg("%s %d: ERROR " fmt, __FUNCTION__, __LINE__, ## args)
#define JBV_InfoLog(fmt, args...) \
        OSAL_logMsg("VIDEO_DEBUG JBV STAT : %s " fmt, __FUNCTION__, ## args)
#else
#define JBV_dbgLog(fmt, args...)
#define JBV_wrnLog(fmt, args...) \
        OSAL_logMsg("%s: WARNNING " fmt, __FUNCTION__, ## args)
#define JBV_errLog(fmt, args...) \
        OSAL_logMsg("%s %d: ERROR " fmt, __FUNCTION__, __LINE__, ## args)

#define JBV_InfoLog(fmt, args...)
#endif

/* Byte reversal function */
#ifdef D2_LITTLE_ENDIAN
#define JBV_htons(x)   \
                        ((((x) & 0xFF00) >> 8)  + \
                        (((x) & 0x00FF) << 8))
#else /* big endian, no reversal */
#define JBV_htons(x)     (x)
#endif /* D2_LITTLE_ENDIAN */

/* Calculate JBV Seqn from RTP seqn. */
#define JBV_SEQN_FROM_RTP_SEQN(x) ((x) & (_JBV_SEQN_MAXDIFF - 1))

/* Calculate Next JBV Seqn given current JBV seqn. */
#define JBV_NEXT_SEQN(x) ((x + 1) & (_JBV_SEQN_MAXDIFF - 1))
/* Calculate Previous JBV Seqn given current JBV seqn. */
#define JBV_PREVIOUS_SEQN(x) ((x - 1) & (_JBV_SEQN_MAXDIFF - 1))

/*
 * This is maximum difference between two seqn in JB.
 */
#define _JBV_SEQN_MAXDIFF    (256)

#define RECV_TS_MAXDIFF      (8)

/* JBV Init level. */
#define JBV_INIT_LEVEL   (0)

/* JBV when in  ACCM state, will accumulate 1 out of X as defined by this constant.*/
#define JBV_ACCM_RATE   (10)

/* JBV Packet loss cache size. */
#define JBV_PACKET_LOSS_CACHE_SIZE (1024)

/* Flag to enable or disable mosic prevention. */
#define JBV_MOSAIC_PREVENTION   (0)

/* video encode clock rate is 90 Kbps*/
#define JBV_VIDEO_CLOCK_RATE_IN_KHZ   (90)

/* mosaic prevent*/
#define JBV_MP_FRAME_INTERVAL_THRESHOLD  (100 * JBV_VIDEO_CLOCK_RATE_IN_KHZ) /*100 ms*/
#define JBV_MP_STATE_PENDDING            (0)
#define JBV_MP_STATE_READY               (1)
#define JBV_MP_PLI_PERIOD_US             (1000000)


/*
 * Assume New Jitter < Current Jitter
 *
 * we calculate the reduced jitter as follows:
 *
 * Reduced Jitter = (New Jitter) + [(Jitter Difference) * .99]
 *
 * .99 can be expressed as (64881) / (2^16)
 */
#define JBV_DECAY_RATE_NUMERATOR   (64881)

/* Init jitter and frame period */
#define JBV_INIT_FRAME_PERIOD_USEC   (66666)         /* 15 fps */
#define JBV_INIT_JITTER_USEC         (2 * JBV_INIT_FRAME_PERIOD_USEC)        /* 2 x JBV_INIT_FRAME_PERIOD_USEC */

/* Max frame period */
#define JBV_FRAME_PERIOD_MAX_USEC    (1000000)       /* 1000 ms i.e. 1fps */
#define JBV_ABNORMAL_PERIOD_USEC     (2000000)       /* >2s, abnormal frame period*/

#define JBV_MAX_JITTER_USEC          (1000000)      /* in usecs */

/* Number of packets to be observed when calculating incoming video payload statistics. */
#define JBV_OBSERVATION_WINDOW       (100)

/*
 * JBV states.
 */
#define JBV_STATE_EMPTY  (0x0000) /* There are no frames in the jitter buffer */
#define JBV_STATE_ACCM   (0x0001) /* There are less frames than desired in the jitter buffer. It should be allowed to grow. */
#define JBV_STATE_NORM   (0x0002) /* There is an appropriate amount of frames in the jitter buffer. */
#define JBV_STATE_LEAK   (0x0004) /* There are more frames than desired in the jitter buffer. It should be allowed to shrink */
#define JBV_STATE_FULL   (0x0008) /* There are the maximum allowable number of frames in the jitter buffer */

/*
 * Lip Sync - Audio Video skew (in milliseconds)
 * Maximum permissible audio video skew when doing Lip Sync.
 */
#define JBV_LIP_SYNC_LEADING_AUDIO_SKEW_MS (45)
#define JBV_LIP_SYNC_LAGGING_AUDIO_SKEW_MS (75)

/* x is audio video skew in milliseconds. Increase/Decrease rate 6.25%% and convert to usec */
#define JBV_LIP_SYNC_FRAME_PERIOD_OFFSET(x) (((x) >> 16) * 1000)

/* JBV coder */
typedef enum {
    JBV_CODER_UNAVAIL = -1,
    JBV_CODER_H264    =  0,
    JBV_CODER_H263    =  1
} JBV_Coder;

typedef struct {
    uint32 sec;
    uint32 usec;
} JBV_Timeval;

typedef struct {
    uint64   ts;                        /* Time Stamp, increments by samples */
    uint32   tsOrig;                    /* The original time stamp from video RTP */
    uint64   atime;                     /* Packet arrival time, local clock */
    int      ptype;                     /* Packet type */
    uint16   seqn;                      /* Sequence number */
    uint16   offset;                    /* Packet size in octets */
    vint     valid;                     /* Indicate if the JBV unit is valid or not. */
    vint     mark;                      /* Mark bit indicating last packet in the sequence */
    uint8    rcsRtpExtnPayload;         /* Rcs 5.1, 1-byte RTP extension payload. */
    uvint    key;                       /* Flag indicating if packet contains IDR slice. */
    uvint    nalu;                      /* NAL Unit type */
    uvint    firstInSeq;                /* Flag indicating if this packet is first in the sequence */
    uint8    data_ptr[VIDEO_MAX_RTP_SZ + 128];
} JBV_Unit;

typedef struct {
    uint16 lostSeqn[JBV_PACKET_LOSS_CACHE_SIZE]; /* Lost packets RTP Seqn numbers. */
    uint16 lostSeqnLength;      /* Number of elements in the lostSeqn array. */
    uint16 lostPacketCount;     /* Total Number of packets lost. */
    vint   lostSinceIdr;        /* Number of frames lost since the last IDR was received. */

    uint16 invalidPacketDrop;   /* Packets that were received but dropped by JBV as they were invalid. */
    uint16 jbvOverflowDrop;     /* Packets that were received but dropped by JBV due to overflow. */
    uint16 tooOldPacketDrop;    /* Packets that were received but dropped by JBV as they were too old. */
    vint   totalPacketDrop;     /* Number of packets that were received but dropped by JBV. */
} JBV_PacketLoss;

typedef struct {
    uint16   counter;           /* Packet counter to keep track of number of packets observed. */
    uint64   firstTs;           /* Arrival time of the first packet in the observation window (usecs) */

    uint32   bytesReceived;     /* Number of video payload bytes received in the observation window */
    uint32   bitrateInKbps;     /* Bitrate in kbps of the video payload received in the observation window */
    uint16   packetRate;        /* Measured incoming packet rate (# packets per second) */
    uint16   lastSeqn;          /* Sequence number of the last packet observed when we calculate the stats. */
} JBV_PacketObserver;

/* A collection of statistics that RTCP reports will read. This
 * will be populated by the jitter buffer. The RTCP utilities
 * must not write to this, except to reset some fields. */
typedef struct {
    /* Total number of key frames that have been received over the course of this call */
    vint               keyFramesRecv;
    /* Flag indicating if a key frame was dropped since last checking the JBV statistics */
    OSAL_Boolean       keyFrameDropped;
    /* Flag indicating if a key frame was read since last checking the JBV statistics */
    OSAL_Boolean       keyFrameRead;
    JBV_PacketLoss     packetLoss;  /* Statistics on lost Video RTP. */
    JBV_PacketObserver observer;   /* Statistics on video media such as bitrate measured by observing incoming packets. */
} JBV_RtcpInfo;

/*
 * this data data struct is used for mosaic prevention
 * */
typedef struct {
    uint16 prevDrawFirstSeqn;  /* first seqn of previous drawn frame */
    uint16 prevDrawLastSeqn;   /* last sqen of previous drawn frame */
    vint frameDropFlag;        /* frame drop flag */
    uint32 rtpTsGapThreshold;  /* interval threshold based on rtp timestamp */
    uint32 prevDrawRtpTs;      /* rtp timestamp of previous drawn frame */
    vint state;                /* state of mosaic prevent */
    vint enable;               /* mosaic prevent enable*/
    uint32 rtpTsDropAccum;     /* the accumulated time of undrawn frame in one intra frame period */
    uint32 rtpTsLatestDrawIdr; /* rtp time stamp of the latest drawn Intra Frame */
    uint32 totalIdr;           /* assume that the sendder transmits one Intra frame per second, it wiil wrap around 136 years later */
    uint64 triggerPliTime;     /* the time trigger PLI rtcp feedback*/
} JBV_MosaicPrevent;


typedef struct {
    uint64          firstUnNormTs;             /* RTP timestamp (unnormalized) of the first packet that was put into JBV. */
    uint64          firstTs;                   /* RTP timestamp (normalized) of the first packet that was put into JBV. This should be 0. */
    uint64          firstAtime;                /* Arrival time of the first packet that was put into JBV. */
    uint64          lastDrawnTs;               /* RTP timestamp of last drawn "frame" from JBV */
    uint64          lastDrawnSeqn;             /* RTP seqn of the last packet of the "frame" that was last drawn from JBV. */
    uint16          lastSeqn;                  /* RTP seqn number of the most recent packet that was put into JBV. */
    uint16          lastMaxSeqn;               /* Maximum RTP seqn number of the most recent packet that was put into JBV. */
    uint64          lastAtime;                 /* Arrival time of the most recent packet that was put into JBV. */
    uint64          lastTs;                    /* RTP timestamp of the most recent packet that was put into JBV. */
    JBV_RtcpInfo    rtcpInfo;                  /* A collection of information that will be read by RTCP utilities */
    uint64          jitter;                    /* Packet delay variation (Refer PDV RFC 5481 Section 4.2). */
    int64           minDelay;                  /*
                                                * The delay of the packet with the lowest value (minimum) for delay
                                                * where delay is measure as (Arrival Time - Normalized RTP Timestamp)
                                                */
    uint64          initTime;                  /* Local time set when Jitter buffer receives the first packet. */
    uint64          framePeriod;               /* Average frame period (average RTP Timestamp diff between two frames) in usec */
    vint            framePeriodOffset;         /*
                                                * Offset (+/-) from the RTP framePeriod at which we should draw from JBV.
                                                * This accounts for jitter and lipsync. Value in usec.
                                                */
    vint            totalFramesReceived;       /* The count of number of frames received by JBV. Used for frame period calculation.  */
    uint64          lastCtime;                 /* Normalized Local time indicating when the last frame was drawn from JBV. */
    vint            drop;                      /* Number of packets that were received but dropped by JBV. */
    vint            ready;                     /* First packet is received (after init). JBV is ready. */
    vint            accmRate;                  /*
                                                *  when JBV is in JBV_STATE_ACCM, 1 out of X packet will not be released, where X is accmRate.
                                                *  For ex:
                                                *  If this is 1 then getPkt will never return a frame when JBV exits ACCM state.
                                                *  If this is 10 then getPkt will not return a frame 1 out of 10 times when JBV exits ACCM state.
                                                */
    vint            accm;
    uint64          level;
    vint            offset;
    uint64          nextDrawTime;               /* Next time to draw a frame */
    uint64          initLevel;                  /* Level to which the buffer will initially be filled, in millisecond */
    uint64          tsLast90K;                  /* Last timestamp received in 90K unit */
    vint            tsOvfl;                     /* ts overflow count */
    vint            state;                      /* JBV current state */
    uint16          eSeqn;                      /* The first seqn of next expect frame to draw */
    uint16          firstSeqnNextFrame;         /* First seqn of next frame to draw */
    uint16          lastSeqnNextFrame;          /* Last seqn of next frame to draw */
    vint            dropTillNextKey;
    vint            eMscPrvt;                   /* _STALE_ usage: Mosaic prevention enable flag */
    uint32          tsLatestIdr;                /* RTP time stamp of the latest Intra frame in the JB */
    JBV_MosaicPrevent mp;                       /* replace eMscPrvt to get enhanced implementation */
    uint8           data[VIDEO_NET_BUFFER_SZ];  /* Buffer for holding reassembled H264 Frame. */
    JBV_Unit        unit[_JBV_SEQN_MAXDIFF];    /* THE JITTER BUFFER. Array of JBV_unit. */

    uint64          recvFrameTs[RECV_TS_MAXDIFF]; /*Buffer for saving ts of frames*/
    uint32          numTs;                        /*number of frame in diferent timestamp*/
    uint64          maxTs;                        /*Maximum frame timestamp in a certain of time*/
} JBV_Obj;

typedef struct {
    uint32    tsOrig;           /* The original time stamp from video RTP */
    vint      valid;            /* Valid flag */
    uint64    atime;            /* Packet arrival time, local clock (usecs) */
    JBV_Coder type;             /* Packet type */
    uint16    seqn;             /* array index in jitter buffer */
    uint32    pSize;            /* Packet size in octets */
    vint      mark;
    uint8     rcsRtpExtnPayload;/* Rcs 5.1, 1-byte RTP extension payload. */
    uint8    *data_ptr;

    uint16    firstSeqn;         /* Sequence number of the first pkt in next frame */
    vint      lastSeqn;          /* Sequence number of the last pkt in next frame */
    uint32    naluBitMask;       /* bit mask to mark the nalu type in the frame */
} JBV_Pkt;

/*
 * Prototypes
 */

int JBV_init(
    JBV_Obj    *obj_ptr);

void JBV_putPkt(
    JBV_Obj     *obj_ptr,
    JBV_Pkt     *pkt_ptr,
    JBV_Timeval *tv_ptr);

void JBV_getPkt(
    JBV_Obj     *obj_ptr,
    JBV_Pkt     *pkt_ptr,
    JBV_Timeval *tv_ptr);

void JBV_fpsAdjust(
    JBV_Obj    *obj_ptr,
    vint       skew);

void JBV_getRtcpInfo(
    JBV_Obj         *obj_ptr,
    JBV_RtcpInfo    *dest_ptr);

#endif
