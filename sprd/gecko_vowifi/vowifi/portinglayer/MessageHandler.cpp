#include "MessageHandler.h"
#include "nsXPCOMCIDInternal.h"


BEGIN_VOWIFI_NAMESPACE

const string MessageHandler::TAG = "MessageHandler";

MessageHandler::MessageHandler() {
    nsCOMPtr<nsIThreadManager> threadManager = do_GetService(NS_THREADMANAGER_CONTRACTID);
    mResult = threadManager->NewThread(0, 0, getter_AddRefs(mThread));
    if (NS_FAILED(mResult)) {
        LOGE(_PTAG(TAG), "create thread failed, please check !!!!!");
    }

    mMessageAsyncTask = new MessageAsyncTask();
}

MessageHandler::~MessageHandler() {
    mThread->Shutdown();
}

shared_ptr<Message> MessageHandler::obtainMessage(int what, void *obj) {
    return Message::obtain(this, what, obj);
}

shared_ptr<Message> MessageHandler::obtainMessage(int what, int arg1, int arg2, void *obj) {
    return Message::obtain(this, what, arg1, arg2, obj);
}

bool MessageHandler::sendEmptyMessage(int what) {
    if (NS_FAILED(mResult)) {
        LOGE(_PTAG(TAG), "sendEmptyMessage: inexistence thread for send message, please check !!!!!");
        return false;
    }

    shared_ptr<Message> msg = Message::obtain(this, what);
    sendMessage(msg);

    return true;
}

bool MessageHandler::sendMessage(const shared_ptr<Message> &msg, uint32_t dispatchFlag) {
    if (NS_FAILED(mResult)) {
        LOGE(_PTAG(TAG), "sendMessage: inexistence thread for send message, please check !!!!!");
        return false;
    }

    if (PORTING_DEBUG) LOGE(_PTAG(TAG), "sendMessage: Dispatch message.what = %d", msg->what);
    MessageAsyncTask *p = static_cast<MessageAsyncTask *>(mMessageAsyncTask.get());
    if (nullptr != p) p->updateMessage(msg);
    mThread->Dispatch(mMessageAsyncTask, dispatchFlag);
    //mThread->Dispatch(mMessageAsyncTask, NS_DISPATCH_SYNC);
    //mThread->Dispatch(mMessageAsyncTask, NS_DISPATCH_NORMAL);

    return true;
}

void MessageHandler::dispatchMessage(const shared_ptr<Message> &msg) {
}

END_VOWIFI_NAMESPACE