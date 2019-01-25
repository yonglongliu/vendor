#include "testitem.h"
#include "cutils/properties.h"

extern pthread_mutex_t tel_mutex;

extern ST_MCCMNC_NUMBER MCCMNC_DIAL_NUMBER[MCCMNC_TOTAL_NUMBER];

extern int checkSimCard();

bool isLteOnly(void)
{
    char testmode[PROPERTY_VALUE_MAX + 1];
    property_get("persist.radio.ssda.testmode", testmode, "0");
    LOGD("isLteOnly: persist.radio.ssda.testmode=%s\n", testmode);

    if (!strcmp(testmode, "3"))   // if persist.radio.ssda.testmode=3, then modem is lte only
    {
        return true;
    }
    else
    {
        return false;
    }
}

extern int s_mccmnc_id;
int getServiceNumberOfMCCMNC(int fd, char *buf, int len){
    char tmp[512];
    char* ptmp = NULL;
    char *oper = NULL;
    int mode=-1, format=-1;
    int nSec = 10;
    int i = 0;
    memset(buf, 0, len);
    while(nSec-- >= 0){
        tel_send_at(fd, "AT+COPS?", tmp, sizeof(tmp), 0);
        //strcpy(tmp, "+COPS: 0,2,\"46001\",0");
        ptmp = strstr(tmp, "COPS");
        LOGD("+COPS =%s", ptmp);
        eng_tok_start(&ptmp);
        eng_tok_nextint(&ptmp, &mode);
        LOGD("get mode =%d", mode);
        if(eng_tok_nextint(&ptmp, &format) != -1 && eng_tok_nextstr(&ptmp, &oper) != -1){
            LOGD("format = %d, oper = %s", format, oper);
            break;
        }else {
            sleep(1);
            LOGD("format = %d, oper = %s", format, oper);
        }
    }

    if (nSec >= 0){
        for(i < 0; i < s_mccmnc_id; i++){
            LOGD("find mccmnc service: %s", MCCMNC_DIAL_NUMBER[i].serviceNumber);
            if (strncmp(MCCMNC_DIAL_NUMBER[i].mccmnc, oper, strlen(MCCMNC_DIAL_NUMBER[i].mccmnc)) == 0){
                LOGD("find mccmnc service: ");
                strcpy(buf, MCCMNC_DIAL_NUMBER[i].serviceNumber);
                buf[len-1] = 0;
                return 0;
            }
        }
    }

    return -1;
}

int test_tel_start(void)
{
	int cur_row = 2;
	int ret, fd, ps_state;
	char tmp[512];
	char* ptmp = NULL;
	time_t start_time, now_time;
	char property[PROPERTY_VALUE_MAX];
	char moemd_tel_port[BUF_LEN];
	char write_buf[1024] = {0};

	ui_fill_locked();
	ui_show_title(MENU_TEST_TEL);
	ui_set_color(CL_GREEN);
	if (isLteOnly()) {
		int sim = -1;;
		cur_row = ui_show_text(cur_row, 0, TEL_TEST_CHECKSIM);
		gr_flip();
		sleep(1);
		sim = checkSimCard();
		if (sim != -1 ){
			cur_row = ui_show_text(cur_row, 0, TEL_TEST_START_LTE);
		}else{
			ret = RL_FAIL;
			cur_row = ui_show_text(cur_row, 0, TEL_TEST_NOSIM);
			gr_flip();
			sleep(3);
			goto end;
		}
	}else{
		cur_row = ui_show_text(cur_row, 0, TEL_TEST_START);
	}

	cur_row = ui_show_text(cur_row, 0, TEL_TEST_TIPS);
	gr_flip();

	property_get(PROP_MODEM_LTE_ENABLE, property, "not_find");
	if(!strcmp(property, "1"))
		snprintf(moemd_tel_port, sizeof(moemd_tel_port), "/dev/stty_lte2");
	else
		snprintf(moemd_tel_port, sizeof(moemd_tel_port), "/dev/stty_w2");

	LOGD("mmitest tel test %s", moemd_tel_port);
	fd = open(moemd_tel_port, O_RDWR);
	if(fd < 0)
	{
		LOGE("mmitest tel test failed");
		ret = RL_FAIL;
		goto end;
	}
	pthread_mutex_lock(&tel_mutex);
	tel_send_at(fd, "AT+SFUN=2", NULL, 0, 0);    //open sim card
	tel_send_at(fd, "AT+SFUN=5", NULL, 0, 0);     //close protocol
	usleep(3000*1000);
	if (isLteOnly()){
		tel_send_at(fd, "AT+SPTESTMODEM=21,254", NULL, 0, 0);
	}else {
		tel_send_at(fd, "AT+SPTESTMODEM=15,10", NULL, 0, 0);
	}
	ret = tel_send_at(fd, "AT+SFUN=4", NULL, 0, 100); //open protocol stack and wait 100s,if exceed more than 20s,we regard rregistering network fail

	pthread_mutex_unlock(&tel_mutex);
	if(ret < 0 )
	{
		ret = RL_FAIL;
		ui_set_color(CL_RED);
		ui_show_text(cur_row, 0, TEL_DIAL_FAIL);
		gr_flip();
		sleep(1);
		goto end;
	}
	start_time = time(NULL);
	for(;;)
	{
		tel_send_at(fd, "AT+CREG?", tmp, sizeof(tmp), 0);
		ptmp = strstr(tmp, "CREG");
		LOGD("+CREG =%s", ptmp);
		eng_tok_start(&ptmp);
		eng_tok_nextint(&ptmp, &ps_state);
		LOGD("get ps mode=%d", ps_state);
		if(2 == ps_state)
		{
			eng_tok_nextint(&ptmp, &ps_state);
			LOGD("get ps state=%d", ps_state);
			if((1 == ps_state) || (8 == ps_state) || (5 == ps_state))
			{
				break;
			}
		}
		sleep(2);
		now_time = time(NULL);
		if (now_time - start_time > TEL_TIMEOUT )
		{
			LOGE("mmitest tel test failed");
			ret = RL_FAIL;
			ui_set_color(CL_RED);
			ui_show_text(cur_row, 0, TEL_DIAL_FAIL);
			gr_flip();
			sleep(1);
			goto end;
		}
	}

	if (!isLteOnly()){
		ret=tel_send_at(fd, "ATD112@1,#;", NULL,0, 0);  //call 112
		cur_row = ui_show_text(cur_row, 0, TEL_DIAL_OVER);
	}else{
		char num[32] = {0};
		char buff[256] = {0};
		int res = getServiceNumberOfMCCMNC(fd, num, sizeof(num));
		if (res == -1){
			strcpy(num, "10086");
		}
		sprintf(buff, "ATD%s;", num);
		ret=tel_send_at(fd, buff, NULL,0, 0);  //call
		memset(buff, 0, sizeof(buff));
		sprintf(buff, "%s%s", num, TEL_DIAL_OVER);
		cur_row = ui_show_text(cur_row, 0, buff);
	}

	usleep(200 * 1000);

#ifdef AUDIO_DRIVER_2
	snprintf(write_buf, sizeof(write_buf) - 1, "set_mode=%d;test_out_stream_route=%d;", AUDIO_MODE_IN_CALL, AUDIO_DEVICE_OUT_SPEAKER);    //open speaker
	LOGD("write:%s", write_buf);
	SendAudioTestCmd((const uchar *)write_buf, strlen(write_buf));
#else
	tel_send_at(fd, "AT+SSAM=1", NULL, 0, 0);  //open speaker
#endif

	gr_flip();

	ret = ui_handle_button(TEXT_PASS, NULL, TEXT_FAIL);
	tel_send_at(fd, "ATH", NULL, 0, 0);             //hang up
	tel_send_at(fd, "AT", NULL, 0, 0);

#ifdef AUDIO_DRIVER_2
	snprintf(write_buf, sizeof(write_buf) - 1, "set_mode=%d;", AUDIO_MODE_NORMAL);
	LOGD("write:%s", write_buf);
	SendAudioTestCmd((const uchar *)write_buf, strlen(write_buf));
#endif

end:
	if(fd >= 0)
		close(fd);

	save_result(CASE_TEST_TEL, ret);
	return ret;
}
