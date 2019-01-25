#include "MessageAsyncTask.h"


BEGIN_VOWIFI_NAMESPACE

const string MessageAsyncTask::TAG = "MessageAsyncTask";

MessageAsyncTask::MessageAsyncTask() {
}

MessageAsyncTask::~MessageAsyncTask() {
}

void MessageAsyncTask::updateMessage(const shared_ptr<Message> &message) {
    mMessage = message;
}

NS_IMETHODIMP MessageAsyncTask::Run() {
    if (PORTING_DEBUG) LOGD(_PTAG(TAG), "RUN: mMessage.what = %d", mMessage->what);
    mMessage->target->handleMessage(mMessage);

    return NS_OK;
}

END_VOWIFI_NAMESPACE