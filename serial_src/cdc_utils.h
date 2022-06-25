#ifndef CDC_UTILS_H
#define CDC_UTILS_H

#include "pico/stdlib.h"
#include "tusb.h"

static void SendBuffer(unsigned char* buffer, uint32_t length) {
    bool firstPrinted = false;
    for (uint8_t itf = 0; itf < CFG_TUD_CDC; itf++) {
        if (tud_cdc_n_connected(itf)) {
            if (!firstPrinted || tud_cdc_n_available(itf)) {
                /*
                for (uint32_t i = 0; i < length; i++)
                    tud_cdc_n_write_char(itf, buffer[i]);
                tud_cdc_n_write_flush(itf);
                
                firstPrinted = true;
                */

                tud_cdc_n_write(itf, buffer, length);
                tud_cdc_n_write_flush(itf);
                firstPrinted = true;
            }
        }
    }
}

static void Print(char* buffer, uint32_t length) {
    bool firstPrinted = false;
    for (uint8_t itf = 0; itf < CFG_TUD_CDC; itf++) {
        if (tud_cdc_n_connected(itf)) {
            if (!firstPrinted || tud_cdc_n_available(itf)) {
                for (uint32_t i = 0; i < length; i++)
                    tud_cdc_n_write_char(itf, buffer[i]);
                tud_cdc_n_write_flush(itf);
                
                firstPrinted = true;
            }
        }
    }
}

static void Print(const char* buffer) {
    bool firstPrinted = false;
    for (uint8_t itf = 0; itf < CFG_TUD_CDC; itf++) {
        if (tud_cdc_n_connected(itf)) {
            if (!firstPrinted || tud_cdc_n_available(itf)) {
                for (uint32_t i = 0; i < strlen(buffer); i++)
                    tud_cdc_n_write_char(itf, buffer[i]);
                tud_cdc_n_write_flush(itf);
                
                firstPrinted = true;
            }
        }
    }
}


#endif // CDC_UTILS_H
