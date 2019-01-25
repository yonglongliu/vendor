#include"testitem.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define READ_BUF_LEN 10240
#define CMD "pnscr -t 1 -d pn544 -f "
#define READ_MODE_FILE  "/system/etc/nfc/Reader_mode.txt"
#define SET_TIMEOUT  " -w 2"		// set timeout (second), if not set the para, default 15 mins

int text_row = 0;

int check_nfc()
{
	char cmd_buf[256];
	char *result_buf;
	char temp_buf[4096];
    char * log_tmp_buf = NULL;
	char * pos_succ = NULL;       //position, the tag of success
	char * pos_fail = NULL;       //position, the tag of fail
	char * pos_script = NULL;       //position, tag of script lunched

	int ret = 1;
	int memsize = 0;

	result_buf = (char*)malloc(sizeof(char)*READ_BUF_LEN);
	memset(result_buf, '\0', READ_BUF_LEN);

	memset(cmd_buf, '\0', sizeof(cmd_buf));
	strcat(cmd_buf, CMD);
	strcat(cmd_buf, READ_MODE_FILE);
	strcat(cmd_buf, SET_TIMEOUT);
	strcat(cmd_buf, " 2>&1");

	LOGD("nfc test cmd: %s",cmd_buf);

	FILE *file = popen(cmd_buf, "r");
	if(file == NULL)
	{
		LOGE("popen fail");
		return RESULT_FAIL;
	}

	while(!feof(file))
	{
		memset(temp_buf, '\0', sizeof(temp_buf));
		if(fread(temp_buf, sizeof(char), sizeof(temp_buf), file) <= 0)
		{
			LOGE("read command echo error!");
			break;
		}
		memsize += strlen(temp_buf);

		if(memsize >=  READ_BUF_LEN-1)
		{
			LOGE("result_buf out of memory: memsize = %d, maxsize = %d",memsize, READ_BUF_LEN);
			break;
		}
		else
			strcat(result_buf, temp_buf);
	}

	pclose(file);

    //print log line by line, LOGD limited the char size
    log_tmp_buf = result_buf;
    char * pos_enter = NULL;        //position of '\n'
    while(1)
    {
        pos_enter = strstr(log_tmp_buf, "\n");
        if(pos_enter)
        {
            memset(temp_buf, '\0', sizeof(temp_buf));
            strncpy(temp_buf, log_tmp_buf, pos_enter - log_tmp_buf);
            LOGD("pnscr_log: %s",temp_buf);
            usleep(1000);
            log_tmp_buf = pos_enter+1;
        }
        else
            break;
    }

	if(result_buf != NULL && strlen(result_buf) > 1)
	{
		pos_succ = strstr(result_buf, "Reader_mode PASSED");
		if(pos_succ == NULL)
			pos_succ = strstr(result_buf, "Data FOUND");
		pos_fail = strstr(result_buf, "FAILED");
		pos_script = strstr(result_buf, "Welcome");
	}

	if(pos_script != NULL)
	{
		LOGD("pnscr script lunch success");
		if(pos_fail != NULL)
		{
			LOGE("nfc check tag fail");
			ret = RESULT_FAIL;
		}
		else if(pos_succ != NULL)
		{
			LOGD("nfc check tag success");
			ret = RESULT_PASS;
		}
		else
		{
			LOGE("unknown error");
			ret = RESULT_FAIL;
		}
	}
	else
	{
		LOGE("pnscr script lunch fail");
		ret = RESULT_FAIL;
	}

	free(result_buf);
	return ret;
}

int test_nfc_start(void)
{
    int ret = RESULT_FAIL;

    ui_fill_locked();
    ui_show_title(MENU_TEST_NFC);
    text_row = 2;

    ui_set_color(CL_WHITE);
    text_row = ui_show_text(text_row, 0, KAIOS_NFC_CHECK);
    gr_flip();

    ret = check_nfc();

    freetype_setsize(CHAR_SIZE*2);      //set larger char size
    if(ret == RESULT_FAIL)
    {
        ui_set_color(CL_RED);
        ui_show_text(text_row + 1, 0, TEXT_FAIL);
        gr_flip();
        sleep(1);
        LOGE("nfc test fail");
    }
    else if(ret == RESULT_PASS)
    {
        ui_set_color(CL_GREEN);
        ui_show_text(text_row + 1, 0, TEXT_PASS);
        gr_flip();
        sleep(1);
        LOGD("nfc test success");
    }
    freetype_setsize(CHAR_SIZE);      //reset char size

    save_result(CASE_TEST_NFC, ret);

    sleep(1);
    return ret;
}