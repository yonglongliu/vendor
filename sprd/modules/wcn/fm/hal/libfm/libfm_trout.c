/*
 * Copyright (c) 2009-2010, Code Aurora Forum. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *        * Redistributions of source code must retain the above copyright
 *            notice, this list of conditions and the following disclaimer.
 *        * Redistributions in binary form must reproduce the above copyright
 *            notice, this list of conditions and the following disclaimer in the
 *            documentation and/or other materials provided with the distribution.
 *        * Neither the name of Code Aurora nor
 *            the names of its contributors may be used to endorse or promote
 *            products derived from this software without specific prior written
 *            permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.    IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define LOG_TAG "fmradio"

#include <cutils/log.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include "fm.h"


#define RDAFM_ENABLE

#ifdef RDAFM_ENABLE
#define FM_IOC_MAGIC        0xf5 // FIXME: any conflict?

#define FM_IOCTL_POWERUP       _IOWR(FM_IOC_MAGIC, 0, struct fm_tune_parm*)
#define FM_IOCTL_POWERDOWN     _IOWR(FM_IOC_MAGIC, 1, int32_t*)
#define FM_IOCTL_SET_TUNE      _IOWR(FM_IOC_MAGIC, 2, struct fm_tune_parm*)
#define FM_IOCTL_SEEK          _IOWR(FM_IOC_MAGIC, 3, struct fm_seek_parm*)
#define FM_IOCTL_SET_VOLUME    _IOWR(FM_IOC_MAGIC, 4, uint32_t*)
#define FM_IOCTL_GET_VOLUME    _IOWR(FM_IOC_MAGIC, 5, uint32_t*)
#define FM_IOCTL_MUTE          _IOWR(FM_IOC_MAGIC, 6, uint32_t*)
#define FM_IOCTL_GET_RSSI      _IOWR(FM_IOC_MAGIC, 7, int32_t*)
#define FM_IOCTL_SCAN          _IOWR(FM_IOC_MAGIC, 8, struct fm_scan_parm*)
#define FM_IOCTL_STOP_SEARCH   _IO(FM_IOC_MAGIC,   9)
#define FM_IOCTL_CONFIG        _IOW(FM_IOC_MAGIC, 20, int)
#define FM_IOCTL_ANA_SWITCH    _IOWR(FM_IOC_MAGIC, 30, int32_t*)

// extension interface
#define FM_IOCTL_DCC_SETBW   _IOWR(FM_IOC_MAGIC, 0x60, int32_t*)

#define FM_AUTO_HILO_OFF    0
#define FM_AUTO_HILO_ON     1

/*  seek direction */
#define FM_SEEK_UP 0
#define FM_SEEK_DOWN 1

// space
#define FM_SPACE_UNKNOWN    0
#define FM_SPACE_100K       1
#define FM_SPACE_200K       2
#define FM_SPACE_50K        5
#define FM_SPACE_DEFAULT    FM_SPACE_100K

#define FM_SEEK_SPACE 1

/*  band */
#define FM_BAND_UNKNOWN 0
#define FM_BAND_UE      1 /*  US/Europe band  87.5MHz ~ 108MHz (DEFAULT) */
#define FM_BAND_JAPAN   2 /*  Japan band      76MHz   ~ 90MHz  */
#define FM_BAND_JAPANW  3 /*  Japan wideband  76MHZ   ~ 108MHz */
#define FM_BAND_SPECIAL 4 /*  special   band  between 76MHZ   and  108MHz */
#define FM_BAND_DEFAULT FM_BAND_UE

struct fm_tune_parm {
    uint8_t err;
    uint8_t band;
    uint8_t space;
    uint8_t hilo;
    uint16_t freq; // IN/OUT parameter
};

struct fm_seek_parm {
    uint8_t err;
    uint8_t band;
    uint8_t space;
    uint8_t hilo;
    uint8_t seekdir;
    uint8_t seekth;
    uint16_t freq; // IN/OUT parameter
};

#else
// ioctl command
#define FM_IOCTL_BASE     'R'
#define FM_IOCTL_ENABLE      _IOW(FM_IOCTL_BASE, 0, int)
#define FM_IOCTL_GET_ENABLE  _IOW(FM_IOCTL_BASE, 1, int)
#define FM_IOCTL_SET_TUNE    _IOW(FM_IOCTL_BASE, 2, int)
#define FM_IOCTL_GET_FREQ    _IOW(FM_IOCTL_BASE, 3, int)
#define FM_IOCTL_SEARCH      _IOW(FM_IOCTL_BASE, 4, int[4])
#define FM_IOCTL_STOP_SEARCH _IOW(FM_IOCTL_BASE, 5, int)
#define FM_IOCTL_SET_VOLUME  _IOW(FM_IOCTL_BASE, 7, int)
#define FM_IOCTL_GET_VOLUME  _IOW(FM_IOCTL_BASE, 8, int)
#define FM_IOCTL_CONFIG      _IOW(FM_IOCTL_BASE, 9, int)
#define FM_IOCTL_GET_RSSI    _IOW(FM_IOCTL_BASE, 10,int)
#define FM_IOCTL_ANA_SWITCH  _IOW(FM_IOCTL_BASE, 30, int)
#endif

// operation result
#define FM_SUCCESS 0
#define FM_FAILURE -1
#define FM_TIMEOUT 1

// search direction
#define SEARCH_DOWN 0
#define SEARCH_UP   1

// operation command
#define V4L2_CID_PRIVATE_BASE           0x8000000
#define V4L2_CID_PRIVATE_TAVARUA_STATE  (V4L2_CID_PRIVATE_BASE + 4)

#define V4L2_CTRL_CLASS_USER            0x980000
#define V4L2_CID_BASE                   (V4L2_CTRL_CLASS_USER | 0x900)
#define V4L2_CID_AUDIO_VOLUME           (V4L2_CID_BASE + 5)
#define V4L2_CID_AUDIO_MUTE             (V4L2_CID_BASE + 9)
#define V4L2_CID_FM_CONFIG              (V4L2_CID_BASE + 10)
#define V4L2_CID_FM_ANTENNA             (V4L2_CID_BASE + 30)

struct sr2315_device_t
{
	struct fm_device_t dev;
	int fd;
	int freq;
};

static int
getFreq(struct fm_device_t* dev,int* freq)
{
    struct sr2315_device_t *device = (struct sr2315_device_t *)dev;	
    if((NULL == device) || (device->fd < 0))
    {
        return FM_FAILURE;
    }
#ifdef RDAFM_ENABLE
    *freq = device->freq;  
    return 0;
#else
    return ioctl(device->fd,FM_IOCTL_GET_FREQ,freq);
#endif
}

static int 
getRssi(struct fm_device_t* dev,int* rssi)
{
    struct sr2315_device_t *device = (struct sr2315_device_t *)dev;
   
    if((NULL == device) || (device->fd < 0))
    {
         return FM_FAILURE;
    }
    return ioctl(device->fd,FM_IOCTL_GET_RSSI,rssi);
}

/*native interface */
static int
setFreq(struct fm_device_t* dev,int freq)
{
    struct sr2315_device_t *device = (struct sr2315_device_t *)dev;
#ifdef RDAFM_ENABLE
    struct fm_tune_parm parm;
    parm.band = FM_BAND_UE;
    parm.freq = freq;
    parm.hilo = FM_AUTO_HILO_OFF;
    parm.space = FM_SPACE_100K;
#endif

    if((NULL == device) || (device->fd < 0))
    {
        return FM_FAILURE;
    }
#ifdef RDAFM_ENABLE
    device->freq = freq;
    return ioctl(device->fd, FM_IOCTL_SET_TUNE, &parm);
#else
    return ioctl(device->fd, FM_IOCTL_SET_TUNE, &freq);
#endif
}

static int gVolume = 0;

static int
setControl(struct fm_device_t* dev,int id, int value)
{
    int err = -1;
    struct sr2315_device_t *device = (struct sr2315_device_t *)dev;
	
    if((NULL == device) || (device->fd < 0))
    {
        return FM_FAILURE;
    }
    int fd = device->fd;

    switch(id) {
        case V4L2_CID_PRIVATE_TAVARUA_STATE:
        {
#ifdef RDAFM_ENABLE
            if (value) {
				struct fm_tune_parm parm;

			    bzero(&parm, sizeof(struct fm_tune_parm));

			    parm.band = FM_BAND_DEFAULT;
			    parm.freq = 879;
			    parm.hilo = FM_AUTO_HILO_OFF;
			    parm.space = FM_SEEK_SPACE;
			    err = ioctl(fd, FM_IOCTL_POWERUP, &parm);
            } else {
                err = ioctl(fd, FM_IOCTL_POWERDOWN, &value);
            }
#else
            err = ioctl(fd, FM_IOCTL_ENABLE, &value);
#endif
        }
        break;

        case V4L2_CID_AUDIO_VOLUME:
        {
            gVolume = value;
            err = ioctl(fd, FM_IOCTL_SET_VOLUME, &value);
        }
        break;

        case V4L2_CID_AUDIO_MUTE:
        {
            if (value == 0) {
                err = ioctl(fd, FM_IOCTL_SET_VOLUME, &gVolume);
            } else {
                int volume = -1;
                err = ioctl(fd, FM_IOCTL_GET_VOLUME, &volume);
                if (err >= 0) {
                    gVolume = volume;
                    volume = 0;
                    err = ioctl(fd, FM_IOCTL_SET_VOLUME, &volume);
                }
            }
        }
        break;

        case V4L2_CID_FM_CONFIG:
        {
            err = ioctl(fd, FM_IOCTL_CONFIG, &value);
        }
        break;
        case V4L2_CID_FM_ANTENNA:
        {
            err = ioctl(fd, FM_IOCTL_ANA_SWITCH, &value);
        }
        break;
	default:
	    ALOGE("peter: error default");
    }

    ALOGE("(setControl)operation=%x value=%d result=%d errno=%d", id, value, err, errno);
    if (err < 0) {
        return FM_FAILURE;
    }

    return FM_SUCCESS;
}

/* native interface */
static int
startSearch(struct fm_device_t* dev,int freq, int dir, int timeout, int reserve)
{
#ifdef RDAFM_ENABLE
	struct sr2315_device_t *device = (struct sr2315_device_t *)dev;
	int ret = 0;
	struct fm_seek_parm parm;

	bzero(&parm, sizeof(struct fm_seek_parm));

	parm.band = FM_BAND_DEFAULT;
	parm.freq = freq;
	parm.hilo = FM_AUTO_HILO_OFF;
	parm.space = FM_SEEK_SPACE;
	if (dir == 1) {
		parm.seekdir = FM_SEEK_DOWN;
	} else if (dir == 0) {
		parm.seekdir = FM_SEEK_UP;
	}
	parm.seekth = 0;

	ret = ioctl(device->fd, FM_IOCTL_SEEK, &parm);
	ALOGD("%s, [fd=%d] [ret=%d]\n", __func__, device->fd, ret);

    if (ret < 0){
        return FM_FAILURE;
    } else {
        device->freq = parm.freq;
    }
ALOGE("(RDAFM - seek)freq=%d direction=%d timeout=%d reserve=%d result=%d\n", parm.freq, dir, timeout, reserve, ret);
    return FM_SUCCESS;
#else
    struct sr2315_device_t *device = (struct sr2315_device_t *)dev;
    int buffer[4] = {0};
    buffer[0] = freq;    //start frequency
    buffer[1] = dir;     //search direction
    buffer[2] = timeout; //timeout
    buffer[3] = 0;       //reserve
    int err = -1;
    int count = 0;
    if((NULL == device) || (device->fd < 0))
    {
        return FM_FAILURE;
    }
    int fd = device->fd;

    err = ioctl(fd, FM_IOCTL_SEARCH, buffer);    
    ALOGE("err=%d\n", err);
    ALOGE("(seek)freq=%d direction=%d timeout=%d reserve=%d result=%d errno=%d\n", freq, dir, timeout, reserve, err, errno);
    if(err < 0){
        return FM_FAILURE;
    }
    if (err == FM_TIMEOUT) {
        return FM_TIMEOUT;
    }
    return FM_SUCCESS;
#endif
}

static int
cancelSearch(struct fm_device_t* dev)
{
    struct sr2315_device_t *device = (struct sr2315_device_t *)dev;
	
    if((NULL == device) || (device->fd < 0))
    {
         return FM_FAILURE;
    }
	
    return ioctl(device->fd, FM_IOCTL_STOP_SEARCH);
}

static int
close_fm(struct hw_device_t *dev)
{
    struct sr2315_device_t *device = (struct sr2315_device_t *)dev;
	
    if(NULL != device)
    {
	if (device->fd >= 0)
        {
            close(device->fd);
	}
        
        free(device);    
        device = NULL;    
    }
    return 0;
}



static int open_fm(const struct hw_module_t* module, char const* name,
        struct hw_device_t** device)
{
    int fd;

    struct sr2315_device_t *sr2315_dev =(struct sr2315_device_t *) malloc(sizeof(struct sr2315_device_t));
    
    memset(sr2315_dev, 0, sizeof(*sr2315_dev));

    sr2315_dev->dev.common.tag = HARDWARE_DEVICE_TAG;
    sr2315_dev->dev.common.version = 0;
    sr2315_dev->dev.common.module = (struct hw_module_t*)module;
    sr2315_dev->dev.common.close = close_fm;
    sr2315_dev->dev.setFreq = setFreq;
    sr2315_dev->dev.getFreq = getFreq;
    sr2315_dev->dev.setControl = setControl;
    sr2315_dev->dev.startSearch = startSearch;
    sr2315_dev->dev.cancelSearch = cancelSearch;
    sr2315_dev->dev.getRssi = getRssi;
#ifdef RDAFM_ENABLE
	fd = open("/dev/fm", O_RDONLY);
        ALOGE("RDA fd open dev/fm, fd =%d",fd);
#else
    fd = open("/dev/Trout_FM", O_RDONLY);
    ALOGE("NON - RDA fd open dev/fm, fd =%d",fd);
#endif
    if(fd <0)
    {
        free(sr2315_dev);
        sr2315_dev = NULL; 
        return FM_FAILURE;
    }
    sr2315_dev->fd = fd;
    *device = &(sr2315_dev->dev.common);
    return 0;
}


static struct hw_module_methods_t fm_module_methods = {
    .open =  open_fm,
};

struct hw_module_t HAL_MODULE_INFO_SYM = {
    .tag = HARDWARE_MODULE_TAG,
    .module_api_version = HARDWARE_MODULE_API_VERSION(0, 1),
    .hal_api_version = HARDWARE_HAL_API_VERSION,
    .id = FM_HARDWARE_MODULE_ID,
    .name = "troutfm",
    .author = "Spreadtrum, Inc.",
    .methods = &fm_module_methods,
};

