#include "HplmnUtils.h"
#include "Constants.h"
#include "TextUtils.h"
#include "ImsConnManUtils.h"
#include "portinglayer/PortingLog.h"
#include "portinglayer/PortingInterface.h"
#include "portinglayer/PortingConstants.h"

#include <stdlib.h>


BEGIN_VOWIFI_NAMESPACE

const string HplmnUtils::TAG = "HplmnUtils";
int HplmnUtils::mPolicyOperatorId = OPERATOR_NAME_ID_INVALID;

typedef struct {
    string operatorNumeric;
    string operatorName;
} OperatorInfo;

static OperatorInfo sOperatorInfo[] = {
    /*** Reliance ***/
    {"405840", "reliance"},
    {"405854", "reliance"},
    {"405855", "reliance"},
    {"405856", "reliance"},
    {"405857", "reliance"},
    {"405858", "reliance"},
    {"405859", "reliance"},
    {"405860", "reliance"},
    {"405861", "reliance"},
    {"405862", "reliance"},
    {"405863", "reliance"},
    {"405864", "reliance"},
    {"405865", "reliance"},
    {"405866", "reliance"},
    {"405867", "reliance"},
    {"405868", "reliance"},
    {"405869", "reliance"},
    {"405870", "reliance"},
    {"405871", "reliance"},
    {"405872", "reliance"},
    {"405873", "reliance"},
    {"405874", "reliance"},
};

HplmnUtils::HplmnUtils() {
}

HplmnUtils::~HplmnUtils() {
}

string HplmnUtils::getHplmn(int phoneId) {
    string primaryHplmn;

    if (INVALID_PHONE_ID == phoneId) {
        LOGE(_TAG(TAG), "getHplmn: Invalid phone id: %d", phoneId);
        return primaryHplmn;
    }

    unsigned int hplmnLength = 6;
    string mcc, mnc;
    string mcc_tag = "mcc", mnc_tag = "mnc";
    string isimInfo = PortingInterface::getIsimImpi(phoneId);
    LOGD(_TAG(TAG), "getHplmn: isimInfo = %s", isimInfo.c_str());

    if (!isimInfo.empty()) {
        if (string::npos != isimInfo.find_first_of(mcc_tag) && string::npos != isimInfo.find_first_of(mnc_tag)) {
            vector<string> vector_str;
            vector<string>::iterator iter;

            TextUtils::splitString(isimInfo, ".", vector_str);
            if (vector_str.empty()) LOGE(_TAG(TAG), "getHplmn: vector is empty !!!!!");

            for (iter = vector_str.begin(); iter != vector_str.end(); iter++) {
                string element = *iter;

                if (string::npos != element.find_first_of(mcc_tag)) {
                    mcc = element.substr(mcc_tag.size());
                } else if (string::npos != element.find_first_of(mnc_tag)) {
                    mnc = element.substr(mnc_tag.size());
                }
            }

            primaryHplmn = mcc + mnc;
            LOGD(_TAG(TAG), "getHplmn: get primary HPLMN from getIsimImpi(): %s, this is ISIM card", primaryHplmn.c_str());

            goto finished;
        } else {
            LOGD(_TAG(TAG), "getHplmn: this ISIM card isn't include mcc/mnc field");
        }
    }

    primaryHplmn = PortingInterface::getSimOperatorNumeric(phoneId);
    LOGD(_TAG(TAG), "getHplmn: get primary HPLMN from getSimOperatorNumeric(): %s", primaryHplmn.c_str());
    if (primaryHplmn.empty()) {
        LOGE(_TAG(TAG), "getHplmn: HPLMN is empty !!!!!");
        return primaryHplmn;
    }

finished:
    if (primaryHplmn.size() < hplmnLength) {
        mcc = primaryHplmn.substr(0, mcc_tag.size());
        mnc = "0" + primaryHplmn.substr(mcc_tag.size());
        primaryHplmn = mcc + mnc;
        LOGD(_TAG(TAG), "getHplmn: update primary HPLMN: %s", primaryHplmn.c_str());
    }

    adaptPolicyOperatorId(primaryHplmn);
    return primaryHplmn;
}

int HplmnUtils::adaptPolicyOperatorId(string& hplmn) {
    string operatorName = OPERATOR_NAME_STRING_UNKNOWN;

    if (hplmn.empty()) {
        LOGE(_TAG(TAG), "adaptPolicyOperatorId: HPLMN is empty, please check !!!!!");
        return mPolicyOperatorId;
    }

    for (uint32_t i = 0; i < sizeof(sOperatorInfo)/sizeof(OperatorInfo); i++) {
        if (0 == hplmn.compare(sOperatorInfo[i].operatorNumeric)) {
            operatorName = sOperatorInfo[i].operatorName;
            if (0 == operatorName.compare(OPERATOR_NAME_STRING_RELIANCE)) {
                mPolicyOperatorId = OPERATOR_NAME_ID_RELIANCE;
            }
            break;
        }
    }

    if (OPERATOR_NAME_ID_INVALID == mPolicyOperatorId) {
        int mcc = atoi(hplmn.substr(0, 3).c_str());

        if (mcc < VALID_IMSI_MCC || ImsConnManUtils::isVowifiLabSimEnable() || ImsConnManUtils::isVowifiWhitelistEnable()) {
            mPolicyOperatorId = OPERATOR_NAME_ID_DEFAULT;
        } else {
            mPolicyOperatorId = OPERATOR_NAME_ID_INVALID;
            LOGE(_TAG(TAG), "adaptPolicyOperatorId: didn't find HPLMN in sOperatorInfo and not Lab's card, mPolicyOperatorId = %d", \
                    mPolicyOperatorId);
        }
    }

    return mPolicyOperatorId;
}

int HplmnUtils::currentPolicyOperatorId() {
    return mPolicyOperatorId;
}

void HplmnUtils::resetPolicyOperatorId() {
    mPolicyOperatorId = OPERATOR_NAME_ID_INVALID;
}

END_VOWIFI_NAMESPACE