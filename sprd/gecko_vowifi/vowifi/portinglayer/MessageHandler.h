#ifndef __SPRD_VOWIFI_PORTING_MESSAGE_HANDLER_H__
#define __SPRD_VOWIFI_PORTING_MESSAGE_HANDLER_H__

#include "nsThreadUtils.h"
#include <string>

using std::string;
using std::shared_ptr;


BEGIN_VOWIFI_NAMESPACE
class Message;
class MessageAsyncTask;

class MessageHandler {
public:
    explicit MessageHandler();
    virtual ~MessageHandler();

    virtual void handleMessage(const shared_ptr<Message> &msg) = 0;

    shared_ptr<Message> obtainMessage(int what, void *obj);
    shared_ptr<Message> obtainMessage(int what, int arg1 = 0, int arg2 = 0, void *obj = nullptr);

    bool sendEmptyMessage(int what);
    bool sendMessage(const shared_ptr<Message> &msg, uint32_t dispatchFlag = NS_DISPATCH_SYNC);

private:
    void dispatchMessage(const shared_ptr<Message> &msg);

private:
    static const string TAG;

    nsCOMPtr<nsIRunnable> mMessageAsyncTask;

    nsresult mResult;
    nsCOMPtr<nsIThread> mThread;
};

END_VOWIFI_NAMESPACE

#endif
