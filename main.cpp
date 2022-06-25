#include "pico/stdlib.h"
#include "serial_src/message.h"
#include "serial_src/serial_dispatcher.h"
#include "tusb.h"
#include "usb_descriptors.h"
#include "settings.h"
#include "message.h"
#include "serial_dispatcher.h"
#include "keyboard.h"
#include "flash_service.h"

Settings& settings = Settings::Instance();

const uint LED_PIN = PICO_DEFAULT_LED_PIN;
static uint64_t startTime = 0;

static bool isInProgrammingMode = false;
static Message answerMessage;

void InitGPIOs() {
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
}

//--------------------------------------------------------------------+                                                                                                               
// Device callbacks                                                                                                                                                                   
//--------------------------------------------------------------------+                                                                                                               
                                                                                                                                                                                      
// Invoked when device is mounted                                                                                                                                                     
void tud_mount_cb(void) {
}

// Invoked when device is unmounted                                                                                                                                                   
void tud_umount_cb(void) {
}                                                                                                                                                                                      

// Invoked when usb bus is suspended                                                                                                                                                  
// remote_wakeup_en : if host allow us to perform remote wakeup                                                                                                                      
// Within 7ms, device must draw an average of current less than 2.5 mA from bus                                                                                                       
void tud_suspend_cb(bool remote_wakeup_en) {
    (void) remote_wakeup_en;
}

// Invoked when usb bus is resumed
void tud_resume_cb(void) {
}


//--------------------------------------------------------------------+
// Messages Callbacks                                                  
//--------------------------------------------------------------------+
void SetBlinkOnTimeMessageCallback(const Message& msg) {
    settings(SettingsIds::BLINK_ON_TIME, *(uint32_t*)msg.Data);
    settings.Save();

    // Send answer back
    answerMessage.Header.Seq = 1;
    answerMessage.Header.Len = 0;
    answerMessage.Header.Id = MESSAGE_ID_SET_BLINK_ON_TIME;
    answerMessage.Header.Status = 0;
    SerialDispatcher::Instance().SendMessage(answerMessage);
}

void SetBlinkOffTimeMessageCallback(const Message& msg) {
    settings(SettingsIds::BLINK_OFF_TIME, *(uint32_t*)msg.Data);
    settings.Save();

    // Send answer back
    answerMessage.Header.Seq = 1;
    answerMessage.Header.Len = 0;
    answerMessage.Header.Id = MESSAGE_ID_SET_BLINK_OFF_TIME;
    answerMessage.Header.Status = 0;
    SerialDispatcher::Instance().SendMessage(answerMessage);
}

void GetFlashPageMessageCallback(const Message& msg) {
    uint8_t* addr = FlashService::Instance().GetPageAddress(
            ((uint32_t*)msg.Data)[0], ((uint32_t*)msg.Data)[1]);

    // Send answer back
    answerMessage.Header.Seq = 1;
    answerMessage.Header.Len = FLASH_PAGE_SIZE;
    answerMessage.Header.Id = MESSAGE_ID_GET_FLASH_PAGE;
    answerMessage.Header.Status = 0;
    for (uint32_t i = 0; i < answerMessage.Header.Len; i++) {
        answerMessage.Data[i] = addr[i];
    }
    SerialDispatcher::Instance().SendMessage(answerMessage);
}

void ProgrammingStartCallback(const Message& msg) {
    isInProgrammingMode = true;
    Keyboard::Instance().ProgrammingStarted();
    
    // Send answer back
    answerMessage.Header.Seq = 1;
    answerMessage.Header.Len = 0;
    answerMessage.Header.Id = MESSAGE_ID_PROGRAMMING_START;
    answerMessage.Header.Status = 0;
    SerialDispatcher::Instance().SendMessage(answerMessage);
}

void ProgrammingEndCallback(const Message& msg) {
    isInProgrammingMode = false;
    Keyboard::Instance().ProgrammingEnded();

    // Send answer back
    answerMessage.Header.Seq = 1;
    answerMessage.Header.Len = 0;
    answerMessage.Header.Id = MESSAGE_ID_PROGRAMMING_END;
    answerMessage.Header.Status = 0;
    SerialDispatcher::Instance().SendMessage(answerMessage);
}

void ProgrammingKeyInfoCallback(const Message& msg) {
    ProgrammingKeyInfo* keyInfoInput = (ProgrammingKeyInfo*)msg.Data;
    eProgrammingStatus status = Keyboard::Instance().GetReadyForProgrammingKey(*keyInfoInput);

    // Send answer back
    answerMessage.Header.Seq = 1;
    answerMessage.Header.Len = 0;
    answerMessage.Header.Id = MESSAGE_ID_PROGRAMMING_KEY_INFO;
    answerMessage.Header.Status = status;
    SerialDispatcher::Instance().SendMessage(answerMessage);

    if (status != PROG_STATUS_OK)
        isInProgrammingMode = false;
}

void ProgrammingKeyPacketCallback(const Message& msg) {
    eProgrammingStatus status = Keyboard::Instance().
        ProgramKeyPacket((uint8_t*)msg.Data, msg.Header.Len, msg.Header.Seq);

    // Send answer back
    answerMessage.Header.Seq = msg.Header.Seq;
    answerMessage.Header.Len = 0;
    answerMessage.Header.Id = MESSAGE_ID_PROGRAMMING_KEY_PACKET;
    answerMessage.Header.Status = status;
    SerialDispatcher::Instance().SendMessage(answerMessage);

    if (status != PROG_STATUS_OK)
        isInProgrammingMode = false;
}

//--------------------------------------------------------------------+
// Blink Task                                                  
//--------------------------------------------------------------------+
void BlinkTask(bool isFast = false) {
    uint32_t onTime = settings(SettingsIds::BLINK_ON_TIME);
    uint32_t offTime = settings(SettingsIds::BLINK_OFF_TIME);

    if (isFast) {
        onTime /= 10;
        offTime /= 10;
    }

    bool isOn = gpio_get(LED_PIN);

    if(isOn) {
        if ((time_us_64() - startTime) / 1000 > onTime) {
            gpio_put(LED_PIN, 0);
            startTime = time_us_64();
        }
    }
    else {
        if ((time_us_64() - startTime) / 1000 > offTime) {
            gpio_put(LED_PIN, 1);
            startTime = time_us_64();
        }
    }
}

//--------------------------------------------------------------------+
// Main Loop                                                  
//--------------------------------------------------------------------+
int main() {
    settings.Load(); // Load settings from flash

    InitGPIOs();
    tusb_init();

    SerialDispatcher::Instance().Initialize();
    Keyboard::Instance().Initialize();

    // Register callbacks
    SerialDispatcher::Instance().RegisterForMessage(MESSAGE_ID_SET_BLINK_ON_TIME, 
            SetBlinkOnTimeMessageCallback);
    SerialDispatcher::Instance().RegisterForMessage(MESSAGE_ID_SET_BLINK_OFF_TIME, 
            SetBlinkOffTimeMessageCallback);
    SerialDispatcher::Instance().RegisterForMessage(MESSAGE_ID_GET_FLASH_PAGE, 
            GetFlashPageMessageCallback);
    SerialDispatcher::Instance().RegisterForMessage(MESSAGE_ID_PROGRAMMING_START,
            ProgrammingStartCallback);
    SerialDispatcher::Instance().RegisterForMessage(MESSAGE_ID_PROGRAMMING_KEY_INFO,
            ProgrammingKeyInfoCallback);
    SerialDispatcher::Instance().RegisterForMessage(MESSAGE_ID_PROGRAMMING_KEY_PACKET,
            ProgrammingKeyPacketCallback);
    SerialDispatcher::Instance().RegisterForMessage(MESSAGE_ID_PROGRAMMING_END,
            ProgrammingEndCallback);

    while (true) {
        tud_task();
        //CdcTask();
        SerialDispatcher::Instance().ListenForMessage();

        if (isInProgrammingMode) {
            BlinkTask(true); 
        }
        else {
            Keyboard::Instance().Main();
            BlinkTask();
        }
    }
}

