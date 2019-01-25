#include "testitem.h"

static int thread_run = 0;

typedef struct _tp_pos
{
	int x;
	int y;
} tp_pos;

typedef struct _area_info
{
	tp_pos left_top;
	tp_pos right_bottom;
	char drawed;
} area_info;

static tp_pos cur_pos;
static tp_pos last_pos;

#define AREA_ROW  11
#define AREA_COL  5
area_info rect_r1 [AREA_ROW];
area_info rect_r2 [AREA_ROW];
area_info rect_c1 [AREA_COL];
area_info rect_c2 [AREA_COL];

int width = 0;
int height = 0;
int tp_width = 0;
int tp_height = 0;
int rect_w = 0;
int rect_h = 0;
char tp_flag = 0;

static int rect_cnt;

void area_rectangle_check(int x, int y);

static int tp_handle_event(struct input_event *event)
{
	if(event->type == EV_ABS)
	{
		switch(event->code)
		{
		case ABS_MT_POSITION_X:
			cur_pos.x = event->value * width / tp_width;
			break;
		case ABS_MT_POSITION_Y:
			cur_pos.y = event->value * height / tp_height;

			if(last_pos.x != 0 && last_pos.y != 0 && (abs(cur_pos.x -last_pos.x) > 0 || abs(cur_pos.y -last_pos.y) > 0))
			{
				LOGD("current loction:(%d,%d)", cur_pos.x, cur_pos.y);
				ui_set_color(CL_WHITE);
				ui_draw_line_mid(last_pos.x, last_pos.y, cur_pos.x, cur_pos.y);
				area_rectangle_check(cur_pos.x, cur_pos.y);
				gr_flip();
			}
			last_pos.x = cur_pos.x;
			last_pos.y = cur_pos.y;
			break;
		}
	}
	return 0;
}

static void* tp_thread()
{
	int ret;
	int fd = -1;
	fd_set rfds;
	struct input_event event;
	struct timeval timeout;
	struct input_absinfo absinfo_x;
	struct input_absinfo absinfo_y;

	if(strlen(SENSOR_DEV_NAME[SENSOR_TS]))
	{
		fd = get_sensor_name(SENSOR_DEV_NAME[SENSOR_TS]);
		if(fd < 0)
		{
			LOGD("mmitest open %s tp fail", SENSOR_DEV_NAME[SENSOR_TS]);
			goto exit;
		}
	}
	else
	{
		LOGD("mmitest get tp event %s failed", SENSOR_DEV_NAME[SENSOR_TS]);
		goto exit;
	}

	memset(&cur_pos, 0, sizeof(tp_pos));
	memset(&last_pos, 0, sizeof(tp_pos));

	if(ioctl(fd, EVIOCGABS(ABS_MT_POSITION_X), &absinfo_x))
	{
		LOGD("mmitest can not get absinfo");
		goto exit;
	}

	if(ioctl(fd, EVIOCGABS(ABS_MT_POSITION_Y), &absinfo_y))
	{
		LOGD("mmitest can not get absinfo");
		goto exit;
	}

	tp_width = absinfo_x.maximum;
	tp_height = absinfo_y.maximum;
	LOGD("mmitest tp_width=%d, tp_height=%d", tp_width, tp_height);

	while(thread_run == 1)
	{
		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;

		//waiting for touch
		ret = select(fd + 1, &rfds, NULL, NULL, &timeout);
		if(ret < 0)
		{
			LOGD("mmitest [%s]: error from select (%d): %s", __FUNCTION__, fd, strerror(errno));
			continue;
		}
		else if(ret == 0)
		{
			LOGD("mmitest [%s]: timeout, %ld", __FUNCTION__, timeout.tv_sec);
			continue;
		}
		else
		{
			if(FD_ISSET(fd, &rfds))
			{
				ret = read(fd, &event, sizeof(event));
				if (ret == sizeof(event))
				{
					//LOGD("mmitest [%s]: timeout, %d", __FUNCTION__, sizeof(event));
					tp_handle_event(&event);			//handle key pressed
				}
				else
				{
					LOGD("sizeof(event) = %d, ret = %d", sizeof(event), ret);
				}
			}
		}
	}
exit:
	if(FD_ISSET(fd, &rfds))
		FD_CLR(fd, & rfds);
	ui_push_result(RL_FAIL);
	if(fd >= 0)
		close(fd);
	return NULL;
}

void draw_rectangle(area_info rect)
{
	if(rect.drawed == 0)
		ui_set_color(CL_RED);
	else
		ui_set_color(CL_GREEN);

	ui_draw_line_mid(rect.left_top.x, rect.left_top.y, rect.right_bottom.x, rect.left_top.y);
	ui_draw_line_mid(rect.left_top.x, rect.left_top.y, rect.left_top.x, rect.right_bottom.y);
	ui_draw_line_mid(rect.right_bottom.x, rect.right_bottom.y, rect.right_bottom.x, rect.left_top.y);
	ui_draw_line_mid(rect.right_bottom.x, rect.right_bottom.y, rect.left_top.x, rect.right_bottom.y);
}

void area_rectangle_init(void)
{
	int i = 0;
	int start_width = (width % AREA_COL) / 2;
	int start_height = (height % AREA_ROW) / 2;

	rect_cnt = 0;

	LOGD("width=%d, height=%d", width, height);
	LOGD("rect_w=%d, rect_h=%d", rect_w, rect_h);

	for(i = 0; i < AREA_ROW; i++)
	{
		rect_r1[i].left_top.x = 0;
		rect_r2[i].left_top.x = rect_w * (AREA_COL - 1) + start_width;
		if(i == 0)
		{
			rect_r1[i].left_top.y = 0;
			rect_r2[i].left_top.y = 0;
		}
		else
		{
			rect_r1[i].left_top.y = i * rect_h + start_height;
			rect_r2[i].left_top.y = i * rect_h + start_height;
		}

		rect_r1[i].right_bottom.x = rect_w + start_width;
		rect_r2[i].right_bottom.x = width - 1;
		if( i == AREA_ROW - 1)
		{
			rect_r1[i].right_bottom.y = height - 1;
			rect_r2[i].right_bottom.y = height - 1;
		}
		else
		{
			rect_r1[i].right_bottom.y = (i + 1) * rect_h + start_height;
			rect_r2[i].right_bottom.y = (i + 1) * rect_h + start_height;
		}

		rect_r1[i].drawed = 0;
		rect_r2[i].drawed = 0;
		draw_rectangle(rect_r1[i]);
		draw_rectangle(rect_r2[i]);
	}

	for(i = 0; i < AREA_COL; i++)
	{
		if( i == 0 )
		{
			rect_c1[i].left_top.x = 0;
			rect_c2[i].left_top.x = 0;
		}
		else
		{
			rect_c1[i].left_top.x = i * rect_w + start_width;
			rect_c2[i].left_top.x = i * rect_w + start_width;
		}
		rect_c1[i].left_top.y = 0;
		rect_c2[i].left_top.y = (rect_h * (AREA_ROW - 1)) + start_height;

		if( i == AREA_COL - 1 )
		{
			rect_c1[i].right_bottom.x = width - 1;
			rect_c2[i].right_bottom.x = width - 1;
		}
		else
		{
			rect_c1[i].right_bottom.x = (i + 1) * rect_w + start_width;
			rect_c2[i].right_bottom.x = (i + 1) * rect_w + start_width;
		}

		rect_c1[i].right_bottom.y = rect_h + start_height;
		rect_c2[i].right_bottom.y = height - 1;

		rect_c1[i].drawed = 0;
		rect_c2[i].drawed = 0;
		draw_rectangle(rect_c1[i]);
		draw_rectangle(rect_c2[i]);
	}

	gr_flip();
}

void area_rectangle_check(int x, int y)
{
	int i = 0;

	if(x >= 0 && x <= rect_w)
	{
		i = y / rect_h;
		if((i < AREA_ROW) && (rect_r1[i].drawed == 0))
		{
			rect_r1[i].drawed = 1;
			draw_rectangle(rect_r1[i]);
			rect_cnt++;
			LOGD("rect_r1 drawed, rect_cnt=%d", rect_cnt);
		}
	}
	else if (x >= (width - rect_w) && x <= (width - 1))
	{
		i = y / rect_h;
		if((i < AREA_ROW) && (rect_r2[i].drawed == 0))
		{
			rect_r2[i].drawed = 1;
			draw_rectangle(rect_r2[i]);
			rect_cnt++;
			LOGD("rect_r2 drawed, rect_cnt=%d", rect_cnt);
		}
	}
	else
	{
		i = x / rect_w;
		if(y >= 0 && y <= rect_h)
		{
			if((i < AREA_COL) && (rect_c1[i].drawed == 0))
			{
				rect_c1[i].drawed = 1;
				draw_rectangle(rect_c1[i]);
				rect_cnt++;
				LOGD("rect_c1 drawed, rect_cnt=%d", rect_cnt);
			}
		}
		else if(y >= (rect_h * (AREA_ROW - 1)) && y <= (rect_h * (AREA_ROW)))
		{
			if((i < AREA_COL) && (rect_c2[i].drawed == 0))
			{
				rect_c2[i].drawed = 1;
				draw_rectangle(rect_c2[i]);
				rect_cnt++;
				LOGD("rect_c2 drawed, rect_cnt=%d", rect_cnt);
			}
		}
	}
	if(rect_cnt >= (AREA_ROW + AREA_COL - 2) * 2)
	{
		thread_run = 0;
		ui_push_result(RL_PASS);
	}
}

int test_tp_start(void)
{
	pthread_t tp_t;
	int ret = -1;
	width = gr_fb_width();
	height = gr_fb_height();
	tp_width = width;
	tp_height = height;
	rect_w = width / AREA_COL;
	rect_h = height / AREA_ROW;

	tp_flag  = 1;
	gr_tp_flag(tp_flag);
	ui_fill_locked();
	area_rectangle_init();
	thread_run = 1;

	pthread_create(&tp_t, NULL, (void*)tp_thread, NULL);

	do
	{
		ret = ui_wait_button(NULL, NULL, NULL);
	}
	while(KEY_VOLUMEDOWN != ret && KEY_VOLUMEUP != ret && KEY_POWER != ret);

	thread_run = 0;
	pthread_join(tp_t, NULL);

	if(KEY_VOLUMEDOWN == ret)
		save_result(CASE_TEST_TP, RL_PASS);
	else if(KEY_POWER == ret)
		save_result(CASE_TEST_TP, RL_FAIL);

	tp_flag  = 0;
	gr_tp_flag(tp_flag);

	usleep(800 * 1000);
	return ret;
}
