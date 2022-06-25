#include <iostream>
#include "keyboard.h"
#include "keycodes.h"
#include "../flash_service.h"
#include "serial_dispatcher.h"

Keyboard::Keyboard() : settings(Settings::Instance()) {
    startTime = 0;
    sendReport = false;
    currentState = KEYBOARD_STATE_SCAN;
    currentRow = 0;
    currentCol = 0;
    currentMacroKeyIndex = 0;
    isInPostDelay = false;
    macroPostDelay = 0;
}

void Keyboard::Initialize() {
    // Initialize Pin constraints
    colPins[0] = COL0_PIN;
    colPins[1] = COL1_PIN;
    colPins[2] = COL2_PIN;
    rowPins[0] = ROW0_PIN;
    rowPins[1] = ROW1_PIN;
    rowPins[2] = ROW2_PIN;

    // Init Columns (ouputs)
    for (int i = 0; i < NUM_COLS; i++) {
        gpio_init(colPins[i]);
        gpio_set_dir(colPins[i], GPIO_OUT);
        gpio_set_pulls(colPins[i], true, false);
    }

    // Init Rows (inputs)
    for (int i = 0; i < NUM_ROWS; i++) {
        gpio_init(rowPins[i]);
        gpio_set_dir(rowPins[i], GPIO_IN);
        gpio_set_pulls(rowPins[i], true, false);
    }

    // Load default keys to make sure defaults are loaded 
    // in case flash was not programmed
    LoadDefaultKeys();
    
    // Load keys from flash (macros or defaults)
    LoadKeysFromFlash();
}

void Keyboard::LoadDefaultKeys() {
    keys[0][0] = Key(KEY_A);
    keys[0][1] = Key(KEY_D);
    keys[0][2] = Key(KEY_G);

    keys[1][0] = Key(KEY_B);
    keys[1][1] = Key(KEY_E);
    keys[1][2] = Key(KEY_H);
    
    keys[2][0] = Key(KEY_C);
    keys[2][1] = Key(KEY_F);
    keys[2][2] = Key(KEY_MOD_LSHIFT, true);
}

void Keyboard::LoadKeysFromFlash() {
    for (int i = 0; i < (NUM_ROWS * NUM_COLS); i++) {
        KeysFlashConfig* keyConfig = GetKeyFlashConfig(i);
        if (keyConfig->MagicNumber != flashMagicNumber)
            continue;

        int row = i / NUM_ROWS;
        int col = i % NUM_ROWS;

        keys[col][row].Reset();
        keys[col][row].Code = keyConfig->KeyCode;
        keys[col][row].Macro = reinterpret_cast<MacroKey*>(keyConfig->MacroBaseAddress);
        keys[col][row].MacroLength = keyConfig->MacroLength;
    }
}

Keyboard::KeysFlashConfig* Keyboard::GetKeyFlashConfig(int keyIndex) {
    if (keyIndex >= (NUM_COLS * NUM_ROWS))
        return nullptr;
    
    int sectorNum = GetFlashSectorNum(keyIndex);
    KeysFlashConfig* keyConfig = (KeysFlashConfig*)FlashService::Instance().
        GetSectorAddress(sectorNum);

    return keyConfig;

}

void Keyboard::ProgrammingStarted() {
    currentState = KEYBOARD_STATE_PROGRAMMING;
}

eProgrammingStatus Keyboard::GetReadyForProgrammingKey(const ProgrammingKeyInfo& keyInfo) {
    if (keyInfo.KeyColumn >= NUM_COLS)
        return PROG_STATUS_INVALID_KEY_COLUMN;
    if (keyInfo.KeyRow >= NUM_ROWS)
        return PROG_STATUS_INVALID_KEY_ROW;
    if (keyInfo.MacroLength > (FLASH_SECTOR_SIZE - FLASH_PAGE_SIZE))
        return PROG_STATUS_INVALID_MACRO_LENGTH;

    curProgKeyInfo = keyInfo;
    int keyIndex = keyInfo.KeyRow * NUM_COLS + keyInfo.KeyColumn;
    int sectorNum = GetFlashSectorNum(keyIndex);

    // Program config page
    KeysFlashConfig flashConfig;
    flashConfig.KeyCode = curProgKeyInfo.KeyCode;
    flashConfig.MacroLength = curProgKeyInfo.MacroLength;
    flashConfig.MacroBaseAddress = 0;
    if (curProgKeyInfo.MacroLength > 0) {
        flashConfig.MacroBaseAddress = (unsigned long)FlashService::Instance().
        GetPageAddress(sectorNum, flashKeyConfigPageNum + 1);
    }
    flashConfig.MagicNumber = flashMagicNumber;


    FlashService::Instance().EraseSector(sectorNum);
    FlashService::Instance().
        WriteToSector(sectorNum, flashKeyConfigPageNum,
                (uint8_t*)(&flashConfig), sizeof(KeysFlashConfig));
    
    return PROG_STATUS_OK;
}

eProgrammingStatus Keyboard::ProgramKeyPacket(uint8_t* data, uint16_t length, uint16_t seq) {
    if (seq >= FlashService::Instance().GetNumPagesPerSector())
        return PROG_STATUS_PACKET_OVERFLOW; 
    if (seq == 0)
        return PROG_STATUS_INVALID_PACKET_SEQ;

    int keyIndex = curProgKeyInfo.KeyRow * NUM_COLS + curProgKeyInfo.KeyColumn;
    int sectorNum = GetFlashSectorNum(keyIndex);
    int pageNum = seq;

    FlashService::Instance().WriteToSector(sectorNum, pageNum, data, length);

    return PROG_STATUS_OK;
}

void Keyboard::ProgrammingEnded() {
    LoadKeysFromFlash();
    currentState = KEYBOARD_STATE_SCAN;
}

void Keyboard::Main() {
    if (currentState == KEYBOARD_STATE_SCAN)
        Scan();
    else if  (currentState == KEYBOARD_STATE_MACRO)
        PlayMacro();

    HidTask();
}

void Keyboard::HidTask() {
    if (!sendReport)
        return;

    if (tud_suspended()) {
        // Wake up host if we are in suspend mode
        // and REMOTE_WAKEUP feature is enabled by host
        tud_remote_wakeup();
    }
    else {
        SendReport();
    }
}

bool Keyboard::SendReport() {
    // skip if hid is not ready yet
    if (!tud_hid_ready())
        return false;

    tud_hid_keyboard_report(REPORT_ID_KEYBOARD, report.GetModifiers(), report.GetKeycodes());
    sendReport = false;
    
    return true;
}

void Keyboard::Scan() {
    for (int col = 0; col < NUM_COLS; col++) {
        gpio_put(colPins[col], false);
        for (int row = 0; row < NUM_ROWS; row++) {
            Key& key = keys[row][col];
            if (!gpio_get(rowPins[row])) {
                // Key was pressed
                
                
                key.DebounceCounter += 1;
                if (key.IsLongPressed) {
                    if (time_us_64() - key.PressStart > settings(AUTO_REPEAT_DELAY)) {
                        key.PressStart = time_us_64();
                        std::cout << "(" << row << ", " << col << ") other auto delay" << std::endl;
                        report.Add(key.IsModifier, key.Code);
                        sendReport = true;
                    }
                }
                else if (key.IsPressed && !key.IsLongPressed) {
                    if (time_us_64() - key.PressStart > settings(AUTO_REPEAT_FIRST_DELAY)) {
                        key.IsLongPressed = true;
                        key.PressStart = time_us_64();
                        std::cout << "(" << row << ", " << col << ") first auto delay" << std::endl;
                        report.Add(key.IsModifier, key.Code);
                        sendReport = true;
                    }
                }
                else {
                    if (key.DebounceCounter > settings(DEBOUNCE_MAX)) {
                        // Check if it is a macro, if it is, change to macro state and break out
                        if (key.Macro != nullptr && key.MacroLength > 0) {
                            currentRow = row;
                            currentCol = col;
                            currentState = KEYBOARD_STATE_MACRO;
                            break;
                        }

                        key.DebounceCounter = 0;
                        key.IsPressed = true;
                        key.PressStart = time_us_64();
                        std::cout << "(" << row << ", " << col << ") is pressed" << std::endl;
                        report.Add(key.IsModifier, key.Code);
                        sendReport = true;
                    }
                }
            }
            else {
                if (key.IsPressed) {
                    // Key was released
                    key.Reset();
                    std::cout << "(" << row << ", " << col << ") was released" << std::endl;
                    report.Remove(key.IsModifier, key.Code);
                    sendReport = true;
                }
            }
        }
        gpio_put(colPins[col], true);

        // changed to macro state, no need to continue the scan
        if (currentState == KEYBOARD_STATE_MACRO)
            break;
    }
}

void Keyboard::PlayMacro() {
    Key& key = keys[currentCol][currentRow];
    MacroKey& mKey = key.Macro[currentMacroKeyIndex];

    if (isInPostDelay) {
        if ((time_us_64() - macroPostDelay) > (mKey.DelayMs * 1000)) {
            currentMacroKeyIndex++;
            isInPostDelay = false;
        }
    }
    else {
        if (mKey.IsPressed)
            report.Add((bool)mKey.IsModifier, (uint8_t)(mKey.Code & 0x00FF));
        else
            report.Remove((bool)mKey.IsModifier, (uint8_t)(mKey.Code & 0x00FF));
        sendReport = true;

        if (mKey.DelayMs > 0) {
            isInPostDelay = true;
            macroPostDelay = time_us_64();
        }
        else
            currentMacroKeyIndex++;

    }

    if (currentMacroKeyIndex == key.MacroLength) {
        currentMacroKeyIndex = 0;
        isInPostDelay = false;
        report.Reset();
        sendReport = true;
        currentState = KEYBOARD_STATE_SCAN;
    }
}   

void tud_hid_report_complete_cb(uint8_t instance, uint8_t const* report, uint8_t len) {
    // TODO: implement
    (void)instance;
    (void)report;
    (void)len;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
    // TODO: implement
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)reqlen;

    return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
    // TODO: implemet
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)bufsize;
}

