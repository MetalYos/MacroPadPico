#include "serial_dispatcher.h"
#include "cdc_utils.h"
#include "message.h"

void SerialDispatcher::Initialize() {

}

void SerialDispatcher::RegisterForMessage(MessageIds id, MessageCallback callback) {
    if (id >= MESSAGE_ID_TOTAL)
        return;
    
    int& numRegistered = idsCallbacks[id].numRegistered;
    if (numRegistered < MAX_CALLBACKS_PER_ID)
        idsCallbacks[id].callbacks[numRegistered++] = callback;
}

bool SerialDispatcher::ListenForMessage() {
    if (tud_cdc_available()) {
        // read datas
        uint32_t count = tud_cdc_read((char*)&msg, sizeof(msg));

        if (count < sizeof(MessageHeader))
            return false;
        if (count < sizeof(MessageHeader) + msg.Header.Len)
            return false;
        
        // Call all of the relevant callbacks
        if (msg.Header.Id >= MESSAGE_ID_TOTAL)
            return false;

        idsCallbacks[msg.Header.Id].ExecuteCallbacks(msg);  
        return true;
    }

    return false;
}

void SerialDispatcher::SendMessage(const MessageHeader& header, unsigned char* data) {
    msg.Header = header;
    if (data == nullptr)
        msg.Header.Len = 0;

    for (int i = 0; i < header.Len; i++) {
        msg.Data[i] = data[i];
    }

    SendBuffer((unsigned char*)&msg, sizeof(MessageHeader) + msg.Header.Len);
}

void SerialDispatcher::SendMessage(const Message& msgToSend) {
    msg.Header = msgToSend.Header;

    for (int i = 0; i < msgToSend.Header.Len; i++) {
        msg.Data[i] = msgToSend.Data[i];
    }

    SendBuffer((unsigned char*)&msg, sizeof(MessageHeader) + msg.Header.Len);
}

Message& SerialDispatcher::GetMessage() {
    return msg;
}
