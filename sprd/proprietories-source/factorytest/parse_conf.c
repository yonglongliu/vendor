#include "testitem.h"

#define MAX_LINE_LEN			256
#define PCBA_SUPPORT_CONFIG   "/system/etc/PCBA.conf"
extern int key_cnt;
extern struct test_key key_info[128];
extern ST_MCCMNC_NUMBER MCCMNC_DIAL_NUMBER[MCCMNC_TOTAL_NUMBER];

static int parse_string(char * buf, char gap, int* value)
{
    int len = 0;
    char *ch = NULL;
    char str[10] = {0};

    if(buf != NULL && value  != NULL){
        ch = strchr(buf, gap);
        if(ch != NULL){
            len = ch - buf ;
            strncpy(str, buf, len);
            *value = atoi(str);
        }
    }

    return len;
}

char *parse_string_ptr(char *src, char c)
{
	char *results;
	results = strchr(src, c);
	if(results == NULL) {
		LOGD("%s is null!", results);
		return NULL;
	}
	results++ ;
	while(results[0]== c)
		results++;
	return results;
}

char * parse_string_get(char *buf, char c, char *str)
{
    int len = 0;
    char *ch = NULL;

    if(buf != NULL){
        ch = strchr(buf, c);
        if(ch != NULL){
            len = ch - buf ;
            strncpy(str, buf, len);
        }
    }
    return str;
}

int parse_case_entries(char *type, int* arg1, int* arg2)
{
	int len;
	char *str = type;

	/* sanity check */
	if(str == NULL) {
		LOGD("type is null!");
		return -1;
	}
	if((len = parse_string(str, '\t', arg1)) <= 0)	return -1;

	str += len + 1;
	if(str == NULL) {
		LOGD("mmitest type is null!");
		return -1;
	}
	if((len = parse_string(str, '\t', arg2)) <= 0)	return -1;

	return 0;
}

int get_sensor_Num(char* s)
{
   if(!strcmp(s,"Ts")) return SENSOR_TS;
   if(!strcmp(s,"Acc")) return SENSOR_ACC;
   if(!strcmp(s,"Pxy")) return SENSOR_PXY;
   if(!strcmp(s,"Lux")) return SENSOR_LUX;
   if(!strcmp(s,"Hde")) return SENSOR_HDE;
   if(!strcmp(s,"Hdk"))	return SENSOR_HDK;
   if(!strcmp(s,"Gyroscope"))	return SENSOR_GYROSCOPE;
   if(!strcmp(s,"Magnetic"))	return SENSOR_MAGNETIC;
   if(!strcmp(s,"Orientation"))	return SENSOR_ORIENTATION;
   if(!strcmp(s,"Pressure"))	return SENSOR_PRESSURE;
   if(!strcmp(s,"Temperature"))	return SENSOR_TEMPERATURE;

   return -1;
}

int parse_sensor_entries(char *buf)
{
	char *pos1, *pos2;
	char type[BUF_LEN] = {0};
	char name[BUF_LEN] = {0};
	int id;

	/* fetch each field */
	if((pos1 = parse_string_ptr(buf, '\t')) == NULL)
		return -1;
	else parse_string_get(pos1, '\t',type);

	if((pos2 = parse_string_ptr(pos1, '\t')) == NULL)
		return -1;
	else  parse_string_get(pos2, '\n',name);

	/* init data structure according to type */
	LOGD("sensor type = %s; sensor name = %s", type, name);
	//if(NULL != type && NULL != name) {
	id = get_sensor_Num(strdup(type));
	if (id != -1) {
	  strncpy(SENSOR_DEV_NAME[id], strdup(name), strlen(strdup(name)));
	  LOGD("type:%d, :%s",id, SENSOR_DEV_NAME[id]);
	}

	return 0;
}

int s_mccmnc_id = 0;
int parse_mccmnc_entries(char *buf)
{
	char *pos1, *pos2;
	char type[BUF_LEN] = {0};
	char name[BUF_LEN] = {0};
	int id;

	/* fetch each field */
	if((pos1 = parse_string_ptr(buf, '\t')) == NULL)
		return -1;
	else parse_string_get(pos1, '\t',type);

	if((pos2 = parse_string_ptr(pos1, '\t')) == NULL)
		return -1;
	else  parse_string_get(pos2, '\n',name);

	LOGD("sensor mccmnc = %s; service number = %s", type, name);

	strncpy(MCCMNC_DIAL_NUMBER[s_mccmnc_id].mccmnc, type, strlen(type));
	strncpy(MCCMNC_DIAL_NUMBER[s_mccmnc_id].serviceNumber, name, strlen(name));

	s_mccmnc_id++;

	return 0;
}

int get_key_Num(char* s)
{
    if(!strcmp(s, "Power")) return KEY_TYPE_POWER;
    if(!strcmp(s, "VolumeDown")) return KEY_TYPE_VOLUMEDOWN;
    if(!strcmp(s, "VolumeUp")) return KEY_TYPE_VOLUMEUP;
    if(!strcmp(s, "Camera")) return KEY_TYPE_CAMERA;
    if(!strcmp(s, "Menu")) return KEY_TYPE_MENU;
    if(!strcmp(s, "Home")) return KEY_TYPE_HOME;
    if(!strcmp(s, "Back")) return KEY_TYPE_BACK;
    if(!strcmp(s, "Up")) return KYE_TYPE_UP;
    if(!strcmp(s, "Left")) return KYE_TYPE_LEFT;
    if(!strcmp(s, "Confirm")) return KYE_TYPE_CONFIRM;
    if(!strcmp(s, "Right")) return KYE_TYPE_RIGHT;
    if(!strcmp(s, "Call")) return KYE_TYPE_CALL;
    if(!strcmp(s, "Down")) return KYE_TYPE_DOWN;
    if(!strcmp(s, "One")) return KYE_TYPE_ONE;
    if(!strcmp(s, "Two")) return KYE_TYPE_TWO;
    if(!strcmp(s, "Three")) return KYE_TYPE_THREE;
    if(!strcmp(s, "Four")) return KYE_TYPE_FOUR;
    if(!strcmp(s, "Five")) return KYE_TYPE_FIVE;
    if(!strcmp(s, "Six")) return KYE_TYPE_SIX;
    if(!strcmp(s, "Seven")) return KYE_TYPE_SEVEN;
    if(!strcmp(s, "Eight")) return KYE_TYPE_EIGHT;
    if(!strcmp(s, "Nine")) return KYE_TYPE_NINE;
    if(!strcmp(s, "Star")) return KYE_TYPE_STAR;
    if(!strcmp(s, "Zero")) return KYE_TYPE_ZERO;
    if(!strcmp(s, "Pound")) return KYE_TYPE_POUND;

    return -1;
}

int parse_key_entries(char *buf)
{
    char *pos1, *pos2;
    char type[BUF_LEN] = {0};
    int value;
    int id;

    /* fetch each field */
    if((pos1 = parse_string_ptr(buf, '\t')) == NULL)
        return -1;
    else parse_string_get(pos1, '\t', type);

    if((pos2 = parse_string_ptr(pos1, '\t')) == NULL)
        return -1;
    else  parse_string(pos2, '\n', &value);

    /* init data structure according to type */
    id = get_key_Num(strdup(type));
    switch (id)
    {
        case KEY_TYPE_POWER:
            strncpy(key_info[key_cnt].key_shown, TEXT_KEY_POWER, strlen(TEXT_KEY_POWER));
            break;
        case KEY_TYPE_VOLUMEDOWN:
            strncpy(key_info[key_cnt].key_shown, TEXT_KEY_VOLUMEDOWN, strlen(TEXT_KEY_VOLUMEDOWN));
            break;
        case KEY_TYPE_VOLUMEUP:
            strncpy(key_info[key_cnt].key_shown, TEXT_KEY_VOLUMEUP, strlen(TEXT_KEY_VOLUMEUP));
            break;
        case KEY_TYPE_CAMERA:
            strncpy(key_info[key_cnt].key_shown, TEXT_KEY_CAMERA, strlen(TEXT_KEY_CAMERA));
            break;
        case KEY_TYPE_MENU:
            strncpy(key_info[key_cnt].key_shown, TEXT_KEY_MENU, strlen(TEXT_KEY_MENU));
            break;
        case KEY_TYPE_HOME:
            strncpy(key_info[key_cnt].key_shown, TEXT_KEY_HOMEPAGE, strlen(TEXT_KEY_HOMEPAGE));
            break;
        case KEY_TYPE_BACK:
            strncpy(key_info[key_cnt].key_shown, TEXT_KEY_BACK, strlen(TEXT_KEY_BACK));
            break;
        case KYE_TYPE_UP:
            strncpy(key_info[key_cnt].key_shown, TEXT_KEY_UP, strlen(TEXT_KEY_UP));
            break;
        case KYE_TYPE_LEFT:
            strncpy(key_info[key_cnt].key_shown, TEXT_KEY_LEFT, strlen(TEXT_KEY_LEFT));
            break;
        case KYE_TYPE_CONFIRM:
            strncpy(key_info[key_cnt].key_shown, TEXT_KEY_CONFIRM, strlen(TEXT_KEY_CONFIRM));
            break;
        case KYE_TYPE_RIGHT:
            strncpy(key_info[key_cnt].key_shown, TEXT_KEY_RIGHT, strlen(TEXT_KEY_RIGHT));
            break;
        case KYE_TYPE_CALL:
            strncpy(key_info[key_cnt].key_shown, TEXT_KEY_CALL, strlen(TEXT_KEY_CALL));
            break;
        case KYE_TYPE_DOWN:
            strncpy(key_info[key_cnt].key_shown, TEXT_KEY_DOWN, strlen(TEXT_KEY_DOWN));
            break;
        case KYE_TYPE_ONE:
            strncpy(key_info[key_cnt].key_shown, TEXT_KEY_ONE, strlen(TEXT_KEY_ONE));
            break;
        case KYE_TYPE_TWO:
            strncpy(key_info[key_cnt].key_shown, TEXT_KEY_TWO, strlen(TEXT_KEY_TWO));
            break;
        case KYE_TYPE_THREE:
            strncpy(key_info[key_cnt].key_shown, TEXT_KEY_THREE, strlen(TEXT_KEY_THREE));
            break;
        case KYE_TYPE_FOUR:
            strncpy(key_info[key_cnt].key_shown, TEXT_KEY_FOUR, strlen(TEXT_KEY_FOUR));
            break;
        case KYE_TYPE_FIVE:
            strncpy(key_info[key_cnt].key_shown, TEXT_KEY_FIVE, strlen(TEXT_KEY_FIVE));
            break;
        case KYE_TYPE_SIX:
            strncpy(key_info[key_cnt].key_shown, TEXT_KEY_SIX, strlen(TEXT_KEY_SIX));
            break;
        case KYE_TYPE_SEVEN:
            strncpy(key_info[key_cnt].key_shown, TEXT_KEY_SEVEN, strlen(TEXT_KEY_SEVEN));
            break;
        case KYE_TYPE_EIGHT:
            strncpy(key_info[key_cnt].key_shown, TEXT_KEY_EIGHT, strlen(TEXT_KEY_EIGHT));
            break;
        case KYE_TYPE_NINE:
            strncpy(key_info[key_cnt].key_shown, TEXT_KEY_NINE, strlen(TEXT_KEY_NINE));
            break;
        case KYE_TYPE_STAR:
            strncpy(key_info[key_cnt].key_shown, TEXT_KEY_STAR, strlen(TEXT_KEY_STAR));
            break;
        case KYE_TYPE_ZERO:
            strncpy(key_info[key_cnt].key_shown, TEXT_KEY_ZERO, strlen(TEXT_KEY_ZERO));
            break;
        case KYE_TYPE_POUND:
            strncpy(key_info[key_cnt].key_shown, TEXT_KEY_POUND, strlen(TEXT_KEY_POUND));
            break;
        default:
            LOGD("invalid key");
            break;
    }

    strncpy(key_info[key_cnt].key_name, strdup(type), strlen(strdup(type)));
    key_info[key_cnt].key = value;
    LOGD("key_info[%d].key:%s,key_info[%d].key:%d", key_cnt, key_info[key_cnt].key_name, key_cnt, key_info[key_cnt].key);
    key_cnt++;

    return 0;
}

int parse_config()
{
	FILE *fp;
	int ret = 0, count = 0, err = 0;
	int id,flag;
	char *type,*name;
	char buffer[MAX_LINE_LEN]={0};

	fp = fopen(PCBA_SUPPORT_CONFIG, "r");
	if(fp == NULL) {
		LOGE("mmitest open %s failed! %d IN", PCBA_SUPPORT_CONFIG, __LINE__);
		return -1;
	}

	/* parse line by line */
	ret = 0;
	while(fgets(buffer, MAX_LINE_LEN, fp) != NULL) {
		if('#'==buffer[0])
			continue;
		if((buffer[0]>='0') && (buffer[0]<='9')){
			ret = parse_case_entries(buffer,&id,&flag);
			if(ret != 0) {
				LOGD("mmitest parse %s,buffer=%s return %d.  reload",PCBA_SUPPORT_CONFIG, buffer,ret);
				fclose(fp);
				return -1;
			}
			support_result[count].id = id;
			support_result[count++].support= flag;
			err = 1;
		}else if(!strncmp("sensor", buffer, 6)){
			ret = parse_sensor_entries(buffer);
			if(ret != 0) {
				LOGD("mmitest parse %s,buffer=%s return %d.  reload",PCBA_SUPPORT_CONFIG, buffer,ret);
				fclose(fp);
				return -1;
			}
			err = 1;
		}else if(!strncmp("key", buffer, 3)){
			ret = parse_key_entries(buffer);
			if(ret != 0) {
				LOGD("mmitest parse %s,buffer=%s return %d.  reload",PCBA_SUPPORT_CONFIG, buffer,ret);
				fclose(fp);
				return -1;
			}
			err = 1;
		}else if (!strncmp("MCCMNC", buffer, 6)){
			ret = parse_mccmnc_entries(buffer);
			if(ret != 0) {
				LOGD("mmitest parse %s,buffer=%s return %d.  reload",PCBA_SUPPORT_CONFIG, buffer,ret);
				fclose(fp);
				return -1;
			}
			err = 1;
		}
		if(0 == err)
			LOGD("can't check line = %s", buffer);
		err = 0;
	}

	fclose(fp);
	if(count < TOTAL_NUM) {
		LOGD("mmitest parse slog.conf failed");
	}

	return ret;
}
