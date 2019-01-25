#ifndef __SPRD_VOWIFI_PORTING_MESSAGE_ASYNC_TASK_H__
#define __SPRD_VOWIFI_PORTING_MESSAGE_ASYNC_TASK_H__

#include <string>

using std::string;
using std::shared_ptr;


BEGIN_VOWIFI_NAMESPACE

class MessageAsyncTask : public nsRunnable {
public:
    NS_DECL_NSIRUNNABLE

    explicit MessageAsyncTask();
    virtual ~MessageAsyncTask();

    void updateMessage(const shared_ptr<Message> &message);

private:
    static const string TAG;

    shared_ptr<Message> mMessage;
};

END_VOWIFI_NAMESPACE

#endif
