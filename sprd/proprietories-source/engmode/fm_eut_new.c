#ifndef _FM_EUT
#define _FM_EUT

#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <string.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <errno.h>
#include <utils/Log.h>

#include "fm_eut_new.h"
#include "eng_diag.h"

#ifdef __cplusplus
extern "C" {
#endif
#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG 	"ENGPC"

#define DBG_ENABLE_DBGMSG
#define DBG_ENABLE_WRNMSG
#define DBG_ENABLE_ERRMSG
#define DBG_ENABLE_INFMSG
#define DBG_ENABLE_FUNINF
#define FM_DEV_NAME "/dev/fm"
#define AUDCTL "/dev/pipe/mmi.audio.ctrl"
#define AUDCTL_NEW "/data/local/media/mmi.audio.ctrl"

/*  seek direction */
#define FM_SEEK_UP 1
#define FM_SEEK_DOWN 0
#define FM_RSSI_MIN 105

#define FM_CMD_STATE 0x00  //open/close fm
#define FM_CMD_VOLUME 0x01  //set FM volume
#define FM_CMD_MUTE   0x02   // set FM mute
#define FM_CMD_TUNE  0x03   //get a single frequency information
#define FM_CMD_SEEK   0x04  //
#define FM_CMD_READ_REG  0x05
#define FM_CMD_WRITE_REG  0x06
#define FM_SUCCESS 0
#define FM_FAILURE  1
#define FM_OPEN 1
#define FM_CLOSE 0

enum fmStatus {
	FM_STATE_DISABLED,
	FM_STATE_ENABLED,
	FM_STATE_PLAYING,
	FM_STATE_STOPED,
	FM_STATE_PANIC,
	FM_STATE_ERR,
};

int fd = -1;

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

typedef struct _FM_SIGNAL_PARAM_T
{
	unsigned char  nOperInd;	// 	Tune: 	0: tune successful;
								//			1: tune failure
								//	Seek: 	0: seek out valid channel successful
								//			1: seek out valid channel failure
	unsigned char  nStereoInd;			// 	0: Stereo; Other: Mono;
	unsigned short 	nRssi;  			// 	RSSI Value
	unsigned int	nFreqValue; 		// 	Frequency, Unit:KHz
	unsigned int	nPwrIndicator; 	    // 	Power indicator
	unsigned int	nFreqOffset; 		// 	Frequency offset
	unsigned int	nPilotDet; 			// 	pilot_det
	unsigned int	nNoDacLpf; 			// 	no_dac_lpf
} __attribute__((PACKED))FM_SIGNAL_PARAM_T;

typedef struct
{
	unsigned char 	nErrorCode;	// DIAG_FM_REG_ERR_E
	unsigned int 	nStartAddr;
	unsigned int 	nUintCount;
} FM_RW_REG_T;

static int mFmTune = 87500;
static FM_SIGNAL_PARAM_T mFmSignalParm;

int SendAudioTestCmd(const uchar * cmd,int bytes)
{
	int pipe_fd = -1;
	int ret=-1;
	int bytes_to_read = bytes;

	if(cmd==NULL){
		return -1;
	}
	ALOGD("SendAudioTestCmd: %s\n", cmd);
	pipe_fd = open(AUDCTL, O_WRONLY | O_NONBLOCK);
	if (pipe_fd < 0) {
		ALOGE("open %s err! fd=%d, errno=%d, %s\n", AUDCTL, pipe_fd, errno, strerror(errno));
		ALOGD("open %s\n", AUDCTL_NEW);
		pipe_fd = open(AUDCTL_NEW, O_WRONLY | O_NONBLOCK);
		if(pipe_fd < 0) {
			ALOGE("open err! fd=%d, errno=%d, %s\n", pipe_fd, errno, strerror(errno));
			return FM_FAILURE;
		}
	}

	do {
		ret = write(pipe_fd, cmd, bytes);
		if( ret > 0) {
			if(ret <= bytes)
				bytes -= ret;
		}
		else if((!((errno == EAGAIN) || (errno == EINTR))) || (0 == ret)) {
			ALOGE("pipe write error %d,bytes read is %d",errno,bytes_to_read - bytes);
			break;
		}
		else
			ALOGW("pipe_write_warning: %d,ret is %d",errno,ret);
	} while(bytes);

	close(pipe_fd);

	if (bytes == bytes_to_read)
		return ret ;
	else
 		return (bytes_to_read - bytes);
}

/* Below Radio_ functions is for google fm apk in android 6.0 and upon */
int Radio_Open( uint freq )
{
	int ret = 0;
	struct fm_tune_parm parm = {0};

	//marlin para: 875, not 87500
	freq = freq/100;

	fd = open(FM_DEV_NAME, O_RDWR);
	if (fd < 0) {
		ALOGE("open err! fd=%d, errno=%d, %s\n", fd, errno, strerror(errno));
		return FM_FAILURE;
	}

	if ( freq < 875 || freq > 1080 ) {
		ALOGE("invalid freq! freq = %d\n", freq);
 		return FM_FAILURE;
	} else {
		parm.freq = freq;
	}

	// use default parameters.
	parm.band = 1;
	parm.hilo = 0;
	parm.space = 1;

	ret = ioctl(fd, FM_IOCTL_POWERUP, &parm);
	if (ret) {
		ALOGE("power up err! ret = %d\n", ret);
		return ret;
	}

	{
		char  cmd_buf[100] ={0};
		sprintf(cmd_buf, "test_stream_route=8");//set audio output device, 8 is headset
		SendAudioTestCmd((const uchar*)cmd_buf,sizeof(cmd_buf));
	}

	return ret;
}

int Radio_Play( uint freq )
{
	int ret = 0;
	struct fm_tune_parm parm = {0};

	// use default parameters.
	parm.band = 1;
	parm.hilo = 0;
	parm.space = 1;

	//marlin para: 875, not 87500
	freq = freq/100;
	parm.freq = freq;
	ALOGI("Radio_Play, freq: %d", parm.freq);
	ret = ioctl(fd, FM_IOCTL_TUNE, &parm);
	if (ret) {
		ALOGE("tune err! freq = %d, ret = %d\n", parm.freq, ret);
		return FM_FAILURE;
	}

	{
		char  cmd_buf[100] ={0};
		sprintf(cmd_buf, "handleFm=1");
		SendAudioTestCmd((const uchar*)cmd_buf,sizeof(cmd_buf));
		sprintf(cmd_buf, "connect=16777216;FM_Volume=7");
		SendAudioTestCmd((const uchar*)cmd_buf,sizeof(cmd_buf));
	}

	return ret;
}

int Radio_Tune( uint freq )
{
	int ret = 0;
	struct fm_tune_parm parm = {0};

	//marlin para: 875, not 87500
	freq = freq/100;
	parm.freq = freq;
	ret = ioctl(fd, FM_IOCTL_TUNE, &parm);
	if (ret) {
		ALOGE("tune err! freq = %d, ret = %d\n", parm.freq, ret);
		return FM_FAILURE;
	}

	return ret;
}

int Radio_Seek( uint *freq, uint dir )
{
	int ret = 0;
	struct fm_seek_parm parm = {0};

	// use default parameters.
	parm.band = 1;
	parm.hilo = 0;
	parm.space = 1;
	//marlin para: 875, not 87500
	parm.freq = (*freq)/100;
	//seekdir = 1 mean seek from 875 to 1080
	parm.seekdir = dir;
	ret = ioctl(fd, FM_IOCTL_SEEK, &parm);
	if (ret) {
		ALOGE("seek err! freq = %d, ret = %d\n", parm.freq, ret);
		return FM_FAILURE;
	} else {
		*freq = parm.freq;
		return ret;
	}
}

int Radio_Close( void )
{
	int ret = 0;
	int type = 0;

	{
		char  cmd_buf[100] ={0};
		sprintf(cmd_buf, "handleFm=0");
		SendAudioTestCmd((const uchar*)cmd_buf,sizeof(cmd_buf));
	}

	ret = ioctl(fd, FM_IOCTL_POWERDOWN, &type);
	if (ret) {
		ALOGE("power down err! ret = %d\n", ret);
		return FM_FAILURE;
	}

	ret = close(fd);
	if (ret) {
		ALOGE("close err! ret = %d\n", ret);
		return FM_FAILURE;
	}

	return ret;
}

int Radio_CheckStatus(uchar *fm_status, uint *fm_freq, int *fm_rssi)
{
	int ret = 0;
	struct fm_check_status parm = {0};

	if (fd < 0) {
		ALOGE("open err! fd = %d\n", fd);
		return FM_FAILURE;
	}
	ret = ioctl(fd, FM_IOCTL_CHECK_STATUS, &parm);
	if (ret) {
		ALOGE("get status err! ret = %d\n", ret);
		return FM_FAILURE;
	}

	*fm_status = parm.status;
	*fm_rssi = parm.rssi;
	*fm_freq = (unsigned int)parm.freq;

	return ret;
}

int Radio_GetRssi(unsigned int *fm_rssi)
{
	int ret = 0;

	if (fd < 0) {
		ALOGE("open err! fd = %d\n", fd);
		return -1;
	}
	ret = ioctl(fd, FM_IOCTL_GETRSSI, fm_rssi);
	if (ret) {
		ALOGE("get rssi err! ret = %d\n", ret);
		return FM_FAILURE;
	}

	return ret;
}

int Radio_Setvol(unsigned char vol)
{
	int ret = 0;

	ret = ioctl(fd, FM_IOCTL_SETVOL, &vol);
	if (ret) {
		ALOGE("set vol err! vol = %d, ret = %d\n", vol, ret);
		return FM_FAILURE;
	}

	return ret;
}

int fm_set_volume(unsigned char vol)
{
	int ret = 0;

	ALOGE("fm_set_volume, vol: %d", vol);
	//ret = Radio_Setvol(vol);
	{
		char  cmd_buf[100] ={0};
		sprintf(cmd_buf, "FM_Volume=%d", vol);
		SendAudioTestCmd((const uchar*)cmd_buf,sizeof(cmd_buf));
	}
	return ret;
}

int fm_set_mute()
{
	int ret = 0;

	ALOGI("fm_set_mute");
	ret = fm_set_volume(0);
	return ret;
}

int fm_read_reg(unsigned int addr, unsigned int *reg_val)
{
	int ret =0;
	struct fm_reg_ctl_parm parm;

	if(addr%4) {
		ALOGE("fm_read_reg the addr is illegal %d" ,addr);
		return FM_FAILURE;
	}
	parm.rw_flag = 1;
	parm.addr = addr;
	parm.err = 0;
	parm.val = 0;
	ALOGI("FM read register addr: 0x%x!\n", parm.addr);
	ret = ioctl(fd, FM_IOCTL_RW_REG, &parm);
	if (0 != ret) {
		ALOGE("FM read write register failed!\n");
		return FM_FAILURE;
	}
	ALOGI("FM err=%d, addr=0x%x, value =0x%x, rw_flag=%d\n",\
		parm.err, parm.addr, parm.val, parm.rw_flag);
	*reg_val = parm.val;
	return FM_SUCCESS;
}

int fm_write_reg(unsigned int addr, unsigned int value)
{
	int ret =0;
	struct fm_reg_ctl_parm parm;

	if(addr%4) {
		ALOGE("fm_write_reg the addr is illegal %d" ,addr);
		return FM_FAILURE;
	}
	parm.rw_flag = 0;
	parm.addr = addr;
	parm.err = 0;
	parm.val = value;
	ALOGI("FM write register addr: 0x%x  val: 0x%x!\n", parm.addr, parm.val);
	ret = ioctl(fd, FM_IOCTL_RW_REG, &parm);
	if (0 != ret) {
		ALOGE("FM read write register failed!\n");
		return FM_FAILURE;
	}
	ALOGI("FM addr=0x%x, value =0x%x\n",parm.addr, parm.val);
	return FM_SUCCESS;
}

void fm_save_val(uint frq)
{
	unsigned int reg_val = 0;
	int rssi = 0;

	fm_read_reg(0xB0, &reg_val);
	mFmSignalParm.nOperInd = (reg_val>>16)&0x1;
	mFmSignalParm.nStereoInd = (reg_val>>17)&0x1;
	mFmSignalParm.nPilotDet = (reg_val>>18)&0x3;
	Radio_GetRssi(&mFmSignalParm.nRssi);
	fm_read_reg(0xBC, &reg_val);
	mFmSignalParm.nFreqOffset = reg_val&0xFFFF;
	fm_read_reg(0xC0, &reg_val);
	reg_val = 512 - (reg_val & 0x1FF);
	mFmSignalParm.nPwrIndicator = reg_val;
	fm_read_reg(0xC8, &reg_val);
	mFmSignalParm.nNoDacLpf = reg_val&0x3;
	mFmSignalParm.nFreqValue = frq;
}

int fm_set_seek(unsigned int startFreq,char mode)
{
	int ret = 0;

	if(mode == 0) {//seek up
		mode = FM_SEEK_UP;
		startFreq = startFreq + 100;
	}
	else if(mode == 1) {//seek down
		mode = FM_SEEK_DOWN;
		startFreq = startFreq - 100;
	}
	ret = Radio_Seek( &startFreq, mode );
	ALOGI("fm_set_seek done, freq: %d", startFreq*100);
	Radio_Tune(startFreq*100);
	fm_save_val(startFreq*100);

	return ret;
}

int fm_get_tune(uint frq)
{
	int ret = 0, rssi = 0;
	unsigned int reg_val = 0;

	ALOGI("fm_get_tune, freq: %d", frq);
	ret = Radio_Tune(frq);
	fm_save_val(frq);

	return ret;
}

int fm_set_status(unsigned char * pdata)
{
	int ret = 0;
	uint freq = 0;

	if (*pdata==1)
	{
		ALOGI("eut: open fm");
		freq = mFmTune;
		ret = Radio_Open(freq);
		if (0 != ret) {
			ALOGE("FM open failed!\n");
			return ret;
		}
		ret = Radio_Play(freq);
		*pdata=0;
	}
	else if(*pdata==0)
	{
		ALOGI("eut: close fm");
		ret = Radio_Close();
	}
	return ret;
}

//for Pandora tool  ( Mobiletest)
int start_fm_test(unsigned char * buf,int len,char *rsp)
{
	unsigned int i;
	int ret = 0;
	unsigned char *pdata=NULL;
	char *p = NULL;
	MSG_HEAD_T *head_ptr=NULL;
	unsigned char temp[11];
	char tmprsp[512]={0};
	unsigned int *set_frq = NULL, reg_val = 0;
	unsigned int headlen = sizeof(MSG_HEAD_T);
	FM_RW_REG_T *receive_addr = NULL;
	FM_RW_REG_T *send_addr = NULL;

	head_ptr = (MSG_HEAD_T *)(buf+1); //Diag_head
	pdata = buf + DIAG_HEADER_LENGTH + 1; //data

	p = tmprsp + headlen;
	ALOGI("start FM test Subtype=%d,%d",head_ptr->subtype,*pdata);

	switch(head_ptr->subtype)
		{
		case FM_CMD_STATE:
			ret =fm_set_status(pdata);
			ALOGI("fm_set_status =%d",ret);
			*p = ret;
			head_ptr->len = headlen + 1;
			break;
		case FM_CMD_VOLUME:
			ALOGI("FM_CMD_VOLUME =%d",*pdata);
			ret = fm_set_volume(*pdata);
			*p = ret;
			head_ptr->len = headlen + 1;
			break;
		case FM_CMD_MUTE:
			ALOGI("FM_CMD_MUTE");
			ret = fm_set_mute();
			*p = ret;
			head_ptr->len = headlen + 1;
			break;
		case FM_CMD_TUNE:
			set_frq = (unsigned int *)pdata;
			ALOGI("FM_CMD_TUNE =%d",*set_frq);
			ret = fm_get_tune(*set_frq);
			head_ptr->len = headlen + sizeof(FM_SIGNAL_PARAM_T);
			memcpy(p,&mFmSignalParm, sizeof(FM_SIGNAL_PARAM_T));
			break;
		case FM_CMD_SEEK:
			set_frq = (unsigned int *)pdata;
			ALOGI("FM_CMD_SEEK =%d, mode = %d, %d\n",*set_frq,*(pdata+4),sizeof(FM_SIGNAL_PARAM_T));
			ret = fm_set_seek(*set_frq,*(pdata+4));
			head_ptr->len = headlen + sizeof(FM_SIGNAL_PARAM_T);
			memcpy(p,&mFmSignalParm, sizeof(FM_SIGNAL_PARAM_T));
			break;
		case FM_CMD_READ_REG:
			receive_addr = (FM_RW_REG_T *)pdata;
			ALOGI("FM_CMD_READ_REG = 0x%x, mode = %d, %d\n",receive_addr->nStartAddr,receive_addr->nUintCount,sizeof(FM_RW_REG_T));
			for(i = 0; i < receive_addr->nUintCount;i++)
			{
				ret &= fm_read_reg(receive_addr->nStartAddr+i*4, &reg_val);
				memcpy((p + sizeof(FM_RW_REG_T) + i*sizeof(unsigned int)),&reg_val, sizeof(unsigned int));
			}
			receive_addr->nErrorCode = ret;
			memcpy(p ,receive_addr, sizeof(FM_RW_REG_T));
			head_ptr->len = headlen +  sizeof(FM_RW_REG_T) + receive_addr->nUintCount * sizeof(unsigned int);
			break;
		case FM_CMD_WRITE_REG:
			receive_addr = (FM_RW_REG_T *)pdata;
			ALOGI("FM_CMD_WRITE_REG = 0x%x, mode = %d\n",receive_addr->nStartAddr,receive_addr->nUintCount);
			for(i = 0; i < receive_addr->nUintCount; i++)
			{
				ALOGE("FM_CMD_WRITE_REG addr=0x%x, value=0x%x\n",
					receive_addr->nStartAddr+i*4, *(unsigned int *)(pdata + sizeof(FM_RW_REG_T)+i*4));
				ret &= fm_write_reg(receive_addr->nStartAddr+i*4, *(unsigned int *)(pdata + sizeof(FM_RW_REG_T)+i*4));
			}
			receive_addr->nErrorCode = ret;
			memcpy(p ,receive_addr, sizeof(FM_RW_REG_T));
			head_ptr->len = headlen +  sizeof(FM_RW_REG_T);
			break;
		default:
			break;
		}
	memcpy(tmprsp, (unsigned char*)head_ptr, headlen);
	for(i = 0 ; i <head_ptr->len;i ++)
	{
		ALOGD("start FM test =%x",tmprsp[i]);
	}
	return translate_packet(rsp,tmprsp,head_ptr->len);
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------


#ifdef __cplusplus
}
#endif

#endif//_FM_EUT

