#include "ATProcesser.h"
#include <iostream>
#include <sstream>
#include <fstream>
//#define BUILD_ENG
#include <string>
#include<sys/socket.h>
#include <cutils/sockets.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <ubi-user.h>
#ifdef BUILD_ENG
//#include "eng_appclient_lib.h"
//#include "AtChannel.h"
#include "sprd_atci.h"
#endif
#include "HTTPRequest.h"
#include "./xmllib/xmlparse.h"
#ifdef ANDROID
#include <cutils/log.h>
#include <android/log.h>
#define MYLOG(args...)  __android_log_print(ANDROID_LOG_INFO,  "HAOYOUGEN", ## args)
#endif
#include <cutils/properties.h>


#define  SUCCESS   0
#define  FAIL      1

#define BUFSIZE    256

/* SPRD bug 842932:miscdata path issue*/
#define PHASE_CHECK_MISCDATA "miscdata"



static char *whitelist[] = {
  "cat /sys/class/power_supply/sprdfgu/fgu_current",
  "cat /data/misc/bluedroid/btmac.txt",
  "cat /data/misc/wifi/wifimac.txt",
  "cat /sys/class/power_supply/battery/temp",
  "cat /sys/class/power_supply/battery/capacity",
  "cat /sys/class/power_supply/battery/voltage_now",
  "cat /sys/class/power_supply/battery/present",
  "getprop",
  "setprop key_sendpower",
  "setprop key_sleepmode",
  "setprop persist.sys.engpc.disable",
  "setprop persist.ylog.enabled",
  "setprop persist.sys.sprd.wcnreset",
  "setprop persist.sys.sprd.modemreset",
  "setprop persist.sys.sprd.pcmlog",
  "cat data/local/slogmodem/slog_modem.conf",
  "ps | grep pnscr",
  "setprop persist.sys.bt.non.ssp",
  "wcnd_cli eng",
  "wcnd -G &",
  "wcnd -G",
  "echo AT+SFUN=5\r > dev/stty_lte31",
  "echo AT+SPTESTMODEM=21,254\r > dev/stty_lte31",
  "echo AT+SFUN=4\r > dev/stty_lte31",
  "/productinfo/factory_reset.txt",
  "saveNfcCplc",
  /* SPRD 840252: add reboot test */
  "reboot",
  "ps | grep b2g | wc -l",
  /* SPRD 833714: control wcn log by adb shell @{ */
  "wcnd_cli wcn at+armlog=1",
  "wcnd_cli wcn at+armlog=0",
  "wcnd_cli wcn at+armlog?",
  "wcnd_cli wcn dump_enable",
  "wcnd_cli wcn dumpmem",
  "wcnd_cli wcn startengpc",
  "wcnd_cli wcn stopengpc",
  "wcnd_cli wcn at+spatassert=1",
  "cplogctl state 5mode",
  "cplogctl enable 5mode",
  "cplogctl enable wcn",
  "cplogctl enable gnss",
  "cplogctl disable 5mode",
  "cplogctl disable gnss",
  "cplogctl disable wcn",
  "cplogctl clear",
  "ls /data/mlog",
  "cat /data/mlog/ae.txt",
  "cat /data/mlog/awb.txt",
  "cat /data/mlog/lsc1.txt",
  "cat /data/mlog/smart.txt",
  "rm /data/sgps_log/Nmealog.txt",
  "cat /data/sgps_log/Nmealog.txt",
  "ylog_cli ylog all start",
  "ylog_cli ylog all stop",
  "ylog_cli ylog tcpdump start",
  "ylog_cli ylog tcpdump stop",
  "ylog_cli ylog hcidump start",
  "ylog_cli ylog hcidump stop",
  "ylog_cli rylogr",
  "mkdir /data/mlog",
  "touch /data/mlog/smart.txt",
  "touch /data/mlog/ae.txt",
  "touch /data/mlog/awb.txt",
  "touch /data/mlog/lsc1.txt",
  "setprop persist.sys.isp.smartdebug 1",
  "setprop persist.sys.isp.ae.mlog /data/mlog/ae.txt",
  "setprop debug.isp.awb.mlog /data/mlog/awb.txt",
  "setprop debug.camera.isp.lsc 1",
  "chmod 777 /data/mlog/*",
  "cat /sys/class/display/panel0/name",
  "cat /sys/devices/virtual/misc/sprd_sensor/camera_sensor_name",
  "persist.sys.engpc",
  "cat /sys/class/power_supply/sprdfgu/fgu_vol",
  "cat /sys/class/power_supply/battery/charger_voltage",
  "cat /sys/class/power_supply/battery/real_time_current",
  "getprop | grep ro.boot.hardware.revision",
  "setprop persist.sys.sensor.id trigger_srid",
  "reboot -p",
  "climax -dsysfs -l /vendor/firmware/tfa9897.cnt --resetMtpEx",
  "climax -dsysfs -l /vendor/firmware/tfa9897.cnt --calibrate=once",
  "climax -dsysfs -l /vendor/firmware/tfa9897.cnt --calshow",
  "ls /sys/bus/i2c/devices",
  "cat /proc/cmdline",
  "cat /sys/bus/mmc/devices/mmc0\\:0001/name",
  "cat /sys/bus/mmc/devices/mmc0\\:0001/manfid",
  "cat /sys/bus/mmc/devices/mmc0\\:0001/oemid",
  "setprop mmitestgps.satellite.enabled",
  "setprop sgps.nmea.enabled",
  "echo 1 > /sys/class/power_supply/battery/stop_charge",
  "echo 0 > /sys/class/power_supply/battery/stop_charge",
  "du -sh /storage/emulated",
  "df /storage/emulated | sed -n 2p",
  "cat /proc/sprd_dmc/property",
  "cat /sys/class/gpio/gpio43",
  /* }@ */
};


ATProcesser::ATProcesser(string url):m_url(url)
{
    m_cmd = HTTPRequest::URL::getParameter(url, "cmd");
    /* SPRD bug 842932:miscdata path issue*/
    string temp = property_get("ro.product.partitionpath");
    phasecheckPath = temp + PHASE_CHECK_MISCDATA;
    MYLOG("ATProcesser : phasecheck Path:%s\n", phasecheckPath.c_str());
}

ATProcesser::~ATProcesser()
{

}

string ATProcesser::response()
{
    MYLOG("handleSocket ATProcesser response m_cmd = %s",m_cmd.c_str());
    /* SPRD:838896 - KAIOS SGPS control config.xml thought engmoded @{ */

    if (m_cmd == "writexml")
    {
        string key = HTTPRequest::URL::getParameter(m_url, "key");
        string value = HTTPRequest::URL::getParameter(m_url, "value");
        MYLOG("response writeXML key:%s, value:%s",key.c_str(), value.c_str());
        return this->writeXML(key, value);
    }
    else if (m_cmd == "readxml")
    {
        string key = HTTPRequest::URL::getParameter(m_url, "key");
        MYLOG("response readXML key: %s", key.c_str());
        return this->readXML(key);
    }
    /* }@ */
    else if (strstr(m_cmd.c_str(), "LOG") != NULL) {
        MYLOG("response get log status");
        m_tcplog_status = property_get("ylog.svc.tcpdump");
        m_hcilog_status = property_get("ylog.svc.hcidump");
        m_pcmlog_status = property_get("persist.sys.sprd.pcmlog");
        MYLOG("response tcpdumpLog = %s, hcidump = %s, pcmdump = %s", m_tcplog_status.c_str(), m_hcilog_status.c_str(), m_pcmlog_status.c_str());
    }
    else if (m_cmd == "readfile"){
        string filepath = HTTPRequest::URL::getParameter(m_url, "file");
        return this->readfile(filepath);
    }
    else if (m_cmd == "showbinfile") {
        string buf = property_get("ro.product.partitionpath");
        MYLOG("binfilepath buf= %s", buf.c_str());
        string binfilepath = buf + "miscdata";//HTTPRequest::URL::getParameter(m_url, "binfile");
        MYLOG("binfilepath = %s", binfilepath.c_str());
        return this->showbinfile(binfilepath);
    }
    else if (m_cmd == "property_get"){
        string key = HTTPRequest::URL::getParameter(m_url, "key");
        return this->property_get(key);
    }else if (m_cmd == "property_set"){
        string key = HTTPRequest::URL::getParameter(m_url, "key");
        string value = HTTPRequest::URL::getParameter(m_url, "value");
        return this->property_set(key, value);
    } else if (m_cmd == "shell" || m_cmd =="shellr"){
        string shellcommand = HTTPRequest::URL::getParameter(m_url, "shellcommand");
        /* SPRD:813850 - sc9820e memory test can't normaly run @{ */
        if (isPermittedtoExec(shellcommand)
            || strstr(shellcommand.c_str(), "memtester") != NULL
            || strstr(shellcommand.c_str(), "flashtest") != NULL) {
            return this->shell(shellcommand, "r");
        } else {
            MYLOG("command %s is not permitted ", shellcommand.c_str());
            return "{\"msg\":\"this command is not permitted\"}";
        }
        /* }@ */
    }else if (m_cmd == "shellw"){
        string shellcommand = HTTPRequest::URL::getParameter(m_url, "shellcommand");
        if(isPermittedtoExec(shellcommand)) {
             return this->shell(shellcommand, "w");
          }else{
             MYLOG("command %s is not permitted ", shellcommand.c_str());
             return "{\"msg\":\"this command is not permitted\"}";
         }
    }else if(m_cmd == "writephasecheck"){
     string station = HTTPRequest::URL::getParameter(m_url, "station");
     string value = HTTPRequest::URL::getParameter(m_url, "value");
     string type =  HTTPRequest::URL::getParameter(m_url, "type");
     return this->writephasecheck(type, station,value);

    }else if (m_cmd == ""){
        return "{\"msg\":\"please input cmd param\"}";
    }
    else if (m_cmd == "getlogstatus") {
        return string(m_tcplog_status) + string(m_hcilog_status) + string(m_pcmlog_status);
    }
    else if (m_cmd == "socket")
    {
        string socketCMD = HTTPRequest::URL::getParameter(m_url, "socketcmd");
        return handleSocket(socketCMD);
    }
    else
    {
        MYLOG("process can not known");

    }
    return process();
}

string ATProcesser::process()
{
    string content;
    #define MAX_RESPONSE_LEN 1024
    int responselen = 0;
    const char* atresptr;
    char atrsp[MAX_RESPONSE_LEN]={0};

    const char *request = m_cmd.c_str();
    int requestlen = m_cmd.length();
    string sims = HTTPRequest::URL::getParameter(m_url, "sim");

    int sim = 0;

    if (!sims.empty()){
        sim = atoi(sims.c_str());
    }

    cout << "cmd=" << m_cmd << ",sim=" << sim << endl;
    MYLOG("cmd= %s", m_cmd.c_str());
#ifdef BUILD_ENG
    MYLOG("request= %s", request);
    atresptr = sendCmd(sim, request);
    snprintf(atrsp, MAX_RESPONSE_LEN, "%s", atresptr);
    MYLOG("atrsp= %s", atrsp);
    content.append(atrsp);
    MYLOG("response.content= %s", content.c_str());
    if (strstr(request, "AT+SPDSP=65535,0,0,4096") != NULL && strstr(atrsp, "OK") != NULL) {
        MYLOG(" open pcm_log ok");
        property_set("persist.sys.sprd.pcmlog", "running");
    } else if (strstr(request, "AT+SPDSP=65535,0,0,0") != NULL && strstr(atrsp, "OK") != NULL) {
        MYLOG(" close pcm_log ok");
        property_set("persist.sys.sprd.pcmlog", "stopped");
    }
#endif
    cout << "response.content=" << content << endl;
    return content;
}

string ATProcesser::handleSocket(string socketcmd)
{
    int numRead = 0;
    char buf[128];
    int  times = 10;
    string m_result;
    string result = "ok";
    char socketName[128];
    char socketCMD[128];

    MYLOG("handleSocket socket_local_client in");
    memset(socketName, 0, sizeof(socketName));
    memset(socketCMD, 0, sizeof(socketCMD));

    if (strstr(socketcmd.c_str(), "openbqb") != NULL)
    {
        strcpy(socketName, "/data/misc/.bqb_ctrl");
        strcpy(socketCMD, "\r\nAT+SPBQBTEST=1\r\n");
    }
    else if (strstr(socketcmd.c_str(), "closebqb") != NULL)
    {
        strcpy(socketName, "/data/misc/.bqb_ctrl");
        strcpy(socketCMD, "\r\nAT+SPBQBTEST=0\r\n");
    }
    else if (strstr(socketcmd.c_str(), "bqbstatus") != NULL)
    {
        strcpy(socketName, "/data/misc/.bqb_ctrl");
        strcpy(socketCMD, "\r\nAT+SPBQBTEST=?\r\n");
    }
    else
    {
        strcpy(socketName, "/data/misc/.bqb_ctrl");
        strcpy(socketCMD, "\r\nAT+SPBQBTEST=?\r\n");
    }
    socketfd = socket_local_client(socketName, ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
    if(socketfd < 0) {
        MYLOG("handleSocket:socket_local_client failed %d", errno);
        return "fail";
    }
    MYLOG("handleSocket socket_local_client out");
    if (write(socketfd, socketCMD, strlen(socketCMD) + 1) <= 0)
    {
        MYLOG("handleSocket send AT COMM_CONTROLLER_TRIGGER failed");
        closeSocket();
        return "fail";
    }
    else
    {
        MYLOG("handleSocket send AT COMM_CONTROLLER_TRIGGER success!");
    }

    memset(buf, 0, sizeof(buf));
    do {
        MYLOG("handleSocket read in");
        numRead = read(socketfd, buf, sizeof(buf));
        m_result = string(buf);
        MYLOG("handleSocket read out --- numRead: %d, buf: %s\n", numRead, buf);
    } while(numRead < 0 && errno == EINTR);

    if (numRead <= 0) {
        MYLOG("handleSocket: error: got to reconnect");
        closeSocket();
        return "fail";
    }
    else if (strstr(socketcmd.c_str(), "openbqb") != NULL)
    {
        if (strstr(m_result.c_str(), "ENABLED") != NULL)
        {
            MYLOG("handleSocket ENABLED OK!");
            result = "ok";
        }
        else
        {
            MYLOG("handleSocket ENABLED Failed!");
            result = "fail";
        }
        MYLOG("handleSocket ENABLED %s\n", m_result.c_str());
    }
    else if (strstr(socketcmd.c_str(), "closebqb") != NULL)
    {
        if (strstr(m_result.c_str(), "DISABLE") != NULL)
        {
            MYLOG("handleSocket DISABLE OK!");
            result = "ok";
        }
        else
        {
            MYLOG("handleSocket DISABLE Failed!");
            result = "fail";
        }
        MYLOG("handleSocket DISABLE %s\n", m_result.c_str());
    }
    else if (strstr(socketcmd.c_str(), "bqbstatus") != NULL)
    {
        if (strstr(m_result.c_str(), "SPBQBTEST OK: BQB") != NULL)
        {
            MYLOG("handleSocket status open!");
            result = "open";
        }
        else
        {
            MYLOG("handleSocket status close!");
            result = "close";
        }
        MYLOG("handleSocket DISABLE %s\n", m_result.c_str());
    }
    closeSocket();
    return result;
}

void ATProcesser::closeSocket()
{
    if (socketfd >= 0)
    {
        close(socketfd);
        socketfd = -1;
    }
}

/*shiwei added*/
bool ATProcesser::isAscii(char b)
{
        if (b >= 0 && b <= 127) {
            return true;
        }
        return false;
}


 bool ATProcesser::checkPhaseCheck(string stream)
 {
    if(stream[0] == '5')
    {
        if((stream[1] == '0' || stream[1] == '1')
                && stream[2] == 'P' && stream[3] == 'S')
        {
            return true;
        }
    }else if(stream[0] == '9')
    {
        if(stream[1] == '0' && stream[2] == 'P' && stream[3] == 'S')
        {
            return true;
        }
    }
    return false;
}

string ATProcesser::getSn1(string stream)
{
    if (stream.empty()) {
        return "Invalid Sn1!";
    }
    if (!isAscii(stream[SN1_START_INDEX])) {
        return "Invalid Sn1!";
    }

    string sn1 = stream.substr(SN1_START_INDEX, SP09_MAX_SN_LEN);
    MYLOG("sn1= %s", sn1.c_str());
    return sn1;
}

string ATProcesser::getSn2(string stream)
{
    if (stream.empty()) {
        return "Invalid Sn2!";
    }
    if (!isAscii(stream[SN2_START_INDEX])) {
        return "Invalid Sn2!";
    }

    string sn2 = stream.substr(SN2_START_INDEX, SP09_MAX_SN_LEN);
    MYLOG("sn2= %s", sn2.c_str());
    return sn2;
}

bool ATProcesser::isStationTest(int station, string stream)
{
    int flag = 1;
    if(stream[0] == '5' && stream[1] == '1')
    {
        if (station < 8) {
            MYLOG("isStationTest station = %d, Testflag_start =0x%x",station,stream[TESTFLAG_START_INDEX_SP15]);  //stream=90PS32779059069917
            return (0 == ((flag << station) & stream[TESTFLAG_START_INDEX_SP15]));
        } else if (station >= 8 && station < 16) {
            MYLOG("isStationTest station = %d, Testflag_start =0x%x",station,stream[TESTFLAG_START_INDEX_SP15+1]);
            return (0 == ((flag << (station - 8)) & stream[TESTFLAG_START_INDEX_SP15 + 1]));
        } else if (station >= 16 && station < 20) {
            MYLOG("isStationTest station = %d, Testflag_start =0x%x",station,stream[TESTFLAG_START_INDEX_SP15+2]);
            return (0 == ((flag << (station - 16)) & stream[TESTFLAG_START_INDEX_SP15 + 2]));
        }
    }
    else
    {
        if (station < 8) {
            MYLOG("isStationTest station = %d, Testflag_start =0x%x",station,stream[TESTFLAG_START_INDEX]);  //stream=90PS32779059069917
            return (0 == ((flag << station) & stream[TESTFLAG_START_INDEX]));
        } else if (station >= 8 && station < 16) {
            MYLOG("isStationTest station = %d, Testflag_start =0x%x",station,stream[TESTFLAG_START_INDEX+1]);
            return (0 == ((flag << (station - 8)) & stream[TESTFLAG_START_INDEX + 1]));
        }
    }
    return false;
}

bool ATProcesser::isStationPass(int station, string stream)
{
    int flag = 1;
    if(stream[0] == '5' && stream[1] == '1')
    {
        if (station < 8) {
            MYLOG("isStationPass station = %d, Testflag_start =0x%x, Testflag_flag =0x%x",station,stream[RESULT_START_INDEX_SP15],stream[RESULT_START_FLAG_SP15]);
            if (stream[RESULT_START_FLAG_SP15] == 0) {
                MYLOG("isStationPass stream[RESULT_START_FLAG] == 0");
                return ((flag << station) & stream[RESULT_START_INDEX_SP15]);
            }
            else {
                MYLOG("isStationPass stream[RESULT_START_FLAG] == 1");
                return !((flag << station) & stream[RESULT_START_INDEX_SP15]);
            }
        } else if (station >= 8 && station < 16) {
            MYLOG("isStationPass station = %d, Testflag_start =0x%x",station,stream[RESULT_START_INDEX_SP15 +1]);
            return (0 == ((flag << (station - 8)) & stream[RESULT_START_INDEX_SP15 + 1]));
        } else if (station >= 16 && station < 20) {
            MYLOG("isStationPass station = %d, Testflag_start =0x%x",station,stream[RESULT_START_INDEX_SP15 +2]);
            return (0 == ((flag << (station - 16)) & stream[RESULT_START_INDEX_SP15 + 2]));
        }
    }
    else
    {
        if (station < 8) {
            MYLOG("isStationPass station = %d, Testflag_start =0x%x, Testflag_flag =0x%x",station,stream[RESULT_START_INDEX],stream[RESULT_START_FLAG]);
            if (stream[RESULT_START_FLAG] == 0) {
                MYLOG("isStationPass stream[RESULT_START_FLAG] == 0");
                return ((flag << station) & stream[RESULT_START_INDEX]);
            }
            else {
                MYLOG("isStationPass stream[RESULT_START_FLAG] == 1");
                return !((flag << station) & stream[RESULT_START_INDEX]);
            }
        } else if (station >= 8 && station < 16) {
            MYLOG("isStationPass station = %d, Testflag_start =0x%x",station,stream[RESULT_START_INDEX +1]);
            return (0 == ((flag << (station - 8)) & stream[RESULT_START_INDEX + 1]));
        }
    }
    return false;
}


string ATProcesser::getTestsAndResult(string stream)
{
    string testResult ;
    string allResult;
    int flag = 1;
    MYLOG("getTestsAndResult");
    if (stream.empty()) {
        return "Invalid Phase check!";
    }

    if (!isAscii(stream[STATION_START_INDEX])) {
        return "Invalid Phase check!";
    }
    MYLOG("getTestsAndResult stream = %s", stream.c_str());
    if(stream[0] == '5' && stream[1] == '1')
    {
        for (int i = 0; i < SP15_MAX_STATION_NUM; i++) {
            if (0 == stream[STATION_START_INDEX_SP15 + i * SP15_MAX_STATION_NAME_LEN]) {
                break;
            }
            testResult = stream.substr(STATION_START_INDEX_SP15 + i * SP15_MAX_STATION_NAME_LEN,
                SP15_MAX_STATION_NAME_LEN);

            MYLOG("getTestsAndResult i=%d,testResult = %s", i,testResult.c_str());

            if (!isStationTest(i, stream)) {
                MYLOG("getTestsAndResult i=%d,testResult = %s", i,"Not test");
                testResult += " Not test";
            } else if (isStationPass(i, stream)) {
                MYLOG("getTestsAndResult i=%d,testResult = %s", i,"Pass");
                testResult += " Pass";
            } else {
                testResult += " Failed";
                    MYLOG("getTestsAndResult i=%d,testResult = %s", i,"Failed");
            }
            flag = flag << 1;
            allResult += testResult + "\n";
        }
    } else
    {
        for (int i = 0; i < SP09_MAX_STATION_NUM; i++) {
            if (0 == stream[STATION_START_INDEX + i * SP09_MAX_STATION_NAME_LEN]) {
                break;
            }
            testResult = stream.substr(STATION_START_INDEX + i * SP09_MAX_STATION_NAME_LEN,
                SP09_MAX_STATION_NAME_LEN);

            MYLOG("getTestsAndResult i=%d,testResult = %s", i,testResult .c_str());
            if (!isStationTest(i, stream)) {
                MYLOG("getTestsAndResult i=%d,testResult = %s", i,"Not test");
                testResult += " Not test";
            } else if (isStationPass(i, stream)) {
                MYLOG("getTestsAndResult i=%d,testResult = %s", i,"Pass");
                testResult += " Pass";
            } else {
                testResult += " Failed";
                MYLOG("getTestsAndResult i=%d,testResult = %s", i,"Failed");
            }
            flag = flag << 1;
            allResult += testResult + "\n";
        }
    }
    MYLOG("******return allResult : %s*****", allResult.c_str());
    return allResult;
}

string ATProcesser::showbinfile(string binfilepath)
{
    string content;
    string result;
    FILE *fp1 = NULL;
    unsigned char buf1[500] = {0};
    int i;
    if((fp1 = fopen(binfilepath.c_str(), "rb"))==NULL)
    {
        result.append("open file error.");
        result.append("\n");
        return result;
    }
    else
    {
        for(i=0;i<500;i++)
        {
            fread(&buf1[i], sizeof(char), 1, fp1);
            if(buf1[i] > 0xff)
            {
                buf1[i]= buf1[i]&0xff;
            }
           // printf("showbinfile 0x%x, ", buf1[i]);
            content += buf1[i];
            //MYLOG("showbinfile binfile char =0x%x,content = %s", content[i],content.c_str());
        }
    }

    fclose(fp1);
    if(!checkPhaseCheck(content))
    {
        result.append("Check Phase Failed.");
        result.append("\n");
        return result;
    }

    result.append("SN1:");
    result.append("\n");
    result.append(getSn1(content));
    result.append("\n");
    result.append("SN2:");
    result.append("\n");
    result.append(getSn2(content));
    result.append("\n");
    result.append("\n");
    //cout<<getSn1(content)<<endl;
    //cout<<getSn2(content)<<endl;
    result.append(getTestsAndResult(content));
    result.append("\n");
    return result;
}

/* SPRD:838896 - KAIOS SGPS control config.xml thought engmoded @{ */
string ATProcesser::writeXML(string key, string value)
{
    TiXmlDocument doc(GNSS_CONFIG_FILE);
    bool loadOkay = doc.LoadFile();
    char  *cKey = NULL;
    char  *cValue = NULL;
    if (!loadOkay)
    {
        return false;
    }

    TiXmlNode *node = 0;
    TiXmlElement *datumElement = 0;
    cKey = (char*)key.c_str();
    cValue = (char*)value.c_str();

    node = doc.FirstChild("GNSS");
    for (node = node->FirstChild(); node != 0; node = node->NextSibling())
    {
        datumElement = node->ToElement();
        MYLOG(" writeXML TiXmlNode   NAME:%15s    VALUE:%15s    \r\n", datumElement->Attribute("NAME"),datumElement->Attribute("VALUE"));
        if ( strcmp( datumElement->Attribute("NAME"), cKey ) == 0 ) {
            MYLOG ("writeXML query okokokokok");
            datumElement->SetAttribute("VALUE", cValue);
            MYLOG("now writeXML TiXmlNode   NAME:%15s    VALUE:%15s \r\n", datumElement->Attribute("NAME"),datumElement->Attribute("VALUE"));
            doc.SaveFile(GNSS_CONFIG_FILE);
            return "success";
        }
    }
    return "fail";
}

string ATProcesser::readXML(string key)
{
    TiXmlDocument doc(GNSS_CONFIG_FILE);
    bool loadOkay = doc.LoadFile();
    char  *cKey = NULL;
    char  *cValue = NULL;
    if (!loadOkay)
    {
        return false;
    }

    TiXmlNode *node = 0;
    TiXmlElement *datumElement = 0;
    cKey = (char*)key.c_str();

    node = doc.FirstChild("GNSS");
    for (node = node->FirstChild(); node != 0; node = node->NextSibling())
    {
        datumElement = node->ToElement();
        MYLOG("readXML TiXmlNode   NAME:%20s    VALUE:%20s    MATCH%20s\r\n", datumElement->Attribute("NAME"),datumElement->Attribute("VALUE"), cKey);
        if ( strcmp( datumElement->Attribute("NAME"), cKey ) == 0 )
        {
            MYLOG ("readXML query okokokokok");
            datumElement->Attribute("VALUE");
            return datumElement->Attribute("VALUE");
        }
    }
    return NULL;
}
/* }@ */


/*shiwei added over*/

string ATProcesser::readfile(string filepath)
{
    string content;
    char buffer[1024] = {0};
    ifstream file(filepath.c_str());
    if (! file.is_open())
    { return content; }
    while (! file.eof() ) {
        memset(buffer, 0, sizeof(buffer));
        file.getline (buffer,sizeof(buffer));
        //cout << buffer << endl;
        content += buffer;
        content += "\n";
    }
    file.close();
    return content;
}

#ifdef BUILD_ENG
#include "cutils/properties.h"
#else
string property_get(char *key, char *value, char *default_value){return "1";};
string property_set(char *key, char *value){return "1";};
#endif

string ATProcesser::property_get(string key)
{
    char value[256];
    memset(value, 0, sizeof(value));
    ::property_get((char *)key.c_str(), value, "");
    string cmdstring = string("getprop ") + key;
    MYLOG("PROPERTY DEBUG property_get key = %s, value = %s.", key.c_str(), value);
    //return this->shell(cmdstring,"r");
    return string(value);
}
string ATProcesser::property_set(string key, string value)
{
    ::property_set((char *)key.c_str(), (char *)value.c_str());
    string cmdstring = string("setprop ") + key + " " + value;
    //return this->shell(cmdstring,"r");
    return "1";
}

string& replace_all(string& str,  const string& old_value,  const string& new_value)
{
    while (true) {
        string::size_type   pos(0);
        if((pos=str.find(old_value))!=string::npos) {
            str.replace(pos, old_value.length(), new_value);
        }
        else {
            break;
        }
    }
    return str;
}

/* SPRD:813850 - sc9820e memory test can't normaly run @{ */
string ATProcesser::getPIDAndKill(char *str) {
    char b[50][6];
    char *p = str;
    int i = 0, j = 0;
    FILE *fp;
    while (*p) {
        if(*p >= '0' && *p <= '9') {
            b[i][j++] = *p ++;
        }
        else
        {
            b[i][j] = 0;
            if(*(p + 1) >= '0' && *(p + 1) <= '9')
            {
                i ++;
                j = 0;
            }
            p ++;
        }
    }
    b[i][j] = 0;
    for (j = 0; j <= i; j ++) {
        string pid = b[j];
        string killPID = "kill " + pid;
        MYLOG("killPID = %s\n", killPID.c_str());
        if (!(fp = popen(killPID.c_str(), "r"))) {
            MYLOG(" popen PROPERTY fail");
            return "fail";
        }
        pclose(fp);
    }
    return "success";
}

string ATProcesser::stopMemtester() {
    MYLOG("stopMemtester");
    FILE *fp;
    char buff[512];
    string result;
    string pidList[30];
    if(!(fp = popen("ps -Z | grep memtester", "r"))){
        MYLOG(" popen PROPERTY DEBUG shell  result = %s.", result.c_str());
        return result;
    }
    memset(buff, 0, sizeof(buff));
    while (fgets(buff, sizeof(buff), fp) != NULL) {
        MYLOG("popen fgets result = %s", result.c_str());
        getPIDAndKill(buff);
        result.append(buff);
        memset(buff, 0, sizeof(buff));
    }
    pclose(fp);
    return result;
}
/* }@ */

/* SPRD:965545 - support flash test @{ */
string ATProcesser::stopFlashtest() {
    MYLOG("stopFlashtest");
    FILE *fp;
    char buff[512];
    string result;
    string pidList[30];
    if(!(fp = popen("ps -Z | grep flashtest", "r"))){
        MYLOG(" popen PROPERTY DEBUG shell  result = %s.", result.c_str());
        return result;
    }
    memset(buff, 0, sizeof(buff));
    while (fgets(buff, sizeof(buff), fp) != NULL) {
        MYLOG("popen fgets result = %s", result.c_str());
        getPIDAndKill(buff);
        result.append(buff);
        memset(buff, 0, sizeof(buff));
    }
    pclose(fp);
    return result;
}
/* }@ */

string ATProcesser::shell(string cmd, string rw)
{
    FILE *in;
    char buff[512];
    string result;
    MYLOG("shell cmd = %s", cmd.c_str());
    if (strstr(cmd.c_str(), "wcnd") != NULL) {
        cmd = replace_all(cmd, "wcnd", "connmgr");
        MYLOG("shell cmd convert: = %s", cmd.c_str());
    /* SPRD:813850 - sc9820e memory test can't normaly run @{ */
    } else if (strstr(cmd.c_str(), "stopmemtester") != NULL) {
        return stopMemtester();
    }
    /* }@ */
	/* SPRD:965545 - support flash test @{ */
	else if (strstr(cmd.c_str(), "stopflashtest") != NULL) {
        return stopFlashtest();
    }
	/* }@ */
    if (!(in = popen(cmd.c_str(), rw.c_str()))) {
        MYLOG(" popen PROPERTY DEBUG shell cmd = %s, result = %s.", cmd.c_str(), result.c_str());
        return result;
    }

    memset(buff, 0, sizeof(buff));
    while (fgets(buff, sizeof(buff), in)!=NULL) {
        result.append(buff);
        memset(buff, 0, sizeof(buff));
    }
    pclose(in);
    MYLOG("PROPERTY DEBUG shell cmd = %s, result = %s.", cmd.c_str(), result.c_str());
    return result;
}

unsigned int ATProcesser:: get_magic(){
        int readnum;
        unsigned int magic = 0;
        FILE* fd=fopen(phasecheckPath.c_str(),"r");
        if(NULL != fd)
        {
            readnum = fread(&magic, sizeof(unsigned int), 1, fd);
            MYLOG("get magic: 0x%x!, %u!\n", magic, magic);
            fclose(fd);
        }
        return magic;
    }

string ATProcesser:: writephasecheck(string type, string station, string value)
{
      string result;
    switch(get_magic()){
        case SP05_SPPH_MAGIC_NUMBER:
        case SP09_SPPH_MAGIC_NUMBER:
        MYLOG("writephasecheck go to sp09");
        result = writephasecheckforsp09(type, station, value);
        break;

        case SP15_SPPH_MAGIC_NUMBER:
        MYLOG("writephasecheck go to sp15");
        result =writephasecheckforsp15(type,station,value);
        break;
        default:
            MYLOG("writephasecheck magic read is %d",get_magic());
        break;
    }
  return result;

}

/* SPRD:939549 - phasecheck write MMI station fail @{ */
string ATProcesser:: writephasecheckforsp09(string type, string station, string value)
{
    int len=0;
    SP09_PHASE_CHECK_T Phase;
    unsigned int i=0, index =0;
    FILE *fp  = NULL;
    string result;

    memset(&Phase,0,sizeof(SP09_PHASE_CHECK_T));
    MYLOG(" writephasecheck type = %s,station = %s, value = %s", type.c_str(), station.c_str(), value.c_str());
    int fd = open(phasecheckPath.c_str(), O_RDWR);
    if (fd >= 0)
    {
        MYLOG("writephasecheck open Ok phasecheckPath = %s \n", phasecheckPath.c_str());
        len = read(fd,&Phase,sizeof(SP09_PHASE_CHECK_T));
        //close(fd);
        if (len <= 0)
        {
            MYLOG("writephasecheck read fail phasecheckPath = %s \n", phasecheckPath.c_str());
            result.append("read file error.");
            result.append("\n");
            return result;
        }
    }
    else
    {
        MYLOG("writephasecheck open fail phasecheckPath = %s \n" , phasecheckPath.c_str());
        result.append("open file error.");
        result.append("\n");
        return result;
    }
    for (i = 0; i < Phase.StationNum; i ++)
    {
        if (0 == station.compare(Phase.StationName[i]))
        {
            // Station is found
            MYLOG("writephasecheck station %s is found, index = %d", station.c_str(),i);
            index = i;
            break;
        }
    }
    if (i >= Phase.StationNum)
    {
        MYLOG("writephasecheck not find the station");
        result.append("not find the station.");
        result.append("\n");
        return result;
    }
    //fd = open(phasecheckPath.c_str(), O_RDWR);
    //if (fd >= 0)
    {
        int oper_type ,oper_value =0;
        oper_type = atoi(type.c_str());
        oper_value = atoi(value.c_str());

        MYLOG(" writephasecheck type = %d, value =%d", oper_type, oper_value);
        if(oper_type== SIGN_TYPE)
        {
            if(oper_value)
            {
                Phase.iTestSign |= (unsigned short)(1<<index);
            }
            else
            {
                Phase.iTestSign &= (unsigned short)(~(1<<index));
            }
            if (strstr(phasecheckPath.c_str(), "ubi") != NULL)
            {
                __s64 up_sz = sizeof(SP09_PHASE_CHECK_T);
                len = ioctl(fd, UBI_IOCVOLUP, &up_sz);
            }
			lseek(fd, 0, SEEK_SET);
            len = write(fd,&Phase,sizeof(SP09_PHASE_CHECK_T));
            fsync(fd);
            MYLOG(" writephasecheck phasecheck_sprd iTestSign 0x%x'", Phase.iTestSign);
        }
        else if(oper_type == ITEM_TYPE)
        {
            if(oper_value)
            {  //fail
                Phase.iItem |= (unsigned short)(1<<index);
            }
            else
            {
                //success
                Phase.iItem &= (unsigned short)(~(1<<index));
                // Phase.iItem &= (unsigned short)(0xFFFF & (~(unsigned short)(1 << index)));
            }
            if (strstr(phasecheckPath.c_str(), "ubi") != NULL)
            {
                __s64 up_sz = sizeof(SP09_PHASE_CHECK_T);
                len = ioctl(fd, UBI_IOCVOLUP, &up_sz);
            }
	 		lseek(fd, 0, SEEK_SET);
            len = write(fd,&Phase,sizeof(SP09_PHASE_CHECK_T));
            fsync(fd);
            MYLOG("writephasecheck phasecheck_sprd iItem 0x%x'", Phase.iItem);
        }
        close(fd);
    }
    result.append("write phasecheck OK.");
    return result;
}


string ATProcesser:: writephasecheckforsp15(string type, string station, string value)
{
    int i = 0, len = 0,index = 0;
    SP15_PHASE_CHECK_T Phase;
    FILE *fp = NULL;
    string result;

    memset(&Phase, 0, sizeof(SP15_PHASE_CHECK_T));
    MYLOG(" writephasecheck type = %s,station = %s, value = %s", type.c_str(), station.c_str(), value.c_str());
    int fd = open(phasecheckPath.c_str(), O_RDWR);
    if (fd >= 0)
    {
        MYLOG("writephasecheck open Ok phasecheckPath = %s \n", phasecheckPath.c_str());
        len = read(fd, &Phase, sizeof(SP15_PHASE_CHECK_T));
        //close(fd);

        if (len <= 0)
        {
            MYLOG("writephasecheck read fail phasecheckPath = %s \n", phasecheckPath.c_str());
            result.append("read file error.");
            result.append("\n");
            return result;
        }
    }
    else
    {
        MYLOG("writephasecheck open fail phasecheckPath = %s \n" , phasecheckPath.c_str());
        result.append("open file error.");
        result.append("\n");
        return result;
    }

    for (i=0; i<Phase.StationNum; i++)
    {
        if (0 == station.compare(Phase.StationName[i]))
        {
            // Station is found
            MYLOG("writephasecheck station  %s is found,index =%d",station.c_str(),i);
            index = i;
            break;
        }

    }

    if (i >= Phase.StationNum)
    {
        MYLOG("writephasecheck not find the station");
        result.append("not find the station.");
        result.append("\n");
        return result;
    }

    {
        int oper_type ,oper_value =0;
        oper_type = atoi(type.c_str());
        oper_value = atoi(value.c_str());
        MYLOG(" writephasecheck type = %d, value =%d'", oper_type,oper_value);
        if(oper_type == SIGN_TYPE)
        {
            if(oper_value)
            {
                Phase.iTestSign |= (unsigned short)(1<<index);
            }
            else
            {
                Phase.iTestSign &= (unsigned short)(~(1<<index));
            }
            if (strstr(phasecheckPath.c_str(), "ubi") != NULL)
            {
                __s64 up_sz = sizeof(SP15_PHASE_CHECK_T);
                len = ioctl(fd, UBI_IOCVOLUP, &up_sz);
            }
			lseek(fd, 0, SEEK_SET);
            len = write(fd, &Phase, sizeof(SP15_PHASE_CHECK_T));

            fsync(fd);
            MYLOG(" writephasecheck phasecheck_sprd iTestSign 0x%x'", Phase.iTestSign);
        }
        else if(oper_type == ITEM_TYPE)
        {
            if(oper_value)
            {
                Phase.iItem |= (unsigned short)(1<<index);
            }
            else
            {
                Phase.iItem &= (unsigned short)(~(1<<index));
            }
            if (strstr(phasecheckPath.c_str(), "ubi") != NULL)
            {
                __s64 up_sz = sizeof(SP15_PHASE_CHECK_T);
                len = ioctl(fd, UBI_IOCVOLUP, &up_sz);
            }
			lseek(fd, 0, SEEK_SET);
            len = write(fd, &Phase, sizeof(SP15_PHASE_CHECK_T));
            fsync(fd);
            MYLOG(" writephasecheck phasecheck_sprd iTestSign 0x%x'", Phase.iTestSign);
        }
        close(fd);
    }

    result.append("write phasecheck OK.");
    return result;
}
/* }@ */

bool ATProcesser::isPermittedtoExec(string cmd)
{
     int i = 0;
     string tempcmd = cmd;
     size_t pos =0;

     for (i = 0; i < sizeof(whitelist) / sizeof(char *); i++) {
         pos = tempcmd.find(whitelist[i]);
         if(pos !=string::npos)
         {
               MYLOG("isPermittedtoExec------find in whitelist\n");
               return true;
         }
     }
     MYLOG("isPermittedtoExec------not find in whitelist\n");
     return false;
}
