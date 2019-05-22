#include "secret_key.h"

#include "stm32f1xx_hal.h"

// by the fault, the secret key is all zeroes.
__attribute__((section(".key_storage")))
const uint8_t SECRET_KEY[32] = {0};

bool secret_key_write(uint8_t secret_key[32]) {
    HAL_FLASH_Unlock();

    FLASH_EraseInitTypeDef erase_init;
    erase_init.TypeErase = FLASH_TYPEERASE_PAGES;
    erase_init.Banks = 0; /* only used for mass erase */
    erase_init.PageAddress = reinterpret_cast<uint32_t>(&SECRET_KEY);
    erase_init.NbPages = 1;

    uint32_t page_error;
    volatile auto a = HAL_FLASHEx_Erase(&erase_init, &page_error);

    for (uint8_t i = 0; i < sizeof(SECRET_KEY)/2; i++) {
        volatile auto b = HAL_FLASH_Program(
            FLASH_TYPEPROGRAM_HALFWORD,
            reinterpret_cast<uint32_t>(&SECRET_KEY),
            reinterpret_cast<uint16_t *>(secret_key)[i]
        );
    }

    HAL_FLASH_Lock();

    return true;
}
