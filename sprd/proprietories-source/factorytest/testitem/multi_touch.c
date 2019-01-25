#include "testitem.h"
#include "math.h"

typedef struct _tp_position		//save a touch event info
{
	int x;
	int y;
	int id;
	int slot;		// the no. of touch
}tp_position;

tp_position cur_posA;
tp_position cur_posB;
tp_position last_posA;
tp_position last_posB;
tp_position pos_temp;

int radius;

enum Enums
{
	START,
	POINT_A,
	POINT_B,
	VALID,
	TYPE_A,		//Protocol type A
	TYPE_B,		//Protocol type B
	UNKNOW,
};

extern int width;
extern int height;
extern int tp_width;
extern int tp_height;
extern char SENSOR_DEV_NAME[SENSOR_NUM][BUF_LEN];

static int multi_tp_thread_run = 0;
extern int tp_flag;

int the_next_signal = START;
float ratio_x = 1.0;
float ratio_y = 1.0;

void show_tips();
void show_box();
int distance_two_points(tp_position, tp_position);

#define min(x,y) (x) < (y) ? (x):(y)
#define max(x,y) (x) > (y) ? (x):(y)
#define test_bit(bit, array)    (array[bit/8] & (1<<(bit%8)))

int text_pos_y;		//the tips position
int max_dist;
int cur_dist;
int threslod;			// the distance threslod of success
char multi_tp_succ;

void init_point(tp_position *point)
{
	point->id = -1;
	point->x = -1;
	point->y = -1;
	point->slot = -1;
}

/*
if touch two point: SYN_MT_REPORT(step0) ---->SYN_MT_REPORT(step1) ---->SYN_REPORT(step2)
if touch one point: SYN_MT_REPORT(step0) ---->SYN_REPORT(step2)
*/
int multitouch_handle_event_typeA(struct input_event *event)
{
	static char step = 0;
	if(event->type == EV_ABS)
	{
		if(ABS_MT_POSITION_X == event->code)
		{
			pos_temp.x = event->value * ratio_x;
			pos_temp.id = VALID;			// type A, no id
		}
		else if(ABS_MT_POSITION_Y == event->code)
		{
			pos_temp.y = event->value * ratio_y;
			pos_temp.id = VALID;
		}
	}
	else if(event->type == EV_SYN)
	{
		if(SYN_MT_REPORT == event->code)			//after report one of point
		{
			if(step == 0)
				cur_posA = pos_temp;
			else if(step == 1)
				cur_posB = pos_temp;
			else
				LOGD("data error !");

			init_point(&pos_temp);
			step++;
		}
		else if(SYN_REPORT == event->code)			//finish report all point
		{
			if(step == 2)				//if touch two fingers
			{
				if(cur_posA.id == VALID && cur_posB.id == VALID)
				{
					cur_dist =  distance_two_points(cur_posA, cur_posB);
					if(cur_dist > max_dist)
						max_dist = cur_dist;
					else if((max_dist - cur_dist) >= threslod && (max_dist - cur_dist) >= max_dist/2)
					{
						multi_tp_thread_run = 0;
						multi_tp_succ = 1;
						LOGD("multi_touch successfull");
					}
					show_box();
					last_posA.x = cur_posA.x;
					last_posA.y = cur_posA.y;
					last_posB.x = cur_posB.x;
					last_posB.y = cur_posB.y;
				}
			}
			else				//if touch one finger
			{
				init_point(&cur_posA);
				init_point(&cur_posB);
				max_dist = 0;
			}
			step = 0;
		}
	}
	return 0;
}

int multitouch_handle_event_typeB(struct input_event *event)
{
	if(event->type == EV_ABS)
	{
		switch(event->code)
		{
			case ABS_MT_TRACKING_ID:
				if(POINT_A == the_next_signal && (event->value) < 0)
				{
					init_point(&cur_posA);
					LOGD("reless A");
				}
				else if(POINT_B == the_next_signal && (event->value) < 0)
				{
					init_point(&cur_posB);
					LOGD("reless B");
				}
				else if(START == the_next_signal)		// touch the first finger(regard as POINT_A)
				{
					LOGD("first point regard as POINT_A");
					if((event->value) < 0)
					{
						init_point(&cur_posA);
						LOGD("reless A");
					}
				}
				break;
			case ABS_MT_POSITION_X:
				if(POINT_A == the_next_signal)
				{
					cur_posA.x = event->value * ratio_x;
				}
				else if(POINT_B == the_next_signal)
				{
					cur_posB.x = event->value * ratio_x;
				}
				else if(START == the_next_signal)
				{
					cur_posA.slot = 0;
					cur_posA.x = event->value * ratio_x;
				}
				break;
			case ABS_MT_POSITION_Y:
				if(POINT_A == the_next_signal)
				{
					cur_posA.y = event->value * ratio_y;
				}
				else if(POINT_B == the_next_signal)
				{
					cur_posB.y = event->value * ratio_y;
				}
				else if(START == the_next_signal)
				{
					cur_posA.slot = 0;
					cur_posA.y = event->value * ratio_y;
				}
				break;
			case ABS_MT_SLOT:
				if(0 == event->value)
				{
					cur_posA.slot = 0;
					the_next_signal = POINT_A;
				}
				else if(1 == event->value)
				{
					cur_posB.slot = 1;
					the_next_signal = POINT_B;
				}
			default:
				break;
		}
	}
	else if(event->type == EV_SYN)
	{
		if((cur_posA.slot < 0) && (cur_posB.slot < 0))			// press none finger
		{
			the_next_signal = START;
			max_dist = 0;
			return 0;
		}
		else if((cur_posA.slot >= 0) && (cur_posB.slot >= 0))			// press two finger
		{
			cur_dist =  distance_two_points(cur_posA, cur_posB);
			if(cur_dist > max_dist)
				max_dist = cur_dist;
			else if((max_dist - cur_dist) >= threslod && (max_dist - cur_dist) >= max_dist/3)
			{
				multi_tp_thread_run = 0;
				multi_tp_succ = 1;
				the_next_signal = START;
			}

			show_box();
			last_posA.x = cur_posA.x;
			last_posA.y = cur_posA.y;
			last_posB.x = cur_posB.x;
			last_posB.y = cur_posB.y;
		}
		else	if((cur_posA.slot == 0) && (cur_posB.slot ==-1))			// press A finger
		{
			max_dist = 0;
		}
	}
	return 0;
}

static bool containsNonZeroByte(const uint8_t* array, uint32_t startIndex, uint32_t endIndex)
{
	const uint8_t* end = array + endIndex;
	array += startIndex;
	while (array != end)
	{
		if (*(array++) != 0)
		{
			return true;
		}
	}
	return false;
}

int check_driver_type(int fd)
{
	int type = UNKNOW;
	uint8_t absBitmask[(ABS_MAX + 1) / 8];

	LOGD("check_tye enter");
	ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(absBitmask)), absBitmask);

	// Is this a multi-touch driver?
	if (test_bit(ABS_MT_POSITION_X, absBitmask) && test_bit(ABS_MT_POSITION_Y, absBitmask))
	{
		LOGD("see ABS_MT_POSITION_X and ABS_MT_POSITION_Y");

		// Is this a type_B multi-touch driver?
		if(test_bit(ABS_MT_TRACKING_ID, absBitmask) || test_bit(ABS_MT_SLOT, absBitmask))
		{
			type = TYPE_B;
			LOGD("see ABS_MT_TRACKING_ID || ABS_MT_SLOT this is TYPE_B");
		}
		else			//is type_A driver
		{
			type = TYPE_A;
			LOGD("this is TYPE_A");
		}
	}
	else
	{
		LOGE("check_driver_type error");
	}

	return type;
}

void* multitouch_thread()
{
	int ret;
	fd_set rfds;
	struct input_event event;
	struct timeval timeout;
	struct input_absinfo absinfo;
	int fd = -1;
	int type = UNKNOW;

	if(strlen(SENSOR_DEV_NAME[SENSOR_TS]))
	{
		fd = get_sensor_name(SENSOR_DEV_NAME[SENSOR_TS]);
		if(fd < 0)
		{
			LOGD("mmitest open %s tp fail",SENSOR_DEV_NAME[SENSOR_TS]);
			goto exit;
		}
	}
	else
	{
		LOGD("mmitest get tp event %s failed",SENSOR_DEV_NAME[SENSOR_TS]);
		goto exit;
	}

	init_point(&cur_posA);
	init_point(&cur_posB);

	if(ioctl(fd, EVIOCGABS(ABS_MT_POSITION_X), &absinfo))
	{
		LOGE("mmitest can not get absinfo\n");
		goto exit;
	}
	tp_width = absinfo.maximum;
	ratio_x = (float)width / tp_width;

	if(ioctl(fd, EVIOCGABS(ABS_MT_POSITION_Y), &absinfo))
	{
		LOGE("mmitest can not get absinfo");
		goto exit;
	}
	tp_height = absinfo.maximum;
	ratio_y = (float)height / tp_height;

	type = check_driver_type(fd);

	LOGD("mmitest: ratio_x = %f  ratio_y = %f", ratio_x,ratio_y);
	LOGD("mmitest [%s]tp width=%d, height=%d", __func__, tp_width, tp_height);

	while(multi_tp_thread_run==1)
	{
		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);
		timeout.tv_sec=1;
		timeout.tv_usec=0;

		//waiting for touch
		ret = select(fd+1, &rfds, NULL, NULL, &timeout);
		if(ret > 0)
		{
			if(FD_ISSET(fd, &rfds))
			{
				ret = read(fd, &event, sizeof(event));
				if (ret == sizeof(event))
				{
					switch(type)
					{
						case TYPE_A:
							multitouch_handle_event_typeA(&event);
							break;
						case TYPE_B:
							multitouch_handle_event_typeB(&event);
							break;
						default :
							type = check_driver_type(fd);
							break;
					}
				}
			}
		}
	}

exit:
	if(multi_tp_succ)
		ui_push_result(RL_PASS);
	else
		ui_push_result(RL_FAIL);

	if(fd >= 0)
		close(fd);

	return NULL;
}

void show_tips()
{
	tp_position init_box_A, init_box_B;

	init_box_A.x = width-(height >>3);
	init_box_A.y = height/4;
	init_box_B.x = height >>3;
	init_box_B.y = height - (height >>3);

	ui_fill_locked();
	ui_show_title(MENU_TEST_MULTI_TOUCH);
	ui_set_color(CL_GREEN);
	ui_show_text(2, 0, TEXT_MULTI_TOUCH_START);

	ui_set_color(CL_YELLOW);
	gr_fill(init_box_A.x-radius,init_box_A.y-radius,init_box_A.x+radius, init_box_A.y+radius);
	gr_fill(init_box_B.x-radius,init_box_B.y-radius,init_box_B.x+radius, init_box_B.y+radius);

	gr_flip();

	text_pos_y = LINE_HEIGHT + TITLE_HEIGHT;
	last_posA.x = init_box_A.x;
	last_posA.y = init_box_A.y;
	last_posB.x = init_box_B.x;
	last_posB.y = init_box_B.y;
}

int distance_two_points(tp_position A, tp_position B)
{
	return (int)sqrt((A.x-B.x)*(A.x-B.x)+(A.y-B.y)*(A.y-B.y));
}

void show_box()
{
	int x0, y0,x1,y1;

	ui_set_color(CL_SCREEN_BG);
	LOGD("(%d,%d)->(%d,%d)",last_posA.x,last_posA.y,cur_posA.x,cur_posA.y);
	LOGD("(%d,%d)->(%d,%d)",last_posB.x,last_posB.y,cur_posB.x,cur_posB.y);

	if((cur_posA.x > 0) && (cur_posA.y < 0))
		cur_posA.y = last_posA.y;
	else if((cur_posA.x < 0) && (cur_posA.y > 0))
		cur_posA.x = last_posA.x;

	if((cur_posB.x > 0) && (cur_posB.y < 0))
		cur_posB.y = last_posB.y;
	else if((cur_posB.x < 0) && (cur_posB.y > 0))
		cur_posB.x = last_posB.x;

	if((last_posA.x > 0) && (last_posA.y > 0))
	{
		x0 = max(last_posA.x - radius, 0);
		y0 = max(last_posA.y - radius, 0);
		y0 = max(y0,text_pos_y);
		x1 = min(last_posA.x + radius, width);
		y1 = min(last_posA.y + radius, height);
		gr_fill(x0, y0, x1, y1);
	}

	if((last_posB.x > 0) && (last_posB.y > 0))
	{
		x0 = max(last_posB.x-radius, 0);
		y0 = max(last_posB.y-radius, 0);
		y0 = max(y0,text_pos_y);
		x1 = min( last_posB.x+radius, width);
		y1 = min(last_posB.y+radius, height);
		gr_fill(x0, y0, x1, y1);
	}

	if(multi_tp_succ == 1)
	{
		ui_set_color(CL_GREEN);
		ui_show_text(3, 0, TEXT_PASS);
	}
	else
		ui_set_color(CL_YELLOW);

	if((cur_posA.x > 0) && (cur_posA.y > 0))
	{
		x0 = max(cur_posA.x-radius, 0);
		y0 = max(cur_posA.y-radius, 0);
		y0 = max(y0,text_pos_y);
		x1 = min( cur_posA.x+radius, width);
		y1 = min(cur_posA.y+radius, height);
		gr_fill(x0, y0, x1, y1);
	}

	if((cur_posB.x > 0) && (cur_posB.y > 0))
	{
		x0 = max(cur_posB.x-radius, 0);
		y0 = max(cur_posB.y-radius, 0);
		y0 = max(y0,text_pos_y);
		x1 = min( cur_posB.x+radius, width);
		y1 = min(cur_posB.y+radius, height);
		gr_fill(x0, y0, x1, y1);
	}

	gr_flip();
}

int test_multi_touch_start(void)
{
	pthread_t multi_touch_t;
	int ret = 0;
	max_dist = 0;
	multi_tp_succ = 0;

	width = gr_fb_width();
	height = gr_fb_height();

	radius = height / 20;
	threslod = 2*radius;

	tp_flag = 1;
	gr_tp_flag(tp_flag);

	show_tips();
	multi_tp_thread_run=1;

	pthread_create(&multi_touch_t, NULL, multitouch_thread, NULL);
	ret = ui_handle_button(NULL,NULL,NULL);
	multi_tp_thread_run=0;

	tp_flag = 0;
	gr_tp_flag(tp_flag);
	pthread_join(multi_touch_t, NULL);

	save_result(CASE_TEST_MULTITOUCH,ret);
	usleep(500 * 1000);

	return ret;
}
