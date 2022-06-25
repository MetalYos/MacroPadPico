#include "report.h"

Report::Report() {
    Reset();
}

void Report::Reset() {
    modifiers = 0;
    numKeycodes = 0;
    for (auto& code : keycodes)
        code = 0;
}

void Report::Add(bool isModifier, uint8_t keycode) {
    // If there is no room to add keycode, change all to -1 and return
    if (numKeycodes >= MAX_KEYS_IN_REPORT) {
        for (auto& code : keycodes)
            code = 0xFF;
        return;
    }

    if (isModifier) {
        // If it is a modifier, add it
        modifiers |= keycode;
    }
    else {
        // If the keycode already exists, do nothing
        for (auto& code : keycodes) {
            if (code == keycode)
                return;
        }

        // Otherwise, add it to the first available spot
        for (auto& code : keycodes) {
            if (code == 0) {
                code = keycode;
                break;
            }
        }
    }
}

void Report::Remove(bool isModifier, uint8_t keycode) {
    if (isModifier) {
        // If it is a modifier, remove it
        modifiers &= ~keycode;
    }
    else {
        // Otherwise, if it exists in the keycodes, remove it
        for (auto& code : keycodes) {
            if (code == keycode)
                code = 0;
        }
    }
}


