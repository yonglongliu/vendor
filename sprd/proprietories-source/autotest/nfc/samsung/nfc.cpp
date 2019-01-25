#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <time.h>
#include <errno.h>

#include <cutils/log.h>
#include "diag.h"
#include <debug.h>
//#include "autotest_modules.h"

#include <hardware/hardware.h>
#include <hardware/nfc.h>
#include "nfc.h"

#define POWER_DRIVER "/dev/sec-nfc"
#define TRANS_DRIVER "/dev/sec-nfc"

/* ioctl */
#define SEC_NFC_MAGIC	'S'
#define	SEC_NFC_GET_MODE		_IOW(SEC_NFC_MAGIC, 0, unsigned int)
#define	SEC_NFC_SET_MODE		_IOW(SEC_NFC_MAGIC, 1, unsigned int)
#define	SEC_NFC_SLEEP			_IOW(SEC_NFC_MAGIC, 2, unsigned int)
#define	SEC_NFC_WAKEUP			_IOW(SEC_NFC_MAGIC, 3, unsigned int)


typedef enum {
    NFC_DEV_MODE_OFF = 0,
    NFC_DEV_MODE_ON,
    NFC_DEV_MODE_BOOTLOADER,
} eNFC_DEV_MODE;


int device_set_mode(eNFC_DEV_MODE mode);
void OSI_delay (uint32_t timeout);
int check_nfc(void);
int check_nfc_hal(void);

static nfc_event_t last_event = HAL_NFC_ERROR_EVT;
static nfc_status_t last_evt_status = HAL_NFC_STATUS_FAILED;
void hal_context_callback(nfc_event_t event, nfc_status_t event_status)
{
    last_event = event;
    last_evt_status = event_status;
    ALOGD("nfc context callback event=%u, status=%u\n", event, event_status);
}

static uint16_t last_recv_data_len = 0;
static uint8_t last_rx_buff[256]={0};
void hal_context_data_callback (uint16_t data_len, uint8_t* p_data)
{
    ALOGD("nfc data callback len=%d\n", data_len);
    last_recv_data_len = data_len;
    memcpy(last_rx_buff, p_data, data_len);
}

	//int ret = 0; //0 means success
	const struct hw_module_t* hw_module = NULL;
	nfc_nci_device_t* nfc_device = NULL;

	int max_try = 0;
	uint8_t nci_core_reset_cmd[4]={0x20,0x00,0x01,0x01};
	uint8_t nci_initializ_cmd[3]={0x20,0x01,0x00};
	uint8_t core_set_config_cmd_1[24] ={0x20,0x02,0x15,0x03,0xA0,0x0E,0x01,0x00,0xA0,0x0D,0x06,0x06,0x42,0x00,0x00,0xF3,0xF3,0xA0,0x0D,0x04,0x06,0x03,0x00,0x71};
	uint8_t core_set_config_cmd_2[28] ={0x20,0x02,0x19,0x06,0xA0,0xEC,0x01,0x01,0xA0,0xED,0x01,0x00,0xA0,0xEE,0x01,0x01,0xA0,0x03,0x01,0x08,0xA0,0x5E,0x01,0x01,0xA0,0x40,0x01,0x01}; //for XTAL
// 	uint8_t core_set_config_cmd_2[36] ={0x20,0x02,0x21,0x08,0xA0,0xEC,0x01,0x01,0xA0,0xED,0x01,0x00,0xA0,0xEE,0x01,0x01,0xA0,0x02,0x01,0x01,0xA0,0x03,0x01,0x11,0xA0,0x04,0x01,0x01,0xA0,0x5E,0x01,0x01,0xA0,0x40,0x01,0x01};//for PLL CLK REQ
	uint8_t rf_set_listen_mode_routing_cmd[10] ={0x21,0x01,0x07,0x00,0x01,0x01,0x03,0x00,0x01,0x05}; 	
//	uint8_t discover_map_cmd[10]={0x21,0x00,0x07,0x02,0x04,0x03,0x02,0x05,0x03,0x03};
	uint8_t discover_map_cmd[13]={0x21,0x00,0x0A,0x03,0x04,0x03,0x02,0x05,0x03,0x03,0x80,0x01,0x80};
/*	uint8_t rf_discover_cmd_all[30]={0x21,0x03,0x1b,0x0d,0x00,0x01,0x01,0x01,0x02,0x01,0x03,0x01,
					   0x05,0x01,0x80,0x01,0x81,0x01,0x82,0x01,0x83,0x01,0x85,0x01,
					   0x06,0x01,0x74,0x01,0x70,0x01};
*/
	uint8_t rf_discover_cmd_all[28] ={0x21,0x03,0x19,0x0C,0x00,0x01,0x01,0x01,0x02,0x01,
									0x03,0x01,0x05,0x01,0x80,0x01,0x81,0x01,0x82,0x01,
									0x83,0x01,0x85,0x01,0x06,0x01,0x70,0x01}; // for pn548
	uint8_t rf_discover_cmd_limit[8]={0x21,0x03,0x05,0x02,0x00,0x01,0x01,0x01};

int nfc_open(void)
{
	int ret =0;
	int hal_reset_nfc = 0;
	
	ALOGD("hw_get_module....");
	ret = hw_get_module (NFC_NCI_NXP_PN54X_HARDWARE_MODULE_ID, &hw_module);
	if (ret == 0)
	{
		ALOGD("OK\nopen module...");
		ret = nfc_nci_open (hw_module, &nfc_device);
		if (ret != 0){
	    		ALOGD ("fail.\n");
			return -1;
		}
	}
	ALOGD("OK\n");

	ALOGD("open nfc device...\n");
        nfc_device->open (nfc_device, hal_context_callback, hal_context_data_callback);

	max_try = 0;
	last_event = HAL_NFC_ERROR_EVT;
	last_evt_status = HAL_NFC_STATUS_FAILED;
	last_recv_data_len = 0;
	memset(last_rx_buff, 0, sizeof(last_rx_buff));
	while(last_event != HAL_NFC_OPEN_CPLT_EVT && last_recv_data_len != 6  ){
		OSI_delay(100);
		if(max_try ++ > 150){//FW will be downloaded if not forced, might take up to 15 seconds?
			ALOGD("open device timeout!\n");
			nfc_nci_close(nfc_device);
			return -1;
		}
	}
	if(last_recv_data_len == 6){ //No FW was downloaded, HAL sent reset but did not report HAL_NFC_OPEN_CPLT_EVT
		if(last_rx_buff[0] == 0x40 &&
			last_rx_buff[1] == 0x00 &&
			last_rx_buff[2] == 0x03 &&
			last_rx_buff[3] == 0x00 &&
			last_rx_buff[5] == 0x01)
		{
			ALOGD("HAL reset nfc device during open.\n");
			hal_reset_nfc = 1;
			ret  = 0;
		}
		else{
			nfc_nci_close(nfc_device);
			ALOGD("HAL reported error reset response!\n");
			return -1;
		}
	}
	else if(last_evt_status == HAL_NFC_STATUS_OK){
		ALOGD("HAL reported open completed event!\n");
	}
	else{
		nfc_nci_close(nfc_device);
		ALOGD("open device failed\n");
		return -1;
	}
	ALOGD("OK\n");

	//Reset device
	if(hal_reset_nfc == 0){
		ALOGD("reset nfc device...\n");
		max_try = 0;
		last_recv_data_len = 0;
		memset(last_rx_buff, 0, sizeof(last_rx_buff));
		nfc_device->write(nfc_device, sizeof(nci_core_reset_cmd), nci_core_reset_cmd);
		while(last_recv_data_len == 0){
			OSI_delay(10);
			if(max_try ++ > 500){
				ALOGD("reset device timeout\n");
				break;
			}
		}
		if(last_recv_data_len == 6 && 
			last_rx_buff[0] == 0x40 &&
			last_rx_buff[1] == 0x00 &&
			last_rx_buff[2] == 0x03 &&
			last_rx_buff[3] == 0x00 &&
			last_rx_buff[5] == 0x01)
		{
			ALOGD("OK\n");
			ret  = 0;
		}
		else{
			ALOGD("failed to reset nfc device.\n");
			ret = -1;
		}
	}

	//Initialize device
	if(ret == 0){
		ALOGD("Initialize nfc device...\n");
		max_try = 0;
		last_recv_data_len = 0;
		memset(last_rx_buff, 0, sizeof(last_rx_buff));
		nfc_device->write(nfc_device, sizeof(nci_initializ_cmd), nci_initializ_cmd);
		while(last_recv_data_len == 0){
			OSI_delay(10);
			if(max_try ++ > 500){
				ALOGD("initiaze device timeout\n");
				break;
			}
		}
		ALOGD("initialze nfc device lanny for sprd BBAutoTtest 20170420 21:05~~last_recv_data_len=d%\n",last_recv_data_len);
		for(int i=0;i<5;i++)
		{
			ALOGD("last_rx_buff[%d]=%d\n",i,last_rx_buff[i]);
		}
		if(last_recv_data_len == 28 && 
			last_rx_buff[0] == 0x40 &&
			last_rx_buff[1] == 0x01 &&
			last_rx_buff[2] == 0x19 &&
			last_rx_buff[3] == 0x00)
		{
			ALOGD("OK\n");
			ret  = 0;
		}
		else{
			ALOGD("failed to initialze nfc device.\n");
			ret = -1;
		}
	}
	if(ret == 0){
		ALOGD("F/W version: %x.%x.%x\n", 
				last_rx_buff[25],
				last_rx_buff[26], 
				last_rx_buff[27]);
	}

	//Nofity HAL for device being initialized
	if(ret == 0){
		ALOGD("notifiy nfc device initialized...\n");
		nfc_device->core_initialized (nfc_device, last_rx_buff);
		max_try = 0;
		last_event = HAL_NFC_ERROR_EVT;
		last_evt_status = HAL_NFC_STATUS_FAILED;
		while(last_event != HAL_NFC_POST_INIT_CPLT_EVT){
			OSI_delay(100);
			if(max_try ++ > 150){//FW will be downloaded if not exit or newer, might take up to 15 seconds?
				ALOGD("notify device timeout\n");
				break;
			}
		}
		if(last_evt_status == HAL_NFC_STATUS_OK){
			ALOGD("OK\n");
		}
		else{
			ALOGD("failed\n");
			ret = -1;
		}
	}
return ret;
}
static int checkID_result=1;	
int nfc_checkID(void)	
{
	//core set config cmd 1
	{
		ALOGD("core set config cmd 1...\n");

		max_try = 0;
		last_recv_data_len = 0;
		memset(last_rx_buff, 0, sizeof(last_rx_buff));
		nfc_device->write(nfc_device, sizeof(core_set_config_cmd_1), core_set_config_cmd_1);
		while(last_recv_data_len == 0){
			OSI_delay(10);
			if(max_try ++ > 100){	
				ALOGD("core set config cmd 1 timeout\n");
				break;
			}
		}
		if(last_recv_data_len == 9 && 
			last_rx_buff[0] == 0x40 &&
			last_rx_buff[1] == 0x02 &&
			last_rx_buff[2] == 0x06)
		{
			ALOGD("OK\n");
			checkID_result = 0;
			
		}
		else{
			ALOGD("failed to core set config cmd 1.\n");
			checkID_result = 1;
		}
	}
	//core set config cmd 2
	if(checkID_result == 0)
	{
		ALOGD("core set config cmd 2...\n");

		max_try = 0;
		last_recv_data_len = 0;
		memset(last_rx_buff, 0, sizeof(last_rx_buff));
		nfc_device->write(nfc_device, sizeof(core_set_config_cmd_2), core_set_config_cmd_2);
		while(last_recv_data_len == 0){
			OSI_delay(10);
			if(max_try ++ > 100){
				ALOGD("core set config cmd 2 timeout\n");
				break;
			}
		}
		if(last_recv_data_len == 5 && 
			last_rx_buff[0] == 0x40 &&
			last_rx_buff[1] == 0x02 &&
			last_rx_buff[2] == 0x02)
		{
			ALOGD("OK\n");
			checkID_result = 0;
			
		}
		else{
			ALOGD("failed to core set config cmd 2.\n");
			checkID_result = 1;
		}
	}
	//rf set listen mode routing cmd
	if(checkID_result == 0)
	{
		ALOGD("rf set listen mode routing cmd...\n");

		max_try = 0;
		last_recv_data_len = 0;
		memset(last_rx_buff, 0, sizeof(last_rx_buff));
		nfc_device->write(nfc_device, sizeof(rf_set_listen_mode_routing_cmd), rf_set_listen_mode_routing_cmd);
		while(last_recv_data_len == 0){
			OSI_delay(10);
			if(max_try ++ > 100){
				ALOGD("rf set listen mode routing cmd timeout\n");
				break;
			}
		}
		if(last_recv_data_len == 4 && 
			last_rx_buff[0] == 0x41 &&
			last_rx_buff[1] == 0x01 &&
			last_rx_buff[2] == 0x01)
		{
			ALOGD("OK\n");
			checkID_result = 0;
		}
		else{
			ALOGD("failed to rf set listen mode routing cmd.\n");
			checkID_result = 1;
		}
	}
	//discover map
	if(checkID_result == 0)
	{
		ALOGD("discover map...\n");

		max_try = 0;
		last_recv_data_len = 0;
		memset(last_rx_buff, 0, sizeof(last_rx_buff));
		nfc_device->write(nfc_device, sizeof(discover_map_cmd), discover_map_cmd);
		while(last_recv_data_len == 0){
			OSI_delay(10);
			if(max_try ++ > 500){
				ALOGD("discover map timeout\n");
				break;
			}
		}
		if(last_recv_data_len == 4 && 
			last_rx_buff[0] == 0x41 &&
			last_rx_buff[1] == 0x00 &&
			last_rx_buff[2] == 0x01)
		{
			ALOGD("OK\n");
			checkID_result = 0;
		}
		else{
			ALOGD("discover map failed.\n");
			checkID_result = 1;
		}
	}
	if(checkID_result == 0){
		ALOGD("rf discover...\n");

		max_try = 0;
		last_recv_data_len = 0;
		memset(last_rx_buff, 0, sizeof(last_rx_buff));
		nfc_device->write(nfc_device, sizeof(rf_discover_cmd_all), rf_discover_cmd_all);
		while(last_recv_data_len == 0){
			OSI_delay(10);
			if(max_try ++ > 100){
				ALOGD("discover cmd timeout\n");
				break;
			}
		}
		if(last_recv_data_len == 4 && 
			last_rx_buff[0] == 0x41 &&
			last_rx_buff[1] == 0x03 &&
			last_rx_buff[2] == 0x01)
		{
			ALOGD("OK\n");
			checkID_result = 0;
			
		}
		else{
			ALOGD("failed to set nfc to poll.\n");
			checkID_result = 1;
		}
	}

	//Ready to detect the tags from antenna
	if(checkID_result == 0){
		ALOGD("nfc is polling, please put a type 1/2/4 tag to the antenna...\n");
		max_try = 0;
		last_recv_data_len = 0;
		memset(last_rx_buff, 0, sizeof(last_rx_buff));

		while(last_recv_data_len == 0){
			OSI_delay(100);
			if(max_try ++ > 300){ //Wait max 30 seconds
				ALOGD("waiting tag timeout\n");
				break;
			}
		}
		if(last_recv_data_len >= 10 && 
			last_rx_buff[0] == 0x61 &&
			last_rx_buff[1] == 0x05 &&
			last_rx_buff[4] == 0x02 &&
			last_rx_buff[5] == 0x04)
		{
			ALOGD("Type 4 Tag detected.\n");
			checkID_result = 0;
		}
		else if(last_recv_data_len >= 10 && 
			last_rx_buff[0] == 0x61 &&
			last_rx_buff[1] == 0x05 &&
			last_rx_buff[4] == 0x01 &&
			last_rx_buff[5] == 0x01)
		{
			ALOGD("Type 1 Tag detected.\n");
			checkID_result = 0;
		}
		else if(last_recv_data_len >= 10 && 
			last_rx_buff[0] == 0x61 &&
			last_rx_buff[1] == 0x05 &&
			last_rx_buff[4] == 0x01 &&
			last_rx_buff[5] == 0x02)
		{
			ALOGD("Type 2 Tag detected.\n");
			checkID_result = 0;
		}
		else{
			ALOGD("failed to detect known tags.\n");
			checkID_result = 1;
		}
	}

	return checkID_result;
}

int nfc_close(void)
{
	int ret=0;
	ALOGD("Close device\n");
	ret = nfc_device->close (nfc_device);
	ret = nfc_nci_close(nfc_device);
	return ret;
}


void OSI_delay (uint32_t timeout)
{
    struct timespec delay;
    int err;

    delay.tv_sec = timeout / 1000;
    delay.tv_nsec = 1000 * 1000 * (timeout%1000);

    do
    {
        err = nanosleep(&delay, &delay);
    } while (err < 0 && errno == EINTR);
}





