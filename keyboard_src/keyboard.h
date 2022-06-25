#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "pico/stdlib.h"
#include "report.h"
#include "settings.h"
#include "tusb.h"
#include "usb_descriptors.h"
#include "message.h"

enum eProgrammingStatus {
    PROG_STATUS_OK = 0,
    PROG_STATUS_INVALID_KEY_COLUMN = 0x1,
    PROG_STATUS_INVALID_KEY_ROW = 0x2,
    PROG_STATUS_INVALID_MACRO_LENGTH = 0x4,
    PROG_STATUS_INVALID_PACKET_SEQ = 0x8,
    PROG_STATUS_PACKET_OVERFLOW = 0x10
};

struct ProgrammingKeyInfo {
    uint16_t KeyColumn;
    uint16_t KeyRow;
    uint16_t KeyCode;
    uint16_t MacroLength;
};

struct MacroKey {
    uint16_t Code;
    uint8_t IsModifier;
    uint8_t IsPressed;
    uint32_t DelayMs;
};

struct Key {
    bool IsPressed;
    bool IsLongPressed;
    uint64_t PressStart;
    uint32_t DebounceCounter;
    uint8_t Code;
    bool IsModifier;
    MacroKey* Macro;
    uint16_t MacroLength;

    Key() {
        Reset();
    }

    Key(uint8_t code, bool isModifier = false) {
        Reset();
        Code = code;
        IsModifier = isModifier;
    }

    void Reset() {
        IsPressed = false;
        IsLongPressed = false;
        PressStart = 0;
        DebounceCounter = 0;

        IsModifier = false;
        Macro = nullptr;
        MacroLength = 0;
    }
};

class Keyboard {
private:
    static const uint8_t NUM_COLS = 3;
    static const uint8_t NUM_ROWS = 3;

    static const uint8_t COL0_PIN = 28;
    static const uint8_t COL1_PIN = 27;
    static const uint8_t COL2_PIN = 26;

    static const uint8_t ROW0_PIN = 7;
    static const uint8_t ROW1_PIN = 6;
    static const uint8_t ROW2_PIN = 5;

    const uint32_t flashFirstKeySectorNum = 1;
    const uint8_t flashKeyConfigPageNum = 0;
    const uint32_t flashMagicNumber = 0xDDCCBBAA;

    struct KeysFlashConfig {
        uint32_t MagicNumber;
        uint16_t KeyCode;
        uint16_t MacroLength;
        uint32_t MacroBaseAddress;
    };

    enum KeyboardStates {
        KEYBOARD_STATE_SCAN,
        KEYBOARD_STATE_MACRO,
        KEYBOARD_STATE_PROGRAMMING
    };

public:
    static Keyboard& Instance() {
        static Keyboard instance;
        return instance;
    }

    void Initialize();
    void Main();

    void ProgrammingStarted();
    eProgrammingStatus GetReadyForProgrammingKey(const ProgrammingKeyInfo& info);
    eProgrammingStatus ProgramKeyPacket(uint8_t* data, uint16_t length, uint16_t seq);
    void ProgrammingEnded();
 
private:
    Keyboard();

    void LoadDefaultKeys();
    void LoadKeysFromFlash();
    KeysFlashConfig* GetKeyFlashConfig(int keyIndex);
  
    void Scan();
    void PlayMacro();
    void HidTask();
    bool SendReport();

    // Invoked when sent REPORT successfully to host
    // Application can use this to send the next report
    // Note: For composite reports, report[0] is report ID
    static void tud_hid_report_complete_cb(uint8_t instance, uint8_t const* report, uint8_t len);

    // Invoked when received GET_REPORT control request
    // Application must fill buffer report's content and return its length.
    // Return zero will cause the stack to STALL request
    static uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen);

    // Invoked when received SET_REPORT control request or
    // received data on OUT endpoint ( Report ID = 0, Type = 0 )
    static void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize);

    inline int GetFlashSectorNum(int keyIndex) { 
        return flashFirstKeySectorNum + keyIndex;
    }

private:
    uint8_t colPins[NUM_COLS];
    uint8_t rowPins[NUM_ROWS];
    Key keys[NUM_ROWS][NUM_COLS];
    uint64_t startTime;
    bool sendReport;
    Report report;
    Settings& settings;
    ProgrammingKeyInfo curProgKeyInfo;

    KeyboardStates currentState;
    int currentRow;
    int currentCol;
    int currentMacroKeyIndex;
    bool isInPostDelay;
    uint64_t macroPostDelay;
};

#endif // KEYBOARD_H    
