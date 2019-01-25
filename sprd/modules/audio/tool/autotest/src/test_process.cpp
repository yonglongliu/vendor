/*
    Copyright (C) 2012 The Android Open Source Project

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/
#define LOG_TAG "hw_test_process"
#include <cutils/log.h>

#include "autotest.h"
#include "tinyxml.h"
#include "test_xml_utils.h"

#define BACKSLASH "\\"
#define SPLIT "\n"
#ifdef __cplusplus
extern "C" {
#endif

static int load_xml_handle(struct xml_handle *xmlhandle, const char *xmlpath,
                    const char *first_name)
{
    xmlhandle->param_doc = XML_open_param(xmlpath);
    if (xmlhandle->param_doc == NULL) {
        LOG_E("load xml handle failed (%s)\n", xmlpath);
        return -1;
    }
    xmlhandle->param_root =  XML_get_root_param(xmlhandle->param_doc);
    if (first_name != NULL) {
        xmlhandle->first_name = strdup(first_name);
    }
    return 0;
}

static void release_xml_handle(struct xml_handle *xmlhandle)
{
    if (xmlhandle->param_doc) {
        XML_release_param(xmlhandle->param_doc);
        xmlhandle->param_doc = NULL;
        xmlhandle->param_root = NULL;
    }
    if (xmlhandle->first_name) {
        free(xmlhandle->first_name);
        xmlhandle->first_name = NULL;
    }
}


int init_sprd_xml(struct hardware_test_handle * test){
    struct xml_handle *xmlhandle=NULL;
    int ret=-1;

    LOG_I("init_sprd_xml enter\n");
    xmlhandle=&test->xml;
    xmlhandle->first_name=NULL;
    xmlhandle->param_root=NULL;
    xmlhandle->param_doc=NULL;

    if(test->xml_path!=NULL){
        ret = load_xml_handle(xmlhandle, test->xml_path, AUDIO_HARDWARE_FIRST_NAME);
    }else{
        ret = load_xml_handle(xmlhandle, AUDIO_HARDWARE_TEST_XML, AUDIO_HARDWARE_FIRST_NAME);
    }

    if (ret != 0) {
        LOG_E("load xml handle failed (%s)\n", AUDIO_HARDWARE_TEST_XML);
        return -1;
    }

    if(NULL==xmlhandle->param_root){
        LOG_E("init_sprd_xml param_root is null\n");
        return -2;
    }
    LOG_I("init_sprd_xml success\n");

    return 0;
}


static int parse_reg_ctrl(TiXmlElement *group){
    const char *tmp=NULL;
    int val=0;
    int bits=0;
    int offset=0;
    int addr=0;
    int reg=0;
    int new_values=0;
    int i=0;
    int mask=0;

    tmp=group->Attribute("val");
    if(NULL!=tmp){
        val=strtoul(tmp,NULL,16);
    }

    tmp=group->Attribute("bits");
    if(NULL!=tmp){
        bits=strtoul(tmp,NULL,10);
    }

    tmp=group->Attribute("offset");
    if(NULL!=tmp){
        offset=strtoul(tmp,NULL,10);
    }

    tmp=group->Attribute("addr");
    if(NULL!=tmp){
        addr=strtoul(tmp,NULL,16);
    }
    _get_register_value(addr,1,&reg);

    for(i=0;i<bits;i++){
        mask |= 1<<i;
        LOG_D("mask[%d]:0x%x",i,mask);
    }

    mask <<=offset;

    val  &=( (1<< bits)-1);
    val <<=offset;

    new_values= reg &(~mask);
    new_values |=val;

    LOG_I("Set reg addr:0x%x val:0x%x mask:0x%x offset:0x%x bits:0x%x pre_valuse:0x%x values:0x%x\n",addr,val,~mask,offset,bits,reg,new_values);
    LOG_I("Set reg addr:0x%x val:0x%x mask:0x%x offset:0x%x bits:0x%x pre_valuse:0x%x values:0x%x\n",addr,val,~mask,offset,bits,reg,new_values);
    _set_register_value(addr,new_values);

    /*
    usleep(20*1000);
    read_reg(addr,1,&reg);
    if(reg!=new_values){
        LOG_I("write addr:0x%x err, get val:0x%x\n",addr,reg);
    }
    */

    reg=-1;
    _get_register_value(addr,1,&reg);

    if(reg!=new_values){
        ALOGW("reg addr:0x%x reg val:0x%x != 0x%x",addr,reg,new_values);
        if((reg&(~mask))!=(new_values&(~mask))){
            return -1;
        }
    }
    return 0;
}


static void run_shell_cmd(const char *cmd_str)
{
    FILE *pipe_file=NULL;
    char result[4096]={0};

    if((pipe_file=popen(cmd_str, "r"))!=NULL){
        fread(result, 1, sizeof(result), pipe_file);
        pclose(pipe_file);
        pipe_file = NULL;
    }
    LOG_I("run_shell_cmd cmd:%s\nresult:%s\n",cmd_str,result);

    return ;
}

static int parse_shell_cmd(TiXmlElement *group){
    const char *tmp=NULL;

    tmp=group->Attribute("cmd");
    run_shell_cmd(tmp);

    return 0;
}

static int hardware_test_case_process(struct hardware_test_handle * test,TiXmlElement *group){
    TiXmlElement  *tmp=NULL;
    const char *tmp_str=NULL;

    tmp = group->FirstChildElement();
    LOG_I("hardware_test_case_process:%s",group->Value());
    while(NULL!=tmp){
        tmp_str = tmp->Value();
        if(strncmp(tmp_str,"reg",strlen("reg"))==0){
            if(parse_reg_ctrl(tmp)<0){
                LOG_E("hardware_test_case_process parse_reg_ctrl fail");
                return -1;
            }
        } else if(strncmp(tmp_str,"ctl",strlen("ctl"))==0){
            parse_mixer_control(&test->mixer_ctl,tmp);
        } else if(strncmp(tmp_str,"cmd",strlen("cmd"))==0){
            parse_shell_cmd(tmp);
        }
        tmp = tmp->NextSiblingElement();
    }
    return 0;
}

int _hardware_test_process(struct hardware_test_handle * test, char * cmd, int len){
    char data_buf[MAX_SOCKT_LEN];
    char *tmpstr;
    const char *tmp=NULL;
    TiXmlElement  *ele=NULL;

    int ret=-1;
    int depth=0;
    char *eq = strchr(cmd, '=');
    char *svalue = NULL;
    char *key = NULL;
    TiXmlElement *tmpgroup = NULL;

    if (eq) {
        key = strndup(cmd, eq - cmd);
        if (*(++eq)) {
            svalue = strdup(eq);
        }
    }

    LOG_I("key:%s val:%s",key,svalue);
    test->data_state = DATA_START;
    char *line = strtok_r(key, BACKSLASH, &tmpstr);

   line = strtok_r(NULL, BACKSLASH, &tmpstr);
   ele= (TiXmlElement  *)test->xml.param_root;
   while (line != NULL) {
        depth++;
        LOG_I("line:%s ele:%s depth:%d\n",line,ele->Value(),depth);
         ele = ele->FirstChildElement(line);

        if(NULL==ele){
            break;
        }
        line = strtok_r(NULL, BACKSLASH, &tmpstr);
    }

    while(ele!=NULL){
        tmp=ele->Attribute(VALUE);
        LOG_I("hardware_test_process ele:%s VALUE:%s %s depth\n",ele->Value(),tmp,svalue,depth);
        if(strncmp(tmp,svalue,strlen(tmp)) == 0){
            if(hardware_test_case_process(test,ele)<0){
                LOG_E("_hardware_test_process hardware_test_case_process fail");
                return -1;
            }
            ret=0;
        }
        ele = ele->NextSiblingElement();
     }
    return ret;
}

int hardware_test_process(struct hardware_test_handle * test, char * cmd, int len){
    char data_buf[MAX_SOCKT_LEN];
    char *tmpstr;
    char *line = strtok_r(cmd, SPLIT, &tmpstr);

   while (line != NULL) {
       if(_hardware_test_process(test,line,strlen(line))<0){
            LOG_E("hardware_test_process _hardware_test_process fail");
            return -1;
       }
        line = strtok_r(NULL, SPLIT, &tmpstr);
    }
    return 0;
}
#ifdef __cplusplus
}
#endif

