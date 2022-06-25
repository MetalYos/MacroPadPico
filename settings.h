#ifndef SETTINGS_H
#define SETTINGS_H

#include "pico/stdlib.h"

enum SettingsIds {
    BLINK_ON_TIME = 0,
    BLINK_OFF_TIME,
    DEBOUNCE_MAX,
    AUTO_REPEAT_FIRST_DELAY,
    AUTO_REPEAT_SPEED,
    AUTO_REPEAT_DELAY,
    SETTINGS_TOTAL
};

class Settings {
private:
    static const uint32_t defaults[(uint32_t)SettingsIds::SETTINGS_TOTAL];
    const uint32_t flashSectorNum = 0;
    const uint8_t flashMagicNumPageNum = 0;
    const uint32_t magicNumber = 0xABCD1234;
    const uint8_t flashSettingsPageNum = 1;

public:
    static Settings& Instance() {
        static Settings instance;
        return instance;
    }

    uint32_t operator() (SettingsIds id);
    void operator() (SettingsIds id, uint32_t val);
    void Load();
    void Save();

private:
    Settings();
    void LoadDefaults();
    void EraseSettingsSector();
    void SaveMagicNumber();
    void SaveSettings();

private:
    uint32_t settings[(uint32_t)SettingsIds::SETTINGS_TOTAL];
    bool isFirstSave;
};

#endif // SETTINGS_H
