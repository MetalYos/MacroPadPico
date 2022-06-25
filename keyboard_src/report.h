#ifndef REPORT_H
#define REPORT_H

#include "pico/stdlib.h"

class Report {
private:
    static const uint8_t MAX_KEYS_IN_REPORT = 6;

public:
    Report();

    void Reset();
    void Add(bool isModifier, uint8_t keycode);
    void Remove(bool isModifier, uint8_t keycode);
    uint8_t GetModifiers() { return modifiers; }
    uint8_t* GetKeycodes() { return keycodes; }

private:
    uint8_t modifiers;
    uint8_t keycodes[MAX_KEYS_IN_REPORT];
    uint8_t numKeycodes;
};

#endif // REPORT_H

