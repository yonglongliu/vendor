#include "ImsConnManMonitor.h"
#include "HplmnUtils.h"
#include "NetworkUtils.h"
#include "Constants.h"
#include "VersionInfo.h"
#include "portinglayer/PortingInterface.h"
#include "portinglayer/PortingLog.h"


BEGIN_VOWIFI_NAMESPACE

const string ImsConnManMonitor::TAG = "ImsConnManMonitor";

ImsConnManMonitor::ImsConnManMonitor()
    : mpIccListener(nullptr)
    , mpShutdownListener(nullptr)
    , mpTeleListener(nullptr)
    , mpImsServiceListener(nullptr)
    , mpImsRegServiceListener(nullptr)
    , mPrimaryPhoneId(INVALID_PHONE_ID)
    , mPrimaryHplmn("")
    , mOldPrimarySimState(SIM_STATE_ABSENT)
    , mNewPrimarySimState(SIM_STATE_ABSENT) {
}

ImsConnManMonitor::~ImsConnManMonitor() {
}

ImsConnManMonitor& ImsConnManMonitor::getInstance() {
    static ImsConnManMonitor instance;

    return instance;
}

void ImsConnManMonitor::onInit() {
    LOGD(_TAG(TAG), "initialize, Version info: %s", STR_VERSION_INFO);

    mpIccListener = new ImsConnManIccListener();
    mpIccListener->registerListener();

    mpTeleListener = new ImsConnManTelephonyListener();
    mpTeleListener->registerListener();

    NetworkUtils::createReadDatabaseTask();
    NetworkUtils::registerListenerAfterBoot();
}

void ImsConnManMonitor::onPrepareBindOperatorService(int phoneId) {
    mPrimaryPhoneId = phoneId;
    mPrimaryHplmn = HplmnUtils::getHplmn(mPrimaryPhoneId);
    LOGD(_TAG(TAG), "onPrepareBindOperatorService: mPrimaryPhoneId = %d, mPrimaryHplmn = %s", \
            mPrimaryPhoneId, mPrimaryHplmn.c_str());

    if (mPrimaryHplmn.empty()) {
        return;
    }

    onBindOperatorService();
}

void ImsConnManMonitor::onBindOperatorService() {
    if (ImsConnManStateInfo::isServiceAttached()) {
        LOGE(_TAG(TAG), "onBindOperatorService: Service has been \"Attached\", don't attach service again !!!!!");
        return;
    }

    if (!ImsConnManStateInfo::isWifiCallingEnabled()) {
        LOGE(_TAG(TAG), "onBindOperatorService: Wifi-calling is disalbed, don't attach service !!!!!");
        return;
    }

    if (INVALID_PHONE_ID == mPrimaryPhoneId) {
        LOGE(_TAG(TAG), "onBindOperatorService: Invalid mPrimaryPhoneId = %d, don't attach service !!!!!", mPrimaryPhoneId);
        return;
    }

    if (SIM_CARD_TYPE_NOT_SIM != PortingInterface::getIccCardType()) {
        LOGE(_TAG(TAG), "onBindOperatorService: Card %d is SIM card, don't attach service !!!!!", mPrimaryPhoneId);

        mPrimaryPhoneId = INVALID_PHONE_ID;
        return;
    }

    if (OPERATOR_NAME_ID_INVALID == HplmnUtils::currentPolicyOperatorId()) {
        LOGE(_TAG(TAG), "onBindOperatorService: PolicyOperatorId is invalid !!!!!");
        mPrimaryPhoneId = INVALID_PHONE_ID;
        return;
    }

    LOGD(_TAG(TAG), "[---Attach Service---]");
    mpService.reset(new ImsConnManService());
    if (nullptr != mpService) mpService->onAttach(HplmnUtils::currentPolicyOperatorId());

    if (nullptr == mpShutdownListener) mpShutdownListener = new ImsConnManShutdownListener();
    mpShutdownListener->registerListener();

    NetworkUtils::registerListenerAfterAttachService();

    if (nullptr == mpImsServiceListener) {
        mpImsServiceListener = new ImsServiceCallbackListener();
        PortingInterface::setImsServiceCallbackListener(mpImsServiceListener);
    }

    if (nullptr == mpImsRegServiceListener) mpImsRegServiceListener = new ImsRegisterServiceListener();
    mpImsRegServiceListener->registerListener();

    mpTeleListener->update(mpService);
    NetworkUtils::update(mpService);
    mpImsServiceListener->update(mpService);
    mpImsRegServiceListener->update(mpService);
}

void ImsConnManMonitor::onUnbindOperatorService() {
    if (!ImsConnManStateInfo::isServiceAttached()) {
        LOGE(_TAG(TAG), "onUnbindOperatorService: Service has been \"Detached\", don't detach service again !!!!!");
        return;
    }

    LOGD(_TAG(TAG), "[---Detach Service---]");

    mPrimaryPhoneId = INVALID_PHONE_ID;
    mPrimaryHplmn.assign("");

    if (nullptr != mpService) mpService->onDetach();
    if (nullptr != mpService) mpService.reset();

    if (nullptr != mpShutdownListener) mpShutdownListener->unregisterListener();
    NetworkUtils::unregisterListenerAfterDetachService();
    if (nullptr != mpImsRegServiceListener) mpImsRegServiceListener->unregisterListener();

    mpTeleListener->update(mpService);
    NetworkUtils::update(mpService);
    mpImsServiceListener->update(mpService);
    mpImsRegServiceListener->update(mpService);
}

void ImsConnManMonitor::updateSimCardState(int simCardState, int slotId) {
    LOGD(_TAG(TAG), "updateSimState: Slot %d simCardState = %d", slotId, simCardState);
    mNewPrimarySimState = simCardState;

    int primaryPhoneId = PortingInterface::getDefaultDataPhoneId();
    if (INVALID_PHONE_ID == primaryPhoneId) {
        LOGE(_TAG(TAG), "updateSimState: Invalid primary phone id: %d", primaryPhoneId);
        return;
    }

    bool isExistSimCard = PortingInterface::hasIccCard(primaryPhoneId);
    LOGD(_TAG(TAG), "updateSimState: Service has been %s, primaryPhoneId: %d, mPrimaryPhoneId = %d, isExistSimCard = %s", \
            (ImsConnManStateInfo::isServiceAttached() ? "\"Attached\"" : "\"Detached\""), \
            primaryPhoneId, \
            mPrimaryPhoneId, \
            isExistSimCard ? "true" : "false");

    if (slotId == primaryPhoneId) {
        LOGD(_TAG(TAG), "updateSimState: mOldPrimarySimState = %d, mNewPrimarySimState = %d", \
                mOldPrimarySimState, mNewPrimarySimState);

        if (ImsConnManStateInfo::isServiceAttached()) {
            if (!isExistSimCard
                    && SIM_STATE_READY == mOldPrimarySimState
                    && SIM_STATE_READY != mNewPrimarySimState) {
                //USIM card is UNKNOWN, ABSENT, LOCKED and so on
                onUnbindOperatorService();
            }
        } else {
            if (SIM_STATE_READY == mNewPrimarySimState) {
                onPrepareBindOperatorService(primaryPhoneId);
            }
        }

        mOldPrimarySimState = mNewPrimarySimState;
    }
}

END_VOWIFI_NAMESPACE
