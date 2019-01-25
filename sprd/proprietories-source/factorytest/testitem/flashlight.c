#include "testitem.h"

volatile bool flashthread_run = true;

void open_flash_light(int i)
{
	int ret = 0;
#ifndef HAL_FLASH_FUN
	ret = system("echo 0x30 > /sys/class/misc/sprd_flash/test") ? -1 : 0;		//for sprdroid 7.0
	if( -1 == ret )
		ret = system("echo 0x02 > /sys/devices/virtual/flash_test/flash_test/flash_value") ? -1 : 0;		//for sprdroid 6.0
#else
       if (i == 0 || i == -1)
        {
            ret = system("echo 0x10 > /sys/class/misc/sprd_flash/test") ? -1 : 0;
        }
       if (i == 1 || i == -1)
        {
            ret = system("echo 0x20 > /sys/class/misc/sprd_flash/test") ? -1 : 0;
        }
#endif
}

void close_flash_light(int i)
{
	int ret = 0;
#ifndef HAL_FLASH_FUN
	ret = system("echo 0x31 > /sys/class/misc/sprd_flash/test") ? -1 : 0;		//for sprdroid 7.0
	if( -1 == ret )
		ret = system("echo 0x00 > /sys/devices/virtual/flash_test/flash_test/flash_value") ? -1 : 0;		//for sprdroid 6.0
#else
       if (i == 0 || i == -1)
        {
            ret = system("echo 0x11 > /sys/class/misc/sprd_flash/test") ? -1 : 0;
        }
       if (i == 1 || i == -1)
        {
            ret = system("echo 0x21 > /sys/class/misc/sprd_flash/test") ? -1 : 0;
        }
#endif
}

void flash_thread(void)
{
	int i = 0;

	LOGD("flash thread begin...");

	if(support_result[CASE_TEST_FLASH].support == 1)		// flashing light
	{
		LOGD("flashing light");
		while(flashthread_run)
		{
			LOGD("%d is on", i%2);
			open_flash_light(i%2);
			usleep(1000*1000);
			LOGD("%d is off", i%2);
			close_flash_light(i%2);
			usleep(1000*1000);
			i++;
		}
	}
	else if(support_result[CASE_TEST_FLASH].support == 2)		//always light on
	{
		LOGD("always light on");
		while(flashthread_run)
		{
			LOGD("%d is on", i%2);
			open_flash_light(i%2);
			usleep(500*1000);
			i++;
		}
	}
    close_flash_light(-1);
	LOGD("flash thread end...");
}

int test_flashlight_start(void)
{
      pthread_t t1;
	volatile int  rtn = RL_FAIL;

	LOGD("enter flashlight test");
	ui_fill_locked();
	ui_show_title(MENU_TEST_FLASHLIGHT);

       flashthread_run = true;
	pthread_create(&t1, NULL, (void*)flash_thread, NULL);

	rtn = ui_handle_button(TEXT_PASS,NULL,TEXT_FAIL);//TEXT_GOBACK
	gr_flip();
       flashthread_run = false;
       pthread_join(t1, NULL);
       close_flash_light(-1);

	gr_flip();
	save_result(CASE_TEST_FLASHLIGHT,rtn);

	usleep(500 * 1000);
	return rtn;
}
