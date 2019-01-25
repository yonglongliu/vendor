#include"testitem.h"

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

#include <hardware/hardware.h>
#include <hardware/nfc.h>

#define POWER_DRIVER "/dev/sec-nfc"
#define TRANS_DRIVER "/dev/sec-nfc"

/* ioctl */
#define	SEC_NFC_MAGIC	'S'
#define	SEC_NFC_GET_MODE		_IOW(SEC_NFC_MAGIC, 0, unsigned int)
#define	SEC_NFC_SET_MODE		_IOW(SEC_NFC_MAGIC, 1, unsigned int)
#define	SEC_NFC_SLEEP			_IOW(SEC_NFC_MAGIC, 2, unsigned int)
#define	SEC_NFC_WAKEUP		_IOW(SEC_NFC_MAGIC, 3, unsigned int)

typedef enum
{
	NFC_DEV_MODE_OFF = 0,
	NFC_DEV_MODE_ON,
	NFC_DEV_MODE_BOOTLOADER,
} eNFC_DEV_MODE;

int device_set_mode(eNFC_DEV_MODE mode);
void OSI_delay (uint32_t timeout);
int check_nfc(void);
void check_nfc_hal(void);
int text_row = 0;
char tag_result[64] = {0};
int nfc_result;
static int nfc_thread_run;

static nfc_event_t last_event = HAL_NFC_ERROR_EVT;
static nfc_status_t last_evt_status = HAL_NFC_STATUS_FAILED;
void hal_context_callback(nfc_event_t event, nfc_status_t event_status)
{
	last_event = event;
	last_evt_status = event_status;
	LOGD("nfc context callback event=%u, status=%u", event, event_status);
}

static uint16_t last_recv_data_len = 0;
static uint8_t last_rx_buff[256] = {0};
void hal_context_data_callback (uint16_t data_len, uint8_t* p_data)
{
	LOGD("nfc data callback len=%d", data_len);
	last_recv_data_len = data_len;
	memcpy(last_rx_buff, p_data, data_len);
}

void check_nfc_hal(void)
{
	int ret = 0;		//0 means success
	const struct hw_module_t* hw_module = NULL;
	nfc_nci_device_t* nfc_device = NULL;

	int max_try = 0;
	uint8_t nci_core_reset_cmd[4] = {0x20, 0x00, 0x01, 0x01};
	uint8_t nci_initializ_cmd[3] = {0x20, 0x01, 0x00};
	uint8_t discover_map_cmd[10] = {0x21, 0x00, 0x07, 0x02, 0x04, 0x03, 0x02, 0x05, 0x03, 0x03};
	uint8_t rf_discover_cmd_all[30] = {0x21, 0x03, 0x1b, 0x0d, 0x00, 0x01, 0x01, 0x01, 0x02, 0x01, 0x03, 0x01,
									0x05, 0x01, 0x80, 0x01, 0x81, 0x01, 0x82, 0x01, 0x83, 0x01, 0x85, 0x01,
									0x06, 0x01, 0x74, 0x01, 0x70, 0x01};
	uint8_t rf_discover_cmd_limit[8] = {0x21, 0x03, 0x05, 0x02, 0x00, 0x01, 0x01, 0x01};

	LOGD("hw_get_module....");
	ret = hw_get_module (NFC_NCI_HARDWARE_MODULE_ID, &hw_module);
	if (ret == 0)
	{
		LOGD("OK, open module...");
		ret = nfc_nci_open (hw_module, &nfc_device);
		if (ret != 0)
		{
			LOGD ("fail.");
			nfc_result = ret;
			ui_push_result(RL_FAIL);
			return;
		}
	}

	nfc_device->open (nfc_device, hal_context_callback, hal_context_data_callback);

	max_try = 0;
	last_event = HAL_NFC_ERROR_EVT;
	last_evt_status = HAL_NFC_STATUS_FAILED;
	last_recv_data_len = 0;
	memset(last_rx_buff, 0, sizeof(last_rx_buff));

	while(last_event != HAL_NFC_OPEN_CPLT_EVT && last_recv_data_len != 6  )
	{
		OSI_delay(100);
		if(max_try ++ > 150)			//FW will be downloaded if not forced, might take up to 15 seconds?
		{
			LOGD("open device timeout!");
			nfc_nci_close(nfc_device);
			nfc_result = ret;
			ui_push_result(RL_FAIL);
			return;
		}
	}

	if(last_recv_data_len == 6)		//No FW was downloaded, HAL sent reset but did not report HAL_NFC_OPEN_CPLT_EVT
	{
		if(last_rx_buff[0] == 0x40 && last_rx_buff[1] == 0x00 &&
		   last_rx_buff[2] == 0x03 && last_rx_buff[3] == 0x00 &&
		   last_rx_buff[5] == 0x01)
		{
			LOGD("HAL reset nfc device during open.");
			ret  = 0;
		}
		else
		{
			nfc_nci_close(nfc_device);
			LOGD("HAL reported error reset response!");
			nfc_result = ret;
			ui_push_result(RL_FAIL);
			return;
		}
	}
	else if(last_evt_status == HAL_NFC_STATUS_OK)
	{
		LOGD("HAL reported open completed event!");
	}
	else
	{
		nfc_nci_close(nfc_device);
		LOGD("open device failed");
		nfc_result = ret;
		ui_push_result(RL_FAIL);
		return;
	}
	LOGD("OK");

	//Reset device
	if(last_recv_data_len == 0)
	{
		LOGD("reset nfc device...");
		max_try = 0;
		last_recv_data_len = 0;
		memset(last_rx_buff, 0, sizeof(last_rx_buff));
		nfc_device->write(nfc_device, sizeof(nci_core_reset_cmd), nci_core_reset_cmd);
		while(last_recv_data_len == 0)
		{
			OSI_delay(10);
			if(max_try ++ > 500)
			{
				LOGD("reset device timeout");
				break;
			}
		}
		if(last_recv_data_len == 6 && last_rx_buff[0] == 0x40 && last_rx_buff[1] == 0x00 &&
			last_rx_buff[2] == 0x03 && last_rx_buff[3] == 0x00 && last_rx_buff[5] == 0x01)
		{
			LOGD("OK");
			ret  = 0;
		}
		else
		{
			LOGD("failed to reset nfc device.");
			ret = -1;
		}
	}

	//Initialize device
	if(ret == 0)
	{
		LOGD("Initialize nfc device...");
		max_try = 0;
		last_recv_data_len = 0;
		memset(last_rx_buff, 0, sizeof(last_rx_buff));
		nfc_device->write(nfc_device, sizeof(nci_initializ_cmd), nci_initializ_cmd);
		while(last_recv_data_len == 0)
		{
			OSI_delay(10);
			if(max_try ++ > 500)
			{
				LOGD("initiaze device timeout");
				break;
			}
		}

		if(last_recv_data_len == 23 &&	last_rx_buff[0] == 0x40 && last_rx_buff[1] == 0x01 &&
			last_rx_buff[2] == 0x14 && last_rx_buff[3] == 0x00)
		{
			LOGD("OK\n");
			ret  = 0;
		}
		else
		{
			LOGD("failed to initialze nfc device.");
			ret = -1;
		}
	}

	if(ret == 0)
	{
		uint8_t major_ver = (uint8_t)((last_rx_buff[19] >> 4) & 0x0f);
		uint8_t minor_ver = (uint8_t)((last_rx_buff[19]) & 0x0f);
		uint16_t build_info_high = (uint16_t)(last_rx_buff[20]);
		uint16_t build_info_low = (uint16_t)(last_rx_buff[21]);

		LOGD("F/W version: %d.%d.%d", major_ver, minor_ver, ((build_info_high * 0x100) + build_info_low));
	}

	//Nofity HAL for device being initialized
	if(ret == 0)
	{
		LOGD("notifiy nfc device initialized...");
		nfc_device->core_initialized (nfc_device, last_rx_buff);
		max_try = 0;
		last_event = HAL_NFC_ERROR_EVT;
		last_evt_status = HAL_NFC_STATUS_FAILED;
		while(last_event != HAL_NFC_POST_INIT_CPLT_EVT)
		{
			OSI_delay(100);
			if(max_try ++ > 150)	//FW will be downloaded if not exit or newer, might take up to 15 seconds?
			{
				LOGD("notify device timeout");
				break;
			}
		}
		if(last_evt_status == HAL_NFC_STATUS_OK)
		{
			LOGD("OK");
		}
		else
		{
			LOGD("failed");
			ret = -1;
		}
	}

	//Set nfc device to poll
	if(ret == 0)
	{
		LOGD("discover map...");

		max_try = 0;
		last_recv_data_len = 0;
		memset(last_rx_buff, 0, sizeof(last_rx_buff));
		nfc_device->write(nfc_device, sizeof(discover_map_cmd), discover_map_cmd);
		while(last_recv_data_len == 0)
		{
			OSI_delay(10);
			if(max_try ++ > 500)
			{
				LOGD("discover map timeout");
				break;
			}
		}

		if(last_recv_data_len == 4 && last_rx_buff[0] == 0x41 && last_rx_buff[1] == 0x00 &&
			last_rx_buff[2] == 0x01 && last_rx_buff[3] == 0x00)
		{
			LOGD("OK");
			ret  = 0;
		}
		else
		{
			LOGD("discover map failed.");
			ret = -1;
		}
	}
	if(ret == 0)
	{
		LOGD("rf discover...");

		max_try = 0;
		last_recv_data_len = 0;
		memset(last_rx_buff, 0, sizeof(last_rx_buff));
		nfc_device->write(nfc_device, sizeof(rf_discover_cmd_all), rf_discover_cmd_all);
		while(last_recv_data_len == 0)
		{
			OSI_delay(10);
			if(max_try ++ > 100)
			{
				LOGD("discover cmd timeout");
				break;
			}
		}

		if(last_recv_data_len == 4 && last_rx_buff[0] == 0x41 && last_rx_buff[1] == 0x03 &&
			last_rx_buff[2] == 0x01 && last_rx_buff[3] == 0x00)
		{
			LOGD("OK");
			ret  = 0;
		}
		else
		{
			LOGD("failed to set nfc to poll.");
			ret = -1;
		}
	}

	//Ready to detect the tags from antenna
	if(ret == 0)
	{
		LOGD("nfc is polling, please put a type 1/2/4 tag to the antenna...");
		max_try = 0;
		last_recv_data_len = 0;
		memset(last_rx_buff, 0, sizeof(last_rx_buff));

		while(last_recv_data_len == 0)
		{
			OSI_delay(100);
			if(nfc_thread_run == 0)
			{
				strncpy(tag_result, "manual exist", sizeof(tag_result));
				LOGD("manual exist");
				ret = -1;
				break;
			}
			if(max_try ++ > NFC_TIMEOUT * 10)  //Wait max 30 seconds
			{
				strncpy(tag_result, "timeout", sizeof(tag_result));
				LOGD("waiting tag timeout");
				ret = -1;
				break;
			}
		}

		LOGD("received data_len = %d",last_recv_data_len);
		if(last_recv_data_len >= 10 && last_rx_buff[0] == 0x61 && last_rx_buff[1] == 0x05 &&
			last_rx_buff[4] == 0x02 && last_rx_buff[5] == 0x04)
		{
			strncpy(tag_result, "Type 4 Tag.", sizeof(tag_result));
			LOGD("Type 4 Tag detected.");
			ret  = TYPE4;
		}
		else if(last_recv_data_len >= 10 && last_rx_buff[0] == 0x61 && last_rx_buff[1] == 0x05 &&
			last_rx_buff[4] == 0x01 && last_rx_buff[5] == 0x01)
		{
			strncpy(tag_result, "Type 1 Tag.", sizeof(tag_result));
			LOGD("Type 1 Tag detected.");
			ret  = TYPE1;
		}
		else if(last_recv_data_len >= 10 && last_rx_buff[0] == 0x61 && last_rx_buff[1] == 0x05 &&
			last_rx_buff[4] == 0x01 && last_rx_buff[5] == 0x02)
		{
			strncpy(tag_result, "Type 2 Tag.", sizeof(tag_result));
			LOGD("Type 2 Tag detected.");
			ret  = TYPE2;
		}
		else if(last_recv_data_len > 0)
		{
			strncpy(tag_result, "Unknow tag", sizeof(tag_result));
			LOGD("detect known tags.");
			ret = UNKNOWN_TYPE;
		}
	}

	LOGD("Close device\n");
	nfc_device->close (nfc_device);
	nfc_nci_close(nfc_device);
	nfc_result = ret;
	if(nfc_result == -1)
		ui_push_result(RL_FAIL);
	else
		ui_push_result(RL_PASS);
	return;
}

int check_nfc(void)
{
	int pw_driver, tr_driver;
	int ret;
	int retry, total;
	unsigned char get_boot_info_cmd[4] = {0x00, 0x01, 0x00, 0x00};
	unsigned char nci_core_reset_cmd[4] = {0x20, 0x00, 0x01, 0x01};
	unsigned char nci_initializ_cmd[3] = {0x20, 0x01, 0x00};
	unsigned char rx_buff[23] = {0};

	pw_driver = open(POWER_DRIVER, O_RDWR | O_NOCTTY);
	if (pw_driver < 0)
	{
		LOGD("Failed to open device driver: %s", POWER_DRIVER);
		return pw_driver;
	}
	tr_driver = pw_driver;

	//Bootloader checking
	LOGD("setting device to bootloader mode...");
	ret = ioctl(pw_driver, SEC_NFC_SET_MODE, (int)NFC_DEV_MODE_BOOTLOADER);
	if (ret)
	{
		LOGD("failed, error=%d", ret);
		close(pw_driver);
		return ret;
	}

	LOGD("OK\nGet bootloader info...");
	OSI_delay(100);

	ret = write(tr_driver, get_boot_info_cmd, 4 );
	if (ret != 4)
	{
		LOGD("failed to send get bootloader command.");
		close(pw_driver);
		return -1;
	}
	LOGD("OK\nValidate bootloader for N81...");

	OSI_delay(10);

	ret = read(tr_driver, rx_buff, 18);
	if (ret != 18)
	{
		LOGD("Read bootloader info error ret = %d, errno = %d", ret, errno);
		return -1;
	}
	if(rx_buff[0] != 0x81 || rx_buff[1] != 0x00 || rx_buff[2] != 0x0E || rx_buff[3] != 0x00 || rx_buff[4] != 0x81)
	{
		LOGD("bootloader info validation error = %d, errno = %d", ret, errno);
		close(pw_driver);
		return -1;
	}
	LOGD("OK");

	//Important note!!
	LOGD("NCI checking requirs the NFC chip to have a working firmware, at factory line,");
	LOGD("Original NFC chips for factory do not have firmware, firmware will be downloaded");
	LOGD("when the first time Android NFC is turned on or NFC HAL is initialized.");
	LOGD("If the NFC in Android is never turned on or HAL is never initialized, following NCI checking should be skipped.");
	LOGD("Setting device to full power nci mode...");
	ret = ioctl(pw_driver, SEC_NFC_SET_MODE, (int)NFC_DEV_MODE_ON);
	if (ret)
	{
		LOGD("failed, error=%d", ret);
		close(pw_driver);
		return ret;
	}

	//Reset command checking
	LOGD("OK\nTry reset command...");
	OSI_delay(100);

	ioctl(pw_driver, SEC_NFC_SLEEP, 0);
	OSI_delay(1);
	ioctl(pw_driver, SEC_NFC_WAKEUP, 0);
	OSI_delay(1);

	ret = write(tr_driver, nci_core_reset_cmd, 4 );
	if (ret != 4)
	{
		LOGD("failed to send reset command.ret=%d", ret);
		close(pw_driver);
		return -1;
	}
	LOGD("OK\nValidate reset command...");

	OSI_delay(10);

	memset(rx_buff, 0, sizeof(rx_buff));
	ret = read(tr_driver, rx_buff, 6);
	if (ret != 6)
	{
		LOGD("Read reset response error ret = %d, errno = %d", ret, errno);
		close(pw_driver);
		return -1;
	}
	if(rx_buff[0] != 0x40 || rx_buff[1] != 0x00 || rx_buff[2] != 0x03 || rx_buff[3] != 0x00 || rx_buff[5] != 0x01)
	{
		LOGD("reset command validation error = %d, errno = %d", ret, errno);
		close(pw_driver);
		return -1;
	}
	LOGD("OK");

	//Initialize command checking
	LOGD("Try initialize command...");
	OSI_delay(10);

	ret = write(tr_driver, nci_initializ_cmd, 3 );
	if (ret != 3)
	{
		LOGD("failed to send initialize command.ret=%d", ret);
		close(pw_driver);
		return -1;
	}
	LOGD("OK\nValidate initialize command...");

	OSI_delay(10);

	memset(rx_buff, 0, sizeof(rx_buff));
	ret = read(tr_driver, rx_buff, 23);
	if (ret != 23)
	{
		LOGD("Read initialize response error ret = %d, errno = %d", ret, errno);
		close(pw_driver);
		return -1;
	}
	if(rx_buff[0] != 0x40 || rx_buff[1] != 0x01 || rx_buff[2] != 0x14 || rx_buff[3] != 0x00)
	{
		LOGD("Initialze command validation error = %d, errno = %d", ret, errno);
		close(pw_driver);
		return -1;
	}
	unsigned char major_ver = (unsigned char)((rx_buff[19] >> 4) & 0x0f);
	unsigned char minor_ver = (unsigned char)((rx_buff[19]) & 0x0f);
	unsigned short build_info_high = (unsigned short)(rx_buff[20]);
	unsigned short build_info_low = (unsigned short)(rx_buff[21]);

	LOGD("OK\nF/W version: %d.%d.%d", major_ver, minor_ver, ((build_info_high * 0x100) + build_info_low));
	LOGD("done! close device.");
	close(pw_driver);

	return 0;
}

// delay n ms
void OSI_delay (uint32_t timeout)
{
	struct timespec delay;
	int err;

	delay.tv_sec = timeout / 1000;
	delay.tv_nsec = 1000 * 1000 * (timeout % 1000);

	do
	{
		err = nanosleep(&delay, &delay);
	}
	while (err < 0 && errno == EINTR);
}

int test_nfc_start(void)
{

	pthread_t t1;
	int ret;

	nfc_result = -1;
	ui_fill_locked();
	ui_show_title(MENU_TEST_NFC);
	text_row = 2;
	gr_flip();

	ui_set_color(CL_WHITE);
	text_row = ui_show_text(text_row, 0, NFC_DRIVER_CHECKING);

	if(check_nfc() == -1)
	{
		ui_set_color(CL_RED);
		text_row = ui_show_text(text_row, 0, TEXT_FAIL);
		gr_flip();
		LOGE("nfc driver check error");
		ret = RESULT_FAIL;
	}
	else
	{
		ui_set_color(CL_GREEN);
		text_row = ui_show_text(text_row, 0, TEXT_PASS);
		gr_flip();

		ui_set_color(CL_WHITE);
		text_row = ui_show_text(text_row+1, 0, NFC_TIPS);
		gr_flip();

		text_row = ui_show_text(text_row, 0, NFC_HAL_SCANING);
		gr_flip();

		nfc_thread_run = 1;
		pthread_create(&t1, NULL, (void*)check_nfc_hal, NULL);
		ui_handle_button(NULL, NULL,TEXT_FAIL);
		nfc_thread_run=0;
		pthread_join(t1, NULL);

		if(nfc_result == -1)
		{
			ret = RESULT_FAIL;
			ui_set_color(CL_RED);
			text_row = ui_show_text(text_row, 0, tag_result);
			ui_show_text(text_row+1, 0, TEXT_TEST_FAIL);
		}
		else
		{
			ret = RESULT_PASS;
			ui_set_color(CL_GREEN);
			text_row = ui_show_text(text_row, 0, tag_result);
			ui_show_text(text_row+1, 0, TEXT_TEST_PASS);
		}

		gr_flip();
	}

	save_result(CASE_TEST_NFC, ret);

	sleep(1);
	return ret;
}