#ifndef FLASH_SERVICE_H
#define FLASH_SERVICE_H

#include "pico/stdlib.h"
#include "hardware/flash.h"
#include "cdc_utils.h"

class FlashService {
private:
    // FLASH_SECTOR_SIZE = 4096 bytes
    // FLASH_PAGE_SIZE = 256 bytes
    const uint32_t FLASH_BASE_SECTOR = 100; // Make sure it is above the application data
    const uint32_t FLASH_BASE_ADDRESS = XIP_BASE;
    const uint32_t FLASH_BASE_OFFSET = FLASH_BASE_SECTOR * FLASH_SECTOR_SIZE;
    const uint32_t FLASH_BASE_OFFSET_ADDRESS = FLASH_BASE_ADDRESS + FLASH_BASE_OFFSET;
    const uint32_t NUM_PAGES_IN_SECTOR = (FLASH_SECTOR_SIZE / FLASH_PAGE_SIZE);

public:
    static FlashService& Instance() {
        static FlashService instance;
        return instance;
    }

    void EraseSector(uint32_t sectorNum);
    void WriteToSector(uint32_t sectorNum, uint8_t pageNum, uint8_t* data, int size);
    uint8_t* GetSectorAddress(uint32_t sectorNum);
    uint8_t* GetPageAddress(uint32_t sectorNum, uint8_t pageNum);
    inline uint8_t GetNumPagesPerSector() const { return NUM_PAGES_IN_SECTOR; }

private:
    FlashService();

private:
    uint8_t localBuffer[FLASH_PAGE_SIZE];
};

#endif // FLASH_SERVICE_H
