#include "Message.h"


BEGIN_VOWIFI_NAMESPACE

const string Message::TAG = "Message";

Message::Message()
    : what(MSG_DEFAULT_ID)
    , arg1(0)
    , arg2(0)
    , obj(nullptr)
    , target(nullptr) {
}

Message::~Message() {
}

void Message::sendToTarget() {
    shared_ptr<Message> m = obtain(this);
    target->sendMessage(m);
}

shared_ptr<Message> Message::obtain() {
    shared_ptr<Message> m(new Message);
    return m;
}

shared_ptr<Message> Message::obtain(Message *msg) {
    shared_ptr<Message> m(msg);
    return m;
}

shared_ptr<Message> Message::obtain(MessageHandler* handler, int what, void *obj) {
    shared_ptr<Message> m = obtain();
    m->what = what;
    m->obj = obj;
    m->target = handler;

    return m;
}

shared_ptr<Message> Message::obtain(MessageHandler* handler, int what, int arg1, int arg2, void *obj) {
    shared_ptr<Message> m = obtain();
    m->what = what;
    m->arg1 = arg1;
    m->arg2 = arg2;
    m->obj = obj;
    m->target = handler;

    return m;
}

END_VOWIFI_NAMESPACE