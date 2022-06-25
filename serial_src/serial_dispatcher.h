#ifndef SERIAL_DISPATCHER_H
#define SERIAL_DISPATCHER_H

#include "message.h"

typedef void (*MessageCallback)(const Message&);

const int MAX_CALLBACKS_PER_ID = 3;

class SerialDispatcher {
private:
    struct MessageIdCallbacks {
        MessageCallback callbacks[MAX_CALLBACKS_PER_ID];
        int numRegistered;

        MessageIdCallbacks() {
            for (int i = 0; i < MAX_CALLBACKS_PER_ID; i++)
                callbacks[i] = &CallbackDummy;
            numRegistered = 0;
        }

        void ExecuteCallbacks(const Message& msg) {
            for (int i = 0; i < numRegistered; i++)
                (*callbacks[i])(msg);
        }
    };
public:
    static SerialDispatcher& Instance() {
        static SerialDispatcher instance;
        return instance;
    }

    void Initialize();
    void RegisterForMessage(MessageIds id, MessageCallback callback);
    bool ListenForMessage();
    void SendMessage(const MessageHeader& header, unsigned char* data);
    void SendMessage(const Message& msg);
    Message& GetMessage();

private:
    static void CallbackDummy(const Message& msg) {
        (void)msg;
    }

private:
    Message msg;
    MessageIdCallbacks idsCallbacks[MESSAGE_ID_TOTAL];
};

#endif // SERIAL_DISPATCHER_H
