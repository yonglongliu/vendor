/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <linux/input.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/reboot.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <linux/input.h>

#include "common.h"
#include "minui.h"
#include "ui.h"
#include "testitem.h"

static pthread_mutex_t gUpdateMutex = PTHREAD_MUTEX_INITIALIZER;
static gr_surface gBackgroundIcon[NUM_BACKGROUND_ICONS];

gr_surface gCurrentIcon = NULL;

int max_rows = 0;       //max lines screen can display
int max_items;		//max menu items screen can display

// Key event input queue
static pthread_mutex_t key_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t key_queue_cond = PTHREAD_COND_INITIALIZER;
static int key_queue[256], key_queue_len = 0;
static volatile char key_pressed[KEY_MAX + 1];

extern char autotest_flag;
extern int usbin_state;
static int _utf8_strlen(const char* str);
int ui_start_menu(char* title, const char** items, int sel);
void parse_title(char * buf, char gap, char* value);

// Clear the screen and draw the currently selected background icon (if any).
// Should only be called with gUpdateMutex locked.
void draw_background_locked(gr_surface icon)
{
	ui_set_color(CL_SCREEN_BG);
	gr_fill(0, 0, gr_fb_width(), gr_fb_height());

	if (icon)
	{
		int iconWidth = gr_get_width(icon);
		int iconHeight = gr_get_height(icon);
		int iconX = (gr_fb_width() - iconWidth) / 2;
		int iconY = (gr_fb_height() - iconHeight) / 2;
		gr_blit(icon, 0, 0, iconWidth, iconHeight, iconX, iconY);
	}
}

static void draw_text_line(int row, const char* text)
{
    if (text[0] != '\0')
        gr_text(0, row*ITEM_HEIGHT + TITLE_HEIGHT+(CHAR_SIZE-ITEM_HEIGHT)/2, text);
}

// Redraw everything on the screen.  Does not flip pages.
// Should only be called with gUpdateMutex locked.
static void draw_screen_locked(void)
{
	pthread_mutex_lock(&gUpdateMutex);
	draw_background_locked(gCurrentIcon);
	pthread_mutex_unlock(&gUpdateMutex);
}

static void update_screen_locked(void)
{
	draw_screen_locked();
	gr_flip();
}

void draw_menu(menu_list *m)
{
    int i = 0;
    int line_row = 0;

    pthread_mutex_lock(&gUpdateMutex);
    draw_background_locked(gCurrentIcon);
    pthread_mutex_unlock(&gUpdateMutex);

    ui_set_color(CL_SCREEN_BG);
    gr_fill(0, 0, gr_fb_width(), gr_fb_height());

    ui_show_title(m->title);
    if(m->page_total > 1)
        ui_show_page(m->page_current, m->page_total);

    line_row = 1;
    for (i = m->item_top_on_page; i < m->item_end_on_page; i++)
    {
        if (i == m->item_select)		//hight light the selected item
        {
            ui_set_color(CL_MENU_HL_BG);
            gr_fill(0, TITLE_HEIGHT + (line_row - 1) * ITEM_HEIGHT, gr_fb_width(), TITLE_HEIGHT + line_row * ITEM_HEIGHT + 1);

            if(strstr(m->menu[i].title, TEXT_PASS))
                ui_set_color(CL_GREEN);
            else if(strstr(m->menu[i].title, TEXT_FAIL))
                ui_set_color(CL_RED);
            else
                ui_set_color(CL_TITLE_FG);

            draw_text_line(line_row, m->menu[i].title);
            line_row++;
            continue;
        }
        else if(strstr(m->menu[i].title, TEXT_PASS))
            ui_set_color(CL_GREEN);
        else if(strstr(m->menu[i].title, TEXT_FAIL))
            ui_set_color(CL_RED);
        else
            ui_set_color(CL_TITLE_FG);

        draw_text_line(line_row, m->menu[i].title);
        line_row++;
    }

    ui_set_color(CL_SCREEN_FG);

    //draw line separator
    gr_fill(0, TITLE_HEIGHT + (line_row - 1)*ITEM_HEIGHT + 2, gr_fb_width(), TITLE_HEIGHT + (line_row - 1)*ITEM_HEIGHT + 4);

    if((m->page_total) <= 1 && (m->self) == ROOT_MENU)
        ui_show_button(NULL, NULL, NULL);
    else if((m->page_total) <= 1)
        ui_show_button(NULL, NULL, TEXT_GOBACK);
    else if((m->self) == ROOT_MENU)
        ui_show_button(NULL, TEXT_NEXT_PAGE, NULL);
    else
        ui_show_button(NULL, TEXT_NEXT_PAGE, TEXT_GOBACK);

    gr_flip();
    usleep(100 * 1000);
}

extern void touch_handle_input(int, struct input_event*);

// Reads input events, handles special hot keys, and adds to the key queue.
static void *input_thread()
{
	int rel_sum = 0;
	int fake_key = 0;
	int fd = -1;

	pthread_detach(pthread_self()); 	//free by itself
	for (;;)
	{
		// wait for the next key event
		struct input_event ev;
		do
		{
			fd = ev_get(&ev, 0);
			SPRD_DBG("eventtype %d,eventcode %d,eventvalue %d", ev.type, ev.code, ev.value);
			if(fd != -1)
				touch_handle_input(fd, &ev);

			if (ev.type == EV_SYN)
				continue;
			else if (ev.type == EV_REL)
			{
				if (ev.code == REL_Y)
				{
					// accumulate the up or down motion reported by
					// the trackball.  When it exceeds a threshold
					// (positive or negative), fake an up/down
					// key event.
					rel_sum += ev.value;
					if (rel_sum > 3)
					{
						fake_key = 1;
						ev.type = EV_KEY;
						ev.code = KEY_DOWN;
						ev.value = 1;
						rel_sum = 0;
					}
					else if (rel_sum < -3)
					{
						fake_key = 1;
						ev.type = EV_KEY;
						ev.code = KEY_UP;
						ev.value = 1;
						rel_sum = 0;
					}
				}
			}
			else
				rel_sum = 0;
		}
		while (ev.type != EV_KEY || ev.code > KEY_MAX);

		pthread_mutex_lock(&key_queue_mutex);
		if (!fake_key)
		{
			// our "fake" keys only report a key-down event (no
			// key-up), so don't record them in the key_pressed
			// table.
			key_pressed[ev.code] = ev.value;
			SPRD_DBG("ev.value=%d", ev.value);
		}
		fake_key = 0;
		const int queue_max = sizeof(key_queue) / sizeof(key_queue[0]);
		if (ev.value > 0 && key_queue_len < queue_max)
		{
			key_queue[key_queue_len++] = ev.code;
			pthread_cond_signal(&key_queue_cond);
		}
		pthread_mutex_unlock(&key_queue_mutex);
	}
	return NULL;
}

void ui_push_result(int result)
{
	pthread_mutex_lock(&key_queue_mutex);
	const int queue_max = sizeof(key_queue) / sizeof(key_queue[0]);
	if (key_queue_len < queue_max)
	{
		switch(result)
		{
		case RL_NA: //cancel
			key_queue[key_queue_len++] = KEY_BACK;
			break;
		case RL_FAIL: //fail
			key_queue[key_queue_len++] = KEY_POWER;
			break;
		case RL_PASS: //ok
			key_queue[key_queue_len++] = KEY_VOLUMEDOWN;
			break;
		default: //cancel
			//	key_queue[key_queue_len++] = KEY_BACK;
			break;
		}
		pthread_cond_signal(&key_queue_cond);
	}
	pthread_mutex_unlock(&key_queue_mutex);
}

void ui_init(void)
{
    pthread_t t;
    int lcd_w = 0, lcd_h = 0;
    LOGD("mmitest ui_init!!");

    gr_init();
    ev_init();

    lcd_w = gr_fb_width();
    lcd_h = gr_fb_height();
    LOGD("mmitest lcd_size=%d*%d", lcd_w, lcd_h);

    CHAR_SIZE = ((lcd_w < lcd_h) ? lcd_w : lcd_h) / 20;     //display 20 words
    LINE_HEIGHT = 1.3 * CHAR_SIZE;
    ITEM_HEIGHT = 2.2 * CHAR_SIZE;
    TITLE_HEIGHT = 3.3 * CHAR_SIZE;

    freetype_setsize(CHAR_SIZE);

#ifdef SPRD_VIRTUAL_TOUCH
    button_width = ((lcd_w < lcd_h) ? lcd_w : lcd_h) / 4;
    button_height = button_width * 0.8;
#else
    button_width = 0;
    button_height = 0;
#endif

    max_items = (lcd_h - button_height - TITLE_HEIGHT) / ITEM_HEIGHT;
    max_rows = (lcd_h - button_height - TITLE_HEIGHT) / LINE_HEIGHT;

    LOGD("mmitest max_rows=%d,max_items=%d", max_rows, max_items);
    pthread_create(&t, NULL, input_thread, NULL);

}

void ui_set_background(int icon)
{
	pthread_mutex_lock(&gUpdateMutex);
	gCurrentIcon = gBackgroundIcon[icon];
	update_screen_locked();
	pthread_mutex_unlock(&gUpdateMutex);
}

int ui_wait_key()
{
    int ret = 0;
    int key = -1;
    struct timespec ntime;
    ntime.tv_sec = time(NULL);
    ntime.tv_nsec = 0;
    pthread_mutex_lock(&key_queue_mutex);
    while (key_queue_len == 0)
    {
        ntime.tv_sec = ntime.tv_sec + 20;
        ret = pthread_cond_timedwait(&key_queue_cond, &key_queue_mutex, &ntime);
        if(ret == ETIMEDOUT)
            break;
    }

    if(0 == ret)
    {
        key = key_queue[0];
        memcpy(&key_queue[0], &key_queue[1], sizeof(int) * --key_queue_len);
    }
    else
        key = ret;

#ifdef Full_keyboard
    switch (key)
    {
    case 139:
        key = KEY_VOLUMEUP;
        break;
    case 158:
        key = KEY_VOLUMEDOWN;
        break;
    default:
        break;
    }
#endif

    pthread_mutex_unlock(&key_queue_mutex);
    return key;
}

void ui_clear_key_queue()
{
	pthread_mutex_lock(&key_queue_mutex);
	key_queue_len = 0;
	pthread_mutex_unlock(&key_queue_mutex);
}

int device_handle_key(int key_code)
{
	if((key_code >= KEY_VIR_ITEMS) && (key_code < KEY_VIR_ITEMS + max_items))
	{
		LOGD("press item No = %d", key_code);
		return key_code;
	}

	switch (key_code)
	{
	case KEY_VOLUMEDOWN:
		LOGD("press KEY_VOLUMEDOWN, the action: HIGHLIGHT_DOWN");
		return HIGHLIGHT_DOWN;
	case KEY_VOLUMEUP:
		LOGD("press KEY_VOLUMEUP, the action: SELECT_ITEM");
		return SELECT_ITEM;
	case KEY_POWER:
		LOGD("press KEY_POWER, the action: GO_BACK");
		return GO_BACK;
	case KEY_VIR_NEXT_PAGE:
		LOGD("press KEY_VIR_NEXT_PAGE, the action: NEXT_PAGE");
		return NEXT_PAGE;
	case KEY_VIR_FAIL:
		LOGD("press KEY_VIR_FAIL, the action: GO_HOME");
		return GO_HOME;
	default:
		LOGD("default, the action: NO_ACTION");
		return NO_ACTION;
	}

	return NO_ACTION;
}

int menu_change(menu_list* m, int action)
{
	if (action < 0)
	{
		switch (action)
		{
		case HIGHLIGHT_DOWN:
			LOGD("HIGHLIGHT_DOWN item No = %d", m->item_select);
			m->item_select = m->item_select + 1;
			if((m->item_select) -(m->item_top_on_page)  >= max_items)
			{
				m->item_top_on_page = m->item_top_on_page + max_items;
				m->item_end_on_page = m->item_top_on_page + max_items;
				m->page_current = m->page_current + 1;
				if(m->item_end_on_page > m->item_total)
					m->item_end_on_page = m->item_total;
			}
			if((m->item_select) >= (m->item_total))
			{
				m->item_select = 0;
				m->item_top_on_page = 0;
				m->page_current = 1;
				m->item_end_on_page= m->item_top_on_page + max_items;
				if(m->item_end_on_page > m->item_total)
					m->item_end_on_page = m->item_total;
			}
			break;
		case NEXT_PAGE:
			LOGD("mmitest KEY_VIR_NEXT_PAGE");
			m->item_top_on_page = m->item_top_on_page + max_items;
			m->item_end_on_page = m->item_end_on_page + max_items;
			m->page_current = m->page_current + 1;
			if(m->item_top_on_page >= m->item_total)
			{
				m->item_top_on_page = 0;
				m->page_current = 1;
				m->item_end_on_page= m->item_top_on_page + max_items;
				if(m->item_end_on_page > m->item_total)
					m->item_end_on_page = m->item_total;
			}
			else if(m->item_end_on_page >= m->item_total)
			{
				m->item_end_on_page = m->item_total;
			}
			if(m->page_total > 1)
				m->item_select = m->item_top_on_page;
			break;
		case SELECT_ITEM:
			LOGD("mmitest SELECT_ITEM");
			return m->item_select;
		case GO_BACK:
			LOGD("mmitest GO_BACK");
			return -2;
		case GO_HOME:		// not support yet
			LOGD("mmitest GO_HOME");
			return -2;
		case NO_ACTION:
			LOGD("mmitest NO_ACTION");
			return -1;
		}
	}
	else if((action >= KEY_VIR_ITEMS ) && (action < KEY_VIR_ITEMS + (m->item_end_on_page - m->item_top_on_page)))
	{
		LOGD("mmitest TOUCH SELECT_ITEM");
		m->item_select = m->item_top_on_page + action - 1;
		return m->item_select;
	}
	return -1;
}

void ui_show_title(const char* title)
{
    ui_set_color(CL_TITLE_BG);
    gr_fill(0, 0, gr_fb_width(), TITLE_HEIGHT);
    ui_set_color(CL_TITLE_FG);
    set_render_mode(Render_BOLD);
    gr_text(0, (TITLE_HEIGHT + CHAR_SIZE) / 2, title);
    set_render_mode(Render_DEFAULT);
}

void ui_show_page(unsigned int n, unsigned int total)
{
    int room_size;
    char tips[10] = {0};

    if(n >= 10 || total >= 10)
        return;

    snprintf(tips, sizeof(tips), " / %s ", TEXT_PAGE);

    tips[0] = n + '0';
    tips[2] = total + '0';

    ui_set_color(CL_TITLE_FG);
    freetype_setsize(CHAR_SIZE/2);
    room_size = get_string_room_size(tips);
    gr_text(gr_fb_width()-room_size, TITLE_HEIGHT-CHAR_SIZE/4, tips);
    freetype_setsize(CHAR_SIZE);
}

void ui_show_button(const char* left,const char* center,const char* right)
{
    int width = gr_fb_width();
    int height = gr_fb_height();
    int room_size;
#ifndef SPRD_VIRTUAL_TOUCH
    left = NULL;
    center = NULL;
    right = NULL;
#endif

    if(left)
    {
        ui_set_color(CL_GREEN);
        gr_fill(0, height - button_height, button_width, height);
        ui_set_color(CL_WHITE);
        room_size = get_string_room_size(left);
        gr_text((button_width - room_size) / 2, height - (button_height - CHAR_SIZE) / 2, left);
    }
    else
    {
        if (usbin_state == 1)
        {
            ui_set_color(CL_SCREEN_BG);
            gr_fill(0, height - button_height, button_width, height);
        }
    }

    if(right)
    {
        ui_set_color(CL_RED);
        gr_fill(width - button_width, height - button_height, width, height);
        ui_set_color(CL_WHITE);
        room_size = get_string_room_size(right);
        gr_text(width - button_width + (button_width - room_size) / 2, height - (button_height - CHAR_SIZE) / 2, right);
    }

    if(center)
    {
        ui_set_color(CL_BLUE);
        gr_fill((width - button_width) / 2, height - button_height, (width + button_width) / 2, height);
        ui_set_color(CL_WHITE);
        room_size = get_string_room_size(center);
        gr_text((width - button_width) / 2 + (button_width - room_size) / 2, height - (button_height - CHAR_SIZE) / 2, center);
    }
}

int ui_show_text(int row, int col, const char* text)
{
    char text_temp[256] = {0};
    char *p_temp = text_temp;
    int cur_row = row;

    if(text != NULL)
    {
        memcpy(text_temp, text, strlen(text));
        while(p_temp != NULL)
        {
            p_temp = gr_text(col * CHAR_SIZE, TITLE_HEIGHT + (cur_row - 2) * LINE_HEIGHT + (CHAR_SIZE + LINE_HEIGHT) / 2, p_temp);
            cur_row++;
        }
    }
    return cur_row;
}


void ui_fill_locked(void)
{
	pthread_mutex_lock(&gUpdateMutex);
	draw_background_locked(gCurrentIcon);
	pthread_mutex_unlock(&gUpdateMutex);
}

int ui_handle_button(const char* left,const char* center,const char* right)
{
    int key = -1;
    int ret;

    if(left != NULL || right != NULL || center != NULL)
    {
        if (usbin_state == 1)
            ui_show_button(NULL, center, right);
        else
            ui_show_button(left, center, right);
        gr_flip();
    }

    ui_clear_key_queue();
    LOGD("mmitest waite key");
    for(;;)
    {
        key = ui_wait_key();
        if(ETIMEDOUT == key)
            continue;
        LOGD("mmitest key=%d", key);
        if(1 == usbin_state)
        {
            if((KEY_VIR_PASS == key) || (KEY_VOLUMEDOWN == key))
                key = -1;
        }
        if((((NULL == left) && (KEY_VIR_PASS == key)) || ((NULL == center) && ((KEY_VOLUMEUP == key) || (KEY_VIR_NEXT_PAGE == key))) || ((NULL == right) && (KEY_VIR_FAIL == key))))
            key = -1;

        switch(key)
        {
            case KEY_VOLUMEDOWN:
                ret = RL_PASS;
                LOGD("mmitest keyVD solved");
                break;
            case KEY_VOLUMEUP:
                ret = RL_BACK;
                LOGD("mmitest keyVU solved");
                break;
            case KEY_POWER:
                ret = RL_FAIL;
                LOGD("mmitest keyP solved");
                break;
            case KEY_VIR_PASS:
                ret = RL_PASS;
                LOGD("mmitest vir pass solved");
                break;
            case KEY_VIR_FAIL:
                ret = RL_FAIL;
                LOGD("mmitest key vir fail solved");
                break;
            case KEY_VIR_NEXT_PAGE:
                ret = RL_NEXT_PAGE;
                LOGD("mmitest key vir next page solved");
                break;
            case KEY_BACK:
                ret = RL_NA;
                LOGD("mmitest keybk solved");
                break;
            default:
                continue;
            }
            usbin_state = 0;
            return ret;
    }
}

int ui_wait_button(const char* left,const char* center,const char* right)
{
	int key = -1;
	int ret;

	if(left != NULL || right != NULL || center != NULL)
	{
		if (usbin_state == 1)
			ui_show_button(NULL, center, right);
		else
			ui_show_button(left, center, right);

		gr_flip();
	}

	ui_clear_key_queue();
	LOGD("mmitest waite key");
	for(;;)
	{
		key = ui_wait_key();
		if(ETIMEDOUT == key)
			continue;
		LOGD("mmitest key=%d",key);
		if(1==usbin_state)
		{
			if((KEY_VIR_PASS==key) || (KEY_VOLUMEDOWN ==key))
				continue;
		}
		if((((NULL==left) && (KEY_VIR_PASS==key)) || ((NULL==center) && ((KEY_VOLUMEUP==key)||(KEY_VIR_NEXT_PAGE==key))) || ((NULL==right) && (KEY_VIR_FAIL==key))))
			continue;

		switch(key)
		{
			case KEY_VOLUMEDOWN:
				LOGD("mmitest KEY_VOLUMEDOWN solved");
				break;
			case KEY_VOLUMEUP:
				LOGD("mmitest KEY_VOLUMEUP solved");
				break;
			case KEY_POWER:
				LOGD("mmitest KEY_POWER solved");
				break;
			case KEY_VIR_PASS:
				LOGD("mmitest VIR_PASS solved");
				break;
			case KEY_VIR_FAIL:
				LOGD("mmitest VIR_FAIL solved");
				break;
			case KEY_VIR_NEXT_PAGE:
				LOGD("mmitest VIR_NEXT_PAGE solved");
				break;
			case KEY_BACK:
				LOGD("mmitest KEY_BACK solved");
				break;
			default:
				continue;
		}
		usbin_state = 0;
		return key;
	}
}

void ui_fill_screen(unsigned char r,unsigned char g,unsigned char b)
{
	gr_color(r, g, b, 255);
	gr_fill(0, 0, gr_fb_width(), gr_fb_height());
}

void ui_clear_rows(int start, int num)
{
    unsigned int left, right, top, bottom;
    left = 0;
    right = gr_fb_width();
    top = TITLE_HEIGHT + (start - 2) *LINE_HEIGHT;
    bottom = top + num * LINE_HEIGHT;

    ui_set_color(CL_SCREEN_BG);
    gr_fill(left, top, right, bottom);
}

void ui_clear_rows_cols(int row_start, int n1,int col_start,int n2)
{
    unsigned int left, right, top, bottom;
    left = CHAR_SIZE * col_start;
    right = CHAR_SIZE * (col_start + n2);
    top = ITEM_HEIGHT + (row_start - 2)*CHAR_SIZE;
    bottom = ITEM_HEIGHT + (row_start - 2 + n1)*CHAR_SIZE;

    ui_set_color(CL_SCREEN_BG);
    gr_fill(left, top, right, bottom);
}

void ui_set_color(int cl)
{
	switch(cl)
	{
		case CL_WHITE:
			gr_color(255, 255, 255, 255);
			break;
		case CL_BLACK:
			gr_color(0, 0, 0, 255);
			break;
		case CL_RED:
			gr_color(255, 0, 0, 255);
			break;
		case CL_BLUE:
			gr_color(0, 0, 255, 255);
			break;
		case CL_GREEN:
			gr_color(0, 255, 0, 255);
			break;
		case CL_YELLOW:
			gr_color(255, 255, 0, 255);
			break;
		case CL_TITLE_BG:
			gr_color(62, 30, 51, 255);
			break;
		case CL_TITLE_FG:
			gr_color(235, 214, 228, 255);
			break;
		case CL_SCREEN_BG:
			gr_color(33, 16, 28, 255);
			break;
		case CL_SCREEN_FG:
			gr_color(255, 255, 255, 255);
			break;
		case CL_MENU_HL_BG:
			gr_color(201, 143, 182, 255);
			break;
		case CL_MENU_HL_FG:
			gr_color(17, 9, 14, 255);
			break;
		default:
			gr_color(0, 0, 0, 255);
			break;
	}

}

void ui_draw_line(int x1,int y1,int x2,int y2)
{
	int *xx1=&x1;
	int *yy1=&y1;
	int *xx2=&x2;
	int *yy2=&y2;

	int i=0;
	int j=0;
	int dx=*xx2-*xx1;
	int dy=*yy2-*yy1;

	if(*xx2>=*xx1)
	{
		for(i=*xx1;i<=*xx2;i++)
		{
			j=(i-*xx1)*dy/dx+*yy1;
			gr_draw_point(i,j);
		}
	}
	else
	{
		for(i=*xx2;i<*xx1;i++)
		{
			j=(i-*xx2)*dy/dx+*yy2;
			gr_draw_point(i,j);
		}
	}
}

void ui_draw_line_mid(int x1,int y1,int x2,int y2)
{
	int *xx1=&x1;
	int *yy1=&y1;
	int *xx2=&x2;
	int *yy2=&y2;

	int i=0;
	int j=0;
	int dx=*xx2-*xx1;
	int dy=*yy2-*yy1;

	if(abs(dx) >= abs(dy))
	{
		if(*xx2>=*xx1)
		{
			for(i=*xx1; i<=*xx2; i++)
			{
				j = (i-*xx1)*dy/dx+*yy1;
				if (j == *yy1 && i == *xx1)
					continue;
				gr_draw_point(i,j);
			}
		}
		else
		{
			for(i=*xx2;i<*xx1;i++)
			{
				j=(i-*xx2)*dy/dx+*yy2;
				if (j == *yy2 && i == *xx2)
					continue;
				gr_draw_point(i,j);
			}
		}
	}
	else
	{
		if(*yy2>=*yy1)
		{
			for(j=*yy1;j<=*yy2;j++)
			{
				i=(j-*yy1)*dx/dy+*xx1;
				if (j == *yy1 && i == *xx1)
					continue;
				gr_draw_point(i,j);
			}
		}
		else
		{
			for(j=*yy2;j<*yy1;j++)
			{
				i=(j-*yy2)*dx/dy+*xx2;
				if (j == *yy2 && i == *xx2)
					continue;
				gr_draw_point(i,j);
			}
		}
	}
}

static int _utf8_strlen(const char* str)
{
	int i = 0;
	int count = 0;
	int len = strlen (str);
	unsigned char chr = 0;
	while (i < len)
	{
		chr = str[i];
		count++;
		i++;
		if(i >= len)
			break;

		if(chr & 0x80)
		{
			chr <<= 1;
			while (chr & 0x80)
			{
				i++;
				chr <<= 1;
			}
		}
	}
	return count;
}

void parse_title(char * buf, char gap, char* value)
{
	char *str= NULL;

	if(buf != NULL && strlen(buf))
	{
		str = strchr(buf, gap);
		if(str != NULL)
		{
			str++;
			strncpy(value, str, strlen(str));
		}
	}
	return ;
}
