#ifndef __ATPROCESSER_H__
#define __ATPROCESSER_H__
#include<iostream>
#include<fstream>
#include<string>
#include<cstring>
#include<stdlib.h>

using namespace std;


#define PHASE_CHECKE_FILE  "/dev/block/platform/soc/soc:ap-ahb/20600000.sdio/by-name/miscdata"
/* SPRD:838896 - KAIOS SGPS control config.xml thought engmoded @{ */
#define GNSS_CONFIG_FILE "/data/gnss/config/config.xml"
/* }@ */
#define CONTROLLER_BQB_SOCKET "/data/misc/.bqb_ctrl"
#define COMM_CONTROLLER_ENABLE "\r\nAT+SPBQBTEST=1\r\n"
#define COMM_CONTROLLER_DISABLE "\r\nAT+SPBQBTEST=0\r\n"
#define COMM_CONTROLLER_TRIGGER "\r\nAT+SPBQBTEST=?\r\n"

#define SIGN_TYPE 0
#define ITEM_TYPE 1


#define MAX_SN_LEN 24
#define SP09_MAX_SN_LEN                    MAX_SN_LEN
#define SP09_MAX_STATION_NUM               (15)
#define SP09_MAX_STATION_NAME_LEN          (10)
#define SP09_SPPH_MAGIC_NUMBER             (0X53503039)    // "SP09"
#define SP05_SPPH_MAGIC_NUMBER             (0X53503035)    // "SP05"
#define SP09_MAX_LAST_DESCRIPTION_LEN      (32)

/*add the struct add define to support the sp15*/
#define SP15_MAX_SN_LEN                 (64)
#define SP15_MAX_STATION_NUM            (20)
#define SP15_MAX_STATION_NAME_LEN       (15)
#define SP15_SPPH_MAGIC_NUMBER          (0X53503135)    // "SP15"
#define SP15_MAX_LAST_DESCRIPTION_LEN   (34)

typedef struct _tagSP15_PHASE_CHECK {
       unsigned int Magic;         // "SP15"
       char SN1[SP15_MAX_SN_LEN];  // SN , SN_LEN=64
       char SN2[SP15_MAX_SN_LEN];  // add for Mobile
       unsigned int StationNum;             // the test station number of the testing
       char StationName[SP15_MAX_STATION_NUM][SP15_MAX_STATION_NAME_LEN];
       unsigned char Reserved[13]; //
       unsigned char SignFlag;
       char szLastFailDescription[SP15_MAX_LAST_DESCRIPTION_LEN];
       unsigned long iTestSign;     // Bit0~Bit14 ---> station0~station 14
       //if tested. 0: tested, 1: not tested
       unsigned long iItem;         // part1: Bit0~ Bit_14 indicate test Station, 0: Pass, 1: fail
} SP15_PHASE_CHECK_T, *LPSP15_PHASE_CHECK_T;

typedef struct _tagSP09_PHASE_CHECK
{
    unsigned int Magic;           // "SP09"   (\C0\CF\u0153ӿ\DAΪSP05)
    char    SN1[SP09_MAX_SN_LEN]; // SN , SN_LEN=24
    char    SN2[SP09_MAX_SN_LEN]; // add for Mobile
    unsigned int StationNum;      // the test station number of the testing
    char    StationName[SP09_MAX_STATION_NUM][SP09_MAX_STATION_NAME_LEN];
    unsigned char Reserved[13];
    unsigned char SignFlag;
    char    szLastFailDescription[SP09_MAX_LAST_DESCRIPTION_LEN];
    unsigned short  iTestSign;    // Bit0~Bit14 ---> station0~station 14
    //if tested. 0: tested, 1: not tested
    unsigned short  iItem;        // part1: Bit0~ Bit_14 indicate test Station,1\B1\ED\CA\u0178Pass,

}SP09_PHASE_CHECK_T, *LPSP09_PHASE_CHECK_T;

//////////////////////////////////////////////////////////////////////////

class ATProcesser
{
public:
    ATProcesser(string url);
    virtual ~ATProcesser();

    string response();

private:
    int socketfd = -1;
    string m_cmd;
    string m_url;
    string process();
    string handleSocket(string socketcmd);
    void closeSocket();
    /*shiwei add*/
    string m_tcplog_status;
    string m_hcilog_status;
    string m_pcmlog_status;
    /* SPRD bug 842932:miscdata path issue*/
    string phasecheckPath;

    static const int SN1_START_INDEX = 4;
    static const int SN2_START_INDEX = SN1_START_INDEX + SP09_MAX_SN_LEN;

    static const int STATION_START_INDEX = 56;
    static const int STATION_START_INDEX_SP15 = 136;
    static const int TESTFLAG_START_INDEX = 252;
    static const int RESULT_START_INDEX = 254;
    static const int RESULT_START_FLAG = 219;

    static const int TESTFLAG_START_INDEX_SP15 = 484;
    static const int RESULT_START_INDEX_SP15 = 488;
    static const int RESULT_START_FLAG_SP15 = 449;

    bool isAscii(char b);
    bool checkPhaseCheck(string stream);
    string getSn1(string stream);
    string getSn2(string stream);
    bool isStationTest(int station, string stream);
    bool isStationPass(int station, string stream);

    string getTestsAndResult(string stream);
    string writephasecheckforsp09(string type, string station, string value);
    string writephasecheckforsp15(string type, string station, string value);
    unsigned int get_magic();
    bool isPermittedtoExec(string cmd);
public:
    string readfile(string filepath);
    string property_get(string key);
    string property_set(string key, string value);
    string shell(string cmd, string rw);
    /*shiwei add*/
    string showbinfile(string binfilepath);
    string writephasecheck(string type, string station, string value);
    /* SPRD:838896 - KAIOS SGPS control config.xml thought engmoded @{ */
    string writeXML(string key, string value);
    string readXML(string key);
    /* }@ */
    /* SPRD:813850 - sc9820e memory test can't normaly run @{ */
    string stopMemtester();
    string getPIDAndKill(char *key);
    /* }@ */
    /* SPRD:965545 - support flash test @{ */
    string stopFlashtest();
    /* }@ */
};

#endif