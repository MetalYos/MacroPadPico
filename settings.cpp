#include "settings.h"
#include "flash_service.h"

const uint32_t Settings::defaults[(uint32_t)SettingsIds::SETTINGS_TOTAL] = {
    500,        // Blink On Time
    500,        // Blink Off Time
    10,         // Debounce Max
    500000,     // Auto repeat first delay in usec
    10,         // Auto repeat speed - presses per second
    166667,     // Auto repeat delay in usec
};

Settings::Settings() {
    LoadDefaults();
}

uint32_t Settings::operator() (SettingsIds id) {
    return settings[(uint32_t)id];
}

void Settings::operator() (SettingsIds id, uint32_t val) {
    settings[(uint32_t)id] = val;
}

void Settings::Load() {
    uint32_t* addr = (uint32_t*)FlashService::Instance().GetPageAddress(flashSectorNum, flashMagicNumPageNum);
    if (magicNumber != addr[0]) {
        Save();
    }
    else {
        addr = (uint32_t*)FlashService::Instance().GetPageAddress(flashSectorNum, flashSettingsPageNum);
        for (int i = 0; i < (int)SETTINGS_TOTAL; i++)
            settings[i] = addr[i];
    }
}

void Settings::Save() {
    EraseSettingsSector();
    SaveMagicNumber();
    SaveSettings();
}

void Settings::LoadDefaults() {
    for (int i = 0; i < (int)SETTINGS_TOTAL; i++)
        settings[i] = defaults[i];
}

void Settings::EraseSettingsSector() {
    // Erase Settings sector
    FlashService::Instance().EraseSector(flashSectorNum);
}

void Settings::SaveMagicNumber() {
    // Write Magic number as this is the first save to flash
    FlashService::Instance().WriteToSector(flashSectorNum, 
            flashMagicNumPageNum, (uint8_t*)&magicNumber, sizeof(magicNumber));
}

void Settings::SaveSettings() {
    // Write settings to flash
    FlashService::Instance().WriteToSector(flashSectorNum, flashSettingsPageNum, 
            (uint8_t*)settings, (int)SETTINGS_TOTAL * sizeof(int));
}

