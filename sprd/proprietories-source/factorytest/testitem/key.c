#include "testitem.h"

struct test_key key_info[128];
int key_cnt = 0;
extern int max_rows;

int test_key_start(void)
{
    int ret;
    int menu_count = 0;
    int key = -1;
    int i = 0, j = 0;
    int cur_row = 2;
    int count = 0;

    struct test_key temp_key;
    struct test_key key_test[128];

    LOGD("mmitest start");
    ui_fill_locked();
    ui_show_title(MENU_TEST_KEY);
    ui_set_color(CL_GREEN);
    cur_row = ui_show_text(cur_row, 0, TEXT_KEY_ILLUSTRATE);
    for(i = 0; i < key_cnt; i++)
    {
        key_info[i].done = 0;
    }
    memcpy(&key_test[0], &key_info[0], sizeof(struct test_key)*key_cnt);

    for(;;)
    {
        cur_row = 3;
        ui_set_color(CL_SCREEN_BG);
        gr_fill(0, TITLE_HEIGHT + (cur_row-2) * LINE_HEIGHT, gr_fb_width(), gr_fb_height());

        for(i = 0; i < key_cnt; i++)
        {
            if((key_test[i].key == key) && (key_test[i].done == 0))
            {
                if(key_test[0].done && key_test[1].done && key_test[2].done)
                {
                    key_test[i].done = 1;
                    memcpy(&key_test[2], &key_test[1], sizeof(struct test_key));
                    memcpy(&key_test[1], &key_test[0], sizeof(struct test_key));
                    memcpy(&key_test[0], &key_test[i], sizeof(struct test_key));
                    memcpy(&key_test[i], &key_test[i + 1], sizeof(struct test_key) * (key_cnt - i));
                }
                else
                {
                    key_test[i].done = 1;
                    memcpy(&temp_key, &key_test[i], sizeof(struct test_key));
                    for(j = i; j >= 1; j--)
                    {
                        memcpy(&key_test[j], &key_test[j - 1], sizeof(struct test_key));
                    }
                    memcpy( &key_test[0], &temp_key, sizeof(struct test_key));
                }
                count++;
            }
        }
        for(i = 0; i < max_rows - 3 && i < key_cnt - count + 3; i++)
        {
            if(key_test[i].done)
            {
                ui_set_color(CL_GREEN);
            }
            else
            {
                ui_set_color(CL_RED);
            }
            cur_row = ui_show_text(cur_row, 0, key_test[i].key_shown);
        }

        gr_flip();
        if((count >= key_cnt))
            break;

        if(key == ETIMEDOUT)
            break;

        key = ui_wait_key();

        LOGD("mmitest key = %d, count=%d", key, count);
    }

    LOGD("mmitest key over");
    if(key == ETIMEDOUT)
    {
        ui_set_color(CL_SCREEN_BG);
        gr_fill(0, 0, gr_fb_width(), gr_fb_height());
        ui_set_color(CL_RED);
        ui_show_text(cur_row + 2, 0, TEXT_TEST_FAIL);
        gr_flip();
        sleep(1);
        ret = RL_FAIL;
    }
    else
    {
        ui_set_color(CL_GREEN);
        ui_show_text(cur_row + 2, 0, TEXT_TEST_PASS);
        gr_flip();
        sleep(1);
        ret = RL_PASS;
    }

    save_result(CASE_TEST_KEY, ret);
    return ret;
}
