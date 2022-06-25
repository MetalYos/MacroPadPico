#include "flash_service.h"
#include "hardware/sync.h"

FlashService::FlashService() {
    for (int i = 0; i < FLASH_PAGE_SIZE; i++)
        localBuffer[0];
}

void FlashService::EraseSector(uint32_t sectorNum) {
    uint32_t absSectorNum = sectorNum + FLASH_BASE_SECTOR; 

    uint32_t intr = save_and_disable_interrupts();
    flash_range_erase(FLASH_SECTOR_SIZE * absSectorNum, FLASH_SECTOR_SIZE);
    restore_interrupts(intr);
}

void FlashService::WriteToSector(uint32_t sectorNum, uint8_t pageNum, uint8_t* data, int size) {
    uint32_t absSectorNum = sectorNum + FLASH_BASE_SECTOR; 

    // Make sure size does not overflow
    if (size > (NUM_PAGES_IN_SECTOR - pageNum) * FLASH_PAGE_SIZE)
        size = (NUM_PAGES_IN_SECTOR - pageNum) * FLASH_PAGE_SIZE;
    
    uint32_t intr = 0;
    uint32_t offset = absSectorNum * FLASH_SECTOR_SIZE + pageNum * FLASH_PAGE_SIZE;
    while (size > 0) {
        for (int i = 0; i < FLASH_PAGE_SIZE; i++) 
            localBuffer[i] = ((i >= size) ? 0 : data[i]);
        
        intr = save_and_disable_interrupts();
        flash_range_program(offset, localBuffer, FLASH_PAGE_SIZE);
        restore_interrupts(intr);
        
        offset += FLASH_PAGE_SIZE; // Go to next page in flash
        data += FLASH_PAGE_SIZE; // Go to next page in data
        size -= FLASH_PAGE_SIZE; // Remove the current page size that was written from total size
    }
}

uint8_t* FlashService::GetSectorAddress(uint32_t sectorNum) {
    uint32_t absSectorNum = sectorNum + FLASH_BASE_SECTOR;
    return (uint8_t*)(FLASH_BASE_ADDRESS + (absSectorNum * FLASH_SECTOR_SIZE));
}

uint8_t* FlashService::GetPageAddress(uint32_t sectorNum, uint8_t pageNum) {
    if (pageNum >= NUM_PAGES_IN_SECTOR)
        return nullptr;
    
    uint8_t* sectorAddress = GetSectorAddress(sectorNum);
    return (sectorAddress + (pageNum * FLASH_PAGE_SIZE));
}

