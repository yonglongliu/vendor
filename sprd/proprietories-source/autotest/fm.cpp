// 
// Spreadtrum Auto Tester
//
// anli   2012-11-29
//
#include <dlfcn.h>
#include <fcntl.h>
#include <string.h>

#include <hardware/fm.h>
//#include <media/AudioSystem.h>
#include <hardware/bluetooth.h>
#include <media/AudioRecord.h>
#include <media/AudioSystem.h>
#include <media/AudioTrack.h>
#include <media/mediarecorder.h>
#include <system/audio.h>
#include <system/audio_policy.h>

#include "type.h"
#include "fm.h"
#include "perm.h"
#include "bt.h"
#ifdef BOARD_HAVE_FM_BCM
#include <hardware/bt_fm.h>
#endif

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//--namespace sci_fm {
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
using namespace android;
using namespace at_perm;
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// enable or disable local debug
#define DBG_ENABLE_DBGMSG
#define DBG_ENABLE_WRNMSG
#define DBG_ENABLE_ERRMSG
#define DBG_ENABLE_INFMSG
#define DBG_ENABLE_FUNINF
#include "debug.h"
#define FM_DEV_NAME "/dev/fm"

/*  seek direction */
#define FM_SEEK_UP 0
#define FM_SEEK_DOWN 1
#define FM_RSSI_MIN 105

static int g_enable_radio_flag = 0;
static unsigned int g_seek_freq =8750;
static  int g_seek_rssi =105;
static int g_seek_status =0;
static int bbat_seek_rssi = 105;
//add bcm fm function
#ifdef BOARD_HAVE_FM_BCM
static const bt_interface_t* sBtInterface = NULL;
static bluetooth_device_t* sBtDevice = NULL;
static const btfm_interface_t* sFmInterface = NULL;
static bt_state_t sBtState = BT_STATE_OFF;
static bt_state_t sFmState = BT_RADIO_OFF;
static int sFmStatus = FM_STATE_PANIC;

static int             sRemoteDevNum = 0;
static bdremote_t      sRemoteDev[MAX_SUPPORT_RMTDEV_NUM];
static int             sInqStatus   = BT_STATUS_INQUIRE_UNK;

static bool set_wake_alarm(uint64_t delay_millis, bool, alarm_cb cb, void *data);
static int acquire_wake_lock(const char *);
static int release_wake_lock(const char *);

//extern void btDeviceFoundCallback(int num_properties, bt_property_t *properties);
//extern void btDiscoveryStateChangedCallback(bt_discovery_state_t state);
//extern void btAdapterStateChangedCallback(bt_state_t state);
#endif
extern int SendAudioTestCmd(const uchar * cmd,int bytes);
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------

static hw_module_t * s_hwModule = NULL;
static fm_device_t * s_hwDev    = NULL;
int fd = -1;
#ifdef BOARD_HAVE_FM_BCM
void btfmEnableCallback (int status)  {  if (status == BT_STATUS_SUCCESS) sFmStatus = FM_STATE_ENABLED; DBGMSG("Enable callback, status: %d", sFmStatus); }
void btfmDisableCallback (int status) { if (status == BT_STATUS_SUCCESS) sFmStatus = FM_STATE_DISABLED;   DBGMSG("Disable callback, status: %d", sFmStatus); }
void btfmtuneCallback (int status, int rssi, int snr, int freq) { DBGMSG("Tune callback, status: %d, freq: %d", status, freq);bbat_seek_rssi=rssi;}
void btfmMuteCallback (int status, BOOLEAN isMute){ DBGMSG("Mute callback, status: %d, isMute: %d", status, isMute); }
void btfmSearchCallback (int status, int rssi, int snr, int freq){ DBGMSG("Search callback, status: %d", status); }
void btfmSearchCompleteCallback(int status, int rssi, int snr, int freq){ DBGMSG("Search complete callback"); g_seek_freq=freq; g_seek_rssi=rssi; g_seek_status =1;}
void btfmAudioModeCallback(int status, int audioMode){ DBGMSG("Audio mode change callback, status: %d, audioMode: %d", status, audioMode); }
void btfmAudioPathCallback(int status, int audioPath){ DBGMSG("Audio path change callback, status: %d, audioPath: %d", status, audioPath); }
void btfmVolumeCallback(int status, int volume){ DBGMSG("Volume change callback, status: %d, volume: %d", status, volume); }
void btDeviceFoundCallback(int num_properties, bt_property_t *properties);
void btDiscoveryStateChangedCallback(bt_discovery_state_t state);
void brcm_btAdapterStateChangedCallback(bt_state_t state);
#endif

struct fm_tune_parm {
    uchar err;
    uchar band;
    uchar space;
    uchar hilo;
    uint freq;
};

struct fm_seek_parm {
    uchar err;
    uchar band;
    uchar space;
    uchar hilo;
    uchar seekdir;
    uchar seekth;
    ushort freq;
};

struct fm_check_status {
    uchar status;
    int rssi;
    uint freq;
};


// add bcm fm function add the struct
#ifdef BOARD_HAVE_FM_BCM
static bt_os_callouts_t stub = {
    sizeof(bt_os_callouts_t),
    set_wake_alarm,
    acquire_wake_lock,
    release_wake_lock,
};

static bt_callbacks_t btCallbacks = {
    sizeof(bt_callbacks_t),
    brcm_btAdapterStateChangedCallback, /*adapter_state_changed */
    NULL, /*adapter_properties_cb */
    NULL, /* remote_device_properties_cb */
    btDeviceFoundCallback, /* device_found_cb */
    btDiscoveryStateChangedCallback, /* discovery_state_changed_cb */
    NULL, /* pin_request_cb  */
    NULL, /* ssp_request_cb  */
    NULL, /*bond_state_changed_cb */
    NULL, /* acl_state_changed_cb */
    NULL, /* thread_evt_cb */
    NULL, /*dut_mode_recv_cb */
    //    NULL, /*authorize_request_cb */
    NULL, /* le_test_mode_cb */
    NULL,
#if defined (SPRD_WCNBT_MARLIN) || defined (SPRD_WCNBT_SR2351)
    NULL,
#endif

};

static btfm_callbacks_t fmCallback = {
    sizeof (btfm_callbacks_t),
    btfmEnableCallback,             // btfm_enable_callback
    btfmDisableCallback,            // btfm_disable_callback
    btfmtuneCallback,               // btfm_tune_callback
    btfmMuteCallback,               // btfm_mute_callback
    btfmSearchCallback,             // btfm_search_callback
    btfmSearchCompleteCallback,     // btfm_search_complete_callback
    NULL,                           // btfm_af_jump_callback
    btfmAudioModeCallback,          // btfm_audio_mode_callback
    btfmAudioPathCallback,          // btfm_audio_path_callback
    NULL,                           // btfm_audio_data_callback
    NULL,                           // btfm_rds_mode_callback
    NULL,                           // btfm_rds_type_callback
    NULL,                           // btfm_deemphasis_callback
    NULL,                           // btfm_scan_step_callback
    NULL,                           // btfm_region_callback
    NULL,                           // btfm_nfl_callback
    btfmVolumeCallback,             //btfm_volume_callback
    NULL,                           // btfm_rds_data_callback
    NULL,                           // btfm_rtp_data_callback
};

#if 0
typedef struct {
    /** set to sizeof(BtFmCallbacks) */
    size_t      size;
    btfm_enable_callback            enable_cb;
    btfm_disable_callback           disable_cb;
    btfm_tune_callback              tune_cb;
    btfm_mute_callback              mute_cb;
    btfm_search_callback            search_cb;
    btfm_search_complete_callback   search_complete_cb;
    btfm_af_jump_callback           af_jump_cb;
    btfm_audio_mode_callback        audio_mode_cb;
    btfm_audio_path_callback        audio_path_cb;
    btfm_audio_data_callback        audio_data_cb;
    btfm_rds_mode_callback          rds_mode_cb;
    btfm_rds_type_callback          rds_type_cb;
    btfm_deemphasis_callback        deemphasis_cb;
    btfm_scan_step_callback         scan_step_cb;
    btfm_region_callback            region_cb;
    btfm_nfl_callback               nfl_cb;
    btfm_volume_callback            volume_cb;
    btfm_rds_data_callback          rds_data_cb;
    btfm_rtp_data_callback          rtp_data_cb;
} btfm_callbacks_t;
#endif
#endif

#define FM_IOC_MAGIC        0xf5
#define FM_IOCTL_POWERUP       _IOWR(FM_IOC_MAGIC, 0, struct fm_tune_parm*)
#define FM_IOCTL_POWERDOWN     _IOWR(FM_IOC_MAGIC, 1, long*)
#define FM_IOCTL_TUNE          _IOWR(FM_IOC_MAGIC, 2, struct fm_tune_parm*)
#define FM_IOCTL_SEEK          _IOWR(FM_IOC_MAGIC, 3, struct fm_seek_parm*)
#define FM_IOCTL_GETRSSI       _IOWR(FM_IOC_MAGIC, 7, long*)
#define FM_IOCTL_CHECK_STATUS  _IOWR(FM_IOC_MAGIC, 71, struct fm_check_status*)

//------------------------------------------------------------------------------
// For 2351
//------------------------------------------------------------------------------
int fmOpen( void )
{
	if( NULL != s_hwDev ) {
		WRNMSG("already opened.\n");
		return 0;
	}

    int err = hw_get_module(FM_HARDWARE_MODULE_ID, (hw_module_t const**)&s_hwModule);
    if( err || NULL == s_hwModule ) {
		ERRMSG("hw_get_module: err = %d\n", err);
		return ((err > 0) ? -err : err);
	}

	err = s_hwModule->methods->open(s_hwModule, FM_HARDWARE_MODULE_ID,
					(hw_device_t**)&s_hwDev);
    if( err || NULL == s_hwDev ) {
		ERRMSG("open err = %d!\n", err);
		return ((err > 0) ? -err : err);
	}

    {
        char  cmd_buf[100] ={0};
        sprintf(cmd_buf, "autotest_fmtest=1");
        SendAudioTestCmd((const uchar*)cmd_buf,sizeof(cmd_buf));
    }
    return 0;
}

//For 2351 chip samsung
#define V4L2_CID_PRIVATE_BASE           0x8000000
#define V4L2_CID_PRIVATE_TAVARUA_STATE  (V4L2_CID_PRIVATE_BASE + 4)

#define V4L2_CTRL_CLASS_USER            0x980000
#define V4L2_CID_BASE                   (V4L2_CTRL_CLASS_USER | 0x900)
#define V4L2_CID_AUDIO_VOLUME           (V4L2_CID_BASE + 5)
#define V4L2_CID_AUDIO_MUTE             (V4L2_CID_BASE + 9)

//------------------------------------------------------------------------------
int fmPlay( uint freq )
{
    if( NULL == s_hwDev ) {
        ERRMSG("not opened!\n");
        return -1;
    }

	status_t   status;
    String8 fm_mute("FM_Volume=0");
    String8 fm_volume("FM_Volume=11");
#if 0
	AudioTrack atrk;

	atrk.set(AUDIO_STREAM_FM, 44100, AUDIO_FORMAT_PCM_16_BIT, AUDIO_CHANNEL_OUT_STEREO);
	atrk.start();

	AudioTrack::Buffer buffer;
	buffer.frameCount = 64;
	status = atrk.obtainBuffer(&buffer, 1);
    if (status == NO_ERROR) {
		memset(buffer.i8, 0, buffer.size);
		atrk.releaseBuffer(&buffer);
	} else if (status != TIMED_OUT && status != WOULD_BLOCK) {
		ERRMSG("cannot write to AudioTrack: status = %d\n", status);
	}
	atrk.stop();
#endif
	int ret = s_hwDev->setControl(s_hwDev, V4L2_CID_PRIVATE_TAVARUA_STATE, 1);
	//DBGMSG("setControl:  TAVARUA = %d\n", ret);
	usleep(20 * 1000);

    uint volume = 12; // max  [0 - 15]
    ret = s_hwDev->setControl(s_hwDev, V4L2_CID_AUDIO_VOLUME, volume);
	//DBGMSG("setControl:  VOLUME = %d\n", ret);

	ret = s_hwDev->setFreq(s_hwDev, freq);
    if( ret < 0 ) {
        ERRMSG("ioctl error: %s\n", strerror(errno));
        return ret;
    }
    {
        char  cmd_buf[100] ={0};
        sprintf(cmd_buf, "autotest_fmtest=2");
        SendAudioTestCmd((const uchar*)cmd_buf,sizeof(cmd_buf));
    }

    return 0;
}

//------------------------------------------------------------------------------
int fmStop( void )
{
    if( NULL != s_hwDev ) {
        char  cmd_buf[100] ={0};
        sprintf(cmd_buf, "autotest_fmtest=0");
        SendAudioTestCmd((const uchar*)cmd_buf,sizeof(cmd_buf));
        s_hwDev->setControl(s_hwDev, V4L2_CID_PRIVATE_TAVARUA_STATE, 0);
    }

    return 0;
}

int fmClose( void )
{
	if( NULL != s_hwDev && NULL != s_hwDev->common.close ) {
		s_hwDev->common.close( &(s_hwDev->common) );
	}
	s_hwDev = NULL;

    if( NULL != s_hwModule ) {
        dlclose(s_hwModule->dso);
		s_hwModule = NULL;
    }

	return 0;
}

#if 0
int fmCheckStatus(uchar *fm_status )
{
    if( NULL == s_hwDev ) {
	    ERRMSG("not opened!\n");
	    return -1;
    }

	int ret = s_hwDev->setControl(s_hwDev, V4L2_CID_PRIVATE_TAVARUA_STATE, 1);
	//DBGMSG("setControl:  TAVARUA = %d\n", ret);
	usleep(20 * 1000);

	ret = s_hwDev->checkStatus(s_hwDev, fm_status);

    if( ret < 0 ) {
        ERRMSG("ioctl error: %s  ret %d  fm_status %d\n", strerror(errno), ret, fm_status);
        return ret;
    }

	return 0;
}
#endif


//for BCM FM function
#ifdef BOARD_HAVE_FM_BCM
static bool set_wake_alarm(uint64_t delay_millis, bool, alarm_cb cb, void *data) {
    /*  saved_callback = cb;
	saved_data = data;

	struct itimerspec wakeup_time;
	memset(&wakeup_time, 0, sizeof(wakeup_time));
	wakeup_time.it_value.tv_sec = (delay_millis / 1000);
	wakeup_time.it_value.tv_nsec = (delay_millis % 1000) * 1000000LL;
	timer_settime(timer, 0, &wakeup_time, NULL);
	return true;
	*/
    static timer_t timer;
    static bool timer_created;

    if (!timer_created) {
	struct sigevent sigevent;
	memset(&sigevent, 0, sizeof(sigevent));
	sigevent.sigev_notify = SIGEV_THREAD;
	sigevent.sigev_notify_function = (void (*)(union sigval))cb;
	sigevent.sigev_value.sival_ptr = data;
	timer_create(CLOCK_MONOTONIC, &sigevent, &timer);
	timer_created = true;
    }

    struct itimerspec new_value;
    new_value.it_value.tv_sec = delay_millis / 1000;
    new_value.it_value.tv_nsec = (delay_millis % 1000) * 1000 * 1000;
    new_value.it_interval.tv_sec = 0;
    new_value.it_interval.tv_nsec = 0;
    timer_settime(timer, 0, &new_value, NULL);

    return true;
}
void brcm_btAdapterStateChangedCallback(bt_state_t state)
{
    g_enable_radio_flag = 1;

    if(state == BT_STATE_OFF || state == BT_STATE_ON)
    {
        sBtState = state;
        DBGMSG("BT Adapter State Changed: %d",state);
    } else if(state == BT_RADIO_OFF || state == BT_RADIO_ON) {
        DBGMSG("FM Adapter State Changed: %d",state);
        if (state != BT_RADIO_ON) return;
            sFmInterface = (btfm_interface_t*)sBtInterface->get_fm_interface();
            int retVal = sFmInterface->init(&fmCallback);
            retVal = sFmInterface->enable(96);
    } else {
         DBGMSG("err State Changed: %d",state);
    }
}
static int acquire_wake_lock(const char *) {
    /*if (!lock_count)
      lock_count = 1;
      */
    return BT_STATUS_SUCCESS;
}

static int release_wake_lock(const char *) {
    /* if (lock_count)
       lock_count = 0;
       */
    return BT_STATUS_SUCCESS;
}


//------------------------------------------------------------------------------
//BT Callbacks END
//------------------------------------------------------------------------------

static  int btHalLoad(void)
{
    int err = 0;

    hw_module_t* module;
    hw_device_t* device;

    INFMSG("Loading HAL lib + extensions");

    err = hw_get_module(BT_HARDWARE_MODULE_ID, (hw_module_t const**)&s_hwModule);
    if (err == 0)
    {
	err = s_hwModule->methods->open(s_hwModule, BT_HARDWARE_MODULE_ID, (hw_device_t**)&s_hwDev);
	if (err == 0) {
	    sBtDevice = (bluetooth_device_t *)s_hwDev;
	    sBtInterface = sBtDevice->get_bluetooth_interface();
	}
    }

    DBGMSG("HAL library loaded (%s)", strerror(err));

    return err;
}

static int btInit(void)
{
    INFMSG("INIT BT ");
    int retVal = (bt_status_t)sBtInterface->init(&btCallbacks);
    DBGMSG("BT init: %d", retVal);
    if((BT_STATUS_SUCCESS == retVal)||(BT_STATUS_DONE == retVal)){
	retVal = (bt_status_t)sBtInterface->set_os_callouts(&stub);
	if((BT_STATUS_SUCCESS == retVal)||(BT_STATUS_DONE == retVal))
	{
	    return (0);
	}
	else
	{
	    return (-1);
	}
    }else{
	return (-1);
    }
}

int Bcm_fmOpen(void)
{

    DBGMSG("Try to open fm \n");
    if (FM_STATE_ENABLED == sFmStatus || FM_STATE_PLAYING == sFmStatus) return 0; // fm has been opened
    if (sBtState == BT_STATE_OFF) {
	if ( btHalLoad() < 0 ) {
	    return -1;
	}
    } else {
	if (NULL == sBtInterface || NULL == sBtDevice) {
	    return -1;
	}
    }

    if ( btInit() < 0 ) {
	return -1;
    }
    g_enable_radio_flag = 0;
    sBtInterface->enableRadio();
   while(g_enable_radio_flag != 1)
		usleep(100);
    DBGMSG("Enable radio okay, try to get fm interface \n");
    {
	char  cmd_buf[100] ={0};
	int fm_audio_volume = 11;
	sprintf(cmd_buf, "autotest_fmtest=1");
	SendAudioTestCmd((const uchar*)cmd_buf,sizeof(cmd_buf));
    }
    WRNMSG("bcmFm open okay \n");
    return 0;
}


int Bcm_fmPlay(uint freq,int *fm_rssi)
{
    int counter = 0;
    DBGMSG("Try to open fm \n");

    while (sFmStatus != FM_STATE_ENABLED && counter++ < 3) sleep(1);

    if (sFmStatus != FM_STATE_ENABLED) {
         ERRMSG("fm service has not enabled, status: %d", sFmStatus);
         return -1;
    }
    DBGMSG("BRCM fm play freq = %d\n",freq);
    sFmInterface->tune(freq * 10);
    sFmInterface->set_audio_path(0x02);
    sFmInterface->set_volume(32);
    {
        char  cmd_buf[100] ={0};
        int fm_audio_volume = 11;
        sprintf(cmd_buf, "autotest_fmtest=2");
        SendAudioTestCmd((const uchar*)cmd_buf,sizeof(cmd_buf));
    }
	sleep(1);
    *fm_rssi = bbat_seek_rssi;
    DBGMSG("Fm play okay fm_rssi = %d \n", bbat_seek_rssi);
    return 0;
}

int Bcm_fmStop(void)
{
    if( NULL != s_hwDev ) {
	char  cmd_buf[100] ={0};
	int fm_audio_volume = 11;
	sprintf(cmd_buf, "autotest_fmtest=0");
	SendAudioTestCmd((const uchar*)cmd_buf,sizeof(cmd_buf));
	sFmStatus = FM_STATE_STOPED;
    }

    DBGMSG("Stop okay \n");
    return 0;
}

int Bcm_fmClose(void)
{
    int counter = 0;

    if (sFmInterface)
	sFmInterface->disable();
    else
	return -1;

    while (counter++ < 3 && FM_STATE_DISABLED != sFmStatus) sleep(1);
    if (FM_STATE_DISABLED != sFmStatus) return -1;

    if (sFmInterface) sFmInterface->cleanup();
    if (sBtInterface) {
	sBtInterface->disableRadio();
    }

    sFmStatus = FM_STATE_DISABLED;

    DBGMSG("Close successful.");

    return 0;
}

#endif

/* Below Radio_ functions is for google fm apk in android 6.0 and upon */

int Radio_Open( uint freq )
{
    int ret = -1;
    struct fm_tune_parm parm = {0};

    fd = open(FM_DEV_NAME, O_RDWR);
    if (fd < 0) {
        ERRMSG("open err! fd = %d\n", fd);
        return -1;
    }

    if ( freq < 875 || freq > 1080 ) {
        ERRMSG("invalid freq! freq = %d\n", freq);
        return -1;
    } else {
        parm.freq = freq;
    }

    ret = ioctl(fd, FM_IOCTL_POWERUP, &parm);
    if (ret) {
        ERRMSG("power up err! ret = %d\n", ret);
        return ret;
    }

    {
        char  cmd_buf[100] ={0};
        sprintf(cmd_buf, "test_stream_route=4");
        SendAudioTestCmd((const uchar*)cmd_buf,sizeof(cmd_buf));
    }

    return ret;
}

int Radio_Play( uint freq )
{
    int ret = -1;
    struct fm_tune_parm parm = {0};
    parm.freq = freq;
    ret = ioctl(fd, FM_IOCTL_TUNE, &parm);
    if (ret) {
        ERRMSG("tune err! freq = %d, ret = %d\n", parm.freq, ret);
        return ret;
    }

    {
        char  cmd_buf[100] ={0};
        sprintf(cmd_buf, "handleFm=1");
        SendAudioTestCmd((const uchar*)cmd_buf,sizeof(cmd_buf));
	usleep(100000);
        sprintf(cmd_buf, "connect=16777216;FM_Volume=11;test_out_stream_route=4");
        SendAudioTestCmd((const uchar*)cmd_buf,sizeof(cmd_buf));
    }

    return ret;
}

int Radio_Tune( uint freq )
{
    int ret = -1;
    struct fm_tune_parm parm = {0};

    parm.freq = freq;
    ret = ioctl(fd, FM_IOCTL_TUNE, &parm);
    if (ret) {
        ERRMSG("tune err! freq = %d, ret = %d\n", parm.freq, ret);
        return ret;
    }

    return ret;
}

int Radio_Seek( uint *freq )
{
    int ret = -1;
    struct fm_seek_parm parm = {0};

    parm.freq = *freq;
    //seekdir = 1 mean seek from 875 to 1080
    parm.seekdir = FM_SEEK_DOWN;
    ret = ioctl(fd, FM_IOCTL_SEEK, &parm);
    if (ret) {
        ERRMSG("tune err! freq = %d, ret = %d\n", parm.freq, ret);
        return ret;
    } else {
        *freq = parm.freq;
        return ret;
    }
}

int Radio_Close( void )
{
    int ret = -1;
    int type = 0;

    {
        char  cmd_buf[100] ={0};
        sprintf(cmd_buf, "handleFm=0");
        SendAudioTestCmd((const uchar*)cmd_buf,sizeof(cmd_buf));
    }

    ret = ioctl(fd, FM_IOCTL_POWERDOWN, &type);
    if (ret) {
        ERRMSG("power down err! ret = %d\n", ret);
        return ret;
    }

    ret = close(fd);
    if (ret) {
        ERRMSG("close err! ret = %d\n", ret);
        return ret;
    }

    return ret;
}

int Radio_CheckStatus(uchar *fm_status, uint *fm_freq, int *fm_rssi)
{
    int ret = -1;
    struct fm_check_status parm = {0};

    if (fd < 0) {
        ERRMSG("open err! fd = %d\n", fd);
        return -1;
    }
    ret = ioctl(fd, FM_IOCTL_CHECK_STATUS, &parm);
    if (ret) {
        ERRMSG("get status err! ret = %d\n", ret);
        return ret;
    }

    *fm_status = parm.status;
    *fm_rssi = parm.rssi;
    *fm_freq = (unsigned int)parm.freq;

    return ret;
}

int Radio_GetRssi(int *fm_rssi)
{
    int ret = -1;

    if (fd < 0) {
        ERRMSG("open err! fd = %d\n", fd);
        return -1;
    }
    ret = ioctl(fd, FM_IOCTL_GETRSSI, fm_rssi);
    if (ret) {
        ERRMSG("get rssi err! ret = %d\n", ret);
        return ret;
    }

    return ret;
}

int Radio_Seek_Channels(uchar *fm_status, uint *start_freq, int *fm_rssi)
{
    int ret = 0, Num = 0;
    struct fm_check_status cur_freq;
    unsigned int LastValidFreq = 0;

    memset(&cur_freq, 0, sizeof(fm_check_status));
    cur_freq.freq = *start_freq;

    while(LastValidFreq < cur_freq.freq) {
        LastValidFreq = cur_freq.freq;
        cur_freq.freq += 1;

        ret = Radio_Seek(&cur_freq.freq);
        LOGI("seek channels, ret: %d, current freq: %d, last freq: %d\n",
            ret, cur_freq.freq, LastValidFreq);
        if (0 != ret || LastValidFreq >= cur_freq.freq) {
            LOGE("seek channels stop: [%d]\n", ret);
            continue;
        }

        Radio_Tune(cur_freq.freq);
#ifndef SPRD_WCNBT_MARLIN //for 2351
        ret = Radio_CheckStatus(&cur_freq.status, &cur_freq.freq, &cur_freq.rssi);
#else //for marlin
        ret = Radio_GetRssi(&cur_freq.rssi);
#endif
        if (!ret) {
            if(cur_freq.rssi <= FM_RSSI_MIN) {
                LOGI("seek channels success: [%d]: freq:%d rssi:%d\n", Num++, cur_freq.freq, cur_freq.rssi);
                cur_freq.status = 0;
                break;
            } else {
                LOGI("Num++: [%d]: freq:%d rssi:%d\n", Num++, cur_freq.freq, cur_freq.rssi);
            }
        }
    }

    *fm_status = cur_freq.status;
    *start_freq = cur_freq.freq;
    *fm_rssi = cur_freq.rssi;

    if ((Num == 0) || (cur_freq.rssi > FM_RSSI_MIN)) {
        *fm_status = 1;
        LOGI("status: %d freq: %d rssi: %d\n", cur_freq.status, cur_freq.freq, cur_freq.rssi);
        return -1;
    }

    return 0;
}

int Radio_SKD_Test(uchar *fm_status, uint *fm_freq, int *fm_rssi)
{
    int ret = 0;

#ifdef BOARD_HAVE_FM_BCM
    Bcm_fmOpen();
    sleep(2);
    sFmInterface->tune(8750);
	sleep(1);
	LOGD("brcm FM tune finish\n");
	g_seek_status = 0;
	sFmInterface->combo_search(8760, 8750, 105,128,1, 0, 0, 0);
	while(g_seek_status !=1)
		usleep(100);
	LOGD("brcm FM seek freq=%d,rssi= %d\n",g_seek_freq,g_seek_rssi);
	*fm_freq = g_seek_freq/10;
	*fm_rssi = g_seek_rssi;
	*fm_status = 0;
       return 0;

#else

//#ifdef GOOGLE_FM_INCLUDED //for android 6 and upon platform Using google original apk
    if( Radio_Open(*fm_freq) < 0 || Radio_Tune(*fm_freq) < 0 ) {
        goto FAIL;
    } else {
        ret = Radio_Seek_Channels(fm_status, fm_freq, fm_rssi);
        if (!ret) {
            ret = Radio_Play(*fm_freq);
            *fm_status = 0;
        } else {
            goto FAIL;
        }
    }
    if( Radio_Close() < 0 ) {
        goto FAIL;
    }
//#else
#if 0
#ifndef SPRD_WCNBT_MARLIN
    if( fmOpen() < 0 || fmPlay(freq) < 0 ) {
        ret = -1;
    }
    fmCheckStatus(&skd_fm_status_r);
    rsp[0]= skd_fm_status_r;
    fmStop();
    fmClose();
#endif
#endif
#endif

    return ret;

FAIL:

    *fm_status = 1;
    return -1;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//--} // namespace
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
