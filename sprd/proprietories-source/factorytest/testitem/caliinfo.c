#include "testitem.h"

extern int max_rows;
extern int max_items;
extern char s_cali_info[1024];
extern char s_cali_info1[1024];
extern char s_cali_info2[1024];
extern char* test_modem_get_caliinfo(void);
extern bool isLteOnly(void);

/********************************************************************
*  Function:str_replace()
*********************************************************************/
void str_replace(char *p_result,char* p_source,char* p_seach,char *p_repstr)
{
    int repstr_len = strlen(p_repstr);
    int search_len = strlen(p_seach);
    char *pos;
    int nLen = 0;
    do{
        pos = strstr(p_source,p_seach);
        if (pos == 0)
        {
            strcpy(p_result, p_source);
            break;
        }

        nLen = pos - p_source;
        memcpy(p_result, p_source, nLen);
        memcpy(p_result + nLen, p_repstr, repstr_len);
        p_source = pos + search_len;
        p_result = p_result + nLen + repstr_len;
    } while(pos);
}

int test_cali_info(void)
{
    int row = 2;
    char tmp[64] = {0}, gsm_cali[64][64] = {0}, wcdma_cali[64][64] = {0}, lte_cali[64][64] = {0};
    struct display_content cali_content[128];
    char property[PROPERTY_VALUE_MAX];
    char* pcali, *pos1, *pos2;
    int gsm_num = 0, wcdma_num = 0, lte_num = 0;
    int len = 0, total_page = 0, cur_page = 1;
    int n = 0;
    int i;

    ui_fill_locked();
    ui_show_title(MENU_CALI_INFO);

    pcali = s_cali_info;
    len = strlen(pcali);

    /*delete the "BIT",and replace the calibrated with cali */
    //GSM cali ino
    if (!isLteOnly()){
        while(len > 0)
        {
            pos1 = strstr(pcali, ":");
            if(pos1 == NULL)
                break;

            pos1++;
            pos2 = strstr(pos1, "BIT");
            if(pos2 == NULL)
            {
                strcpy(tmp, pos1);
                len = 0;
            }
            else
            {
                memcpy(tmp, pos1, (pos2 - pos1));
                tmp[pos2 - pos1] = '\0';
                len -= (pos2 - pcali);
                pcali = pos2;
            }

            str_replace(gsm_cali[gsm_num], tmp, "calibrated", "cali");
            memset(tmp, 0, sizeof(tmp));

            if(strstr(gsm_cali[gsm_num], "Not"))
                cali_content[n].color = CL_RED;
            else
                cali_content[n].color = CL_GREEN;

            cali_content[n++].content = gsm_cali[gsm_num++];
        }
        cali_content[n++].content = NULL;       //for separate different cali item
    }

    //LTE cali ino
    property_get(PROP_MODEM_LTE_ENABLE, property, "not_find");
    if(!strcmp(property, "1"))
    {
        pcali = s_cali_info2;
        len = strlen(pcali);
        while(len > 0)
        {
            pos1 = strstr(pcali, ":");
            if(pos1 == NULL)
                break;

            pos1++;
            pos2 = strstr(pos1, "BIT");
            if(pos2 == NULL)
            {
                strcpy(tmp, pos1);
                len = 0;
            }
            else
            {
                memcpy(tmp, pos1, (pos2 - pos1));
                tmp[pos2 - pos1] = '\0';
                len -= (pos2 - pcali);
                pcali = pos2;
            }
            str_replace(lte_cali[lte_num], tmp, "BAND", "WCDMA BAND");
            memset(tmp, 0, sizeof(tmp));

            if(strstr(lte_cali[lte_num], "Not"))
                cali_content[n].color = CL_RED;
            else
                cali_content[n].color = CL_GREEN;

            cali_content[n++].content = lte_cali[lte_num++];
        }
    }
    cali_content[n++].content = NULL;       //for separate different cali item

    if (!isLteOnly()){
        //WCDMA cali ino
        pcali = s_cali_info1;
        len = strlen(pcali);
        while(len > 0)
        {
            pos1 = strstr(pcali, ":");
            if(pos1 == NULL)
                break;

            pos1++;
            pos2 = strstr(pos1, "BIT");
            if(pos2 == NULL)
            {
                strcpy(tmp, pos1);
                len = 0;
            }
            else
            {
                memcpy(tmp, pos1, (pos2 - pos1));
                tmp[pos2 - pos1] = '\0';
                len -= (pos2 - pcali);
                pcali = pos2;
            }
            str_replace(wcdma_cali[wcdma_num], tmp, "BAND", "WCDMA BAND");
            memset(tmp, 0, sizeof(tmp));

            if(strstr(wcdma_cali[wcdma_num], "Not"))
                cali_content[n].color = CL_RED;
            else
                cali_content[n].color = CL_GREEN;

            cali_content[n++].content = wcdma_cali[wcdma_num++];
        }
        cali_content[n++].content = NULL;       //for separate different cali item
    }

    total_page += ((((gsm_num % max_rows) > 0) ? 1 : 0) + (gsm_num / max_rows ));
    total_page += ((((wcdma_num % max_rows) > 0) ? 1 : 0) + (wcdma_num / max_rows));
    total_page += ((((lte_num % max_rows) > 0) ? 1 : 0) + (lte_num / max_rows));
    LOGD("gsm_num = %d,%d,%d", gsm_num, wcdma_num, lte_num);

    for(i = 0; i < n; i++)
    {
        ui_set_color(cali_content[i].color);
        row = ui_show_text(row, 0, cali_content[i].content);

        if((row - 2) >= max_rows || i == (n - 1) || (row != 2 && cali_content[i].content == NULL))
        {
            ui_show_page(cur_page, total_page);
            switch(ui_wait_button(NULL, TEXT_NEXT_PAGE, TEXT_GOBACK))
            {
                case KEY_VOLUMEDOWN:
                case KEY_VIR_NEXT_PAGE:
                    ui_set_color(CL_SCREEN_BG);
                    gr_fill(0, 0, gr_fb_width(), gr_fb_height());
                    ui_set_color(CL_TITLE_BG);
                    ui_show_title(MENU_CALI_INFO);
                    cur_page++;
                    row = 2;
                    if(i == (n - 1))		//if is the last page, back to the first page
                    {
                        i = -1;
                        cur_page = 1;
                        LOGD("turn to the first page");
                    }
                    else
                        LOGD("turn to the next page");
                    break;
                case KEY_POWER:
                case KEY_VIR_BACK:
                case KEY_BACK:
                    LOGD("exist");
                    return 0;
                default:
                    i--;
                    row--;
                    LOGE("the key is unexpected!");
            }
        }
    }
    return 1;
}
