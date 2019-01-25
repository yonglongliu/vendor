#ifndef __SPRD_VOWIFI_PORTING_MESSAGE_H__
#define __SPRD_VOWIFI_PORTING_MESSAGE_H__

#include "MessageHandler.h"
#include <string>

using std::string;
using std::shared_ptr;


BEGIN_VOWIFI_NAMESPACE
class MessageHandler;

class Message {
public:
    explicit Message();
    virtual ~Message();

    void sendToTarget();

    static shared_ptr<Message> obtain();
    static shared_ptr<Message> obtain(Message *msg);
    static shared_ptr<Message> obtain(MessageHandler* handler, int what, void *obj);
    static shared_ptr<Message> obtain(MessageHandler* handler, int what, int arg1 = 0, int arg2 = 0, void *obj = nullptr);

public:
    int what;
    int arg1;
    int arg2;
    void *obj;
    MessageHandler *target;

private:
    static const string TAG;
};

END_VOWIFI_NAMESPACE

#endif
