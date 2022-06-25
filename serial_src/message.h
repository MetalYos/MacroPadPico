#ifndef MESSAGE_H
#define MESSAGE_H

const int MAX_DATA_LENGTH = 256;
const unsigned char MESSAGE_START_MARK = 1;

enum MessageTypes {
    MESSAGE_TYPE_ANSWER = 0x41,
    MESSAGE_TYPE_REQUEST = 0x52
};

enum MessageIds {
    MESSAGE_ID_SET_BLINK_ON_TIME = 0,
    MESSAGE_ID_SET_BLINK_OFF_TIME,
    MESSAGE_ID_GET_FLASH_PAGE,
    MESSAGE_ID_PROGRAMMING_START,
    MESSAGE_ID_PROGRAMMING_KEY_INFO,
    MESSAGE_ID_PROGRAMMING_KEY_PACKET,
    MESSAGE_ID_PROGRAMMING_END,
    MESSAGE_ID_TOTAL
};

struct MessageHeader {
    unsigned char Mark;
    unsigned char Type;
    unsigned short Seq;
    unsigned short Len;
    unsigned char Id;
    unsigned char Status;

    MessageHeader() :
        Mark(MESSAGE_START_MARK),
        Type(MESSAGE_TYPE_ANSWER),
        Seq(1),
        Len(0),
        Id(MESSAGE_ID_TOTAL),
        Status(0)
    {}

    MessageHeader(unsigned char id) :
        Mark(MESSAGE_START_MARK),
        Type(MESSAGE_TYPE_ANSWER),
        Seq(1),
        Len(0),
        Id(id),
        Status(0)
    {}

};

struct Message {
    MessageHeader Header;
    unsigned char Data[MAX_DATA_LENGTH];
};

#endif // MESSAGE_H
