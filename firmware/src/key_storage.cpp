#include "key_storage.h"

#include "stm32f1xx_hal.h"

//typedef struct {
//    uint32_t system_id;
//    uint32_t key_id;
//    uint32_t door_id;
//    uint32_t key_type;
//    uint8_t[32] key;
//} PubKey_T;

// by the fault, the secret key is all zeroes.
uint32_t owner_system_id;
uint32_t owner_door_id;
PubKey_T keystore[MAX_KEY_SLOTS]; //keyslot[0] is always empty

__attribute__((section(".owner_storage")))
const uint32_t owner_system_id_flash = 0;
const uint32_t owner_door_id_flash = 0;
__attribute__((section(".key_storage")))
const PubKey_T keystore_flash[MAX_KEY_SLOTS] = {0}; //keyslot[0] is always empty


uint8_t get_key_index(uint32_t system_id, uint32_t key_id, uint32_t key_type) //TODO can be timed, is it a risk?
{
    uint8_t i = 0;

    if (key_type == 0)
    {
	return 0;
    }

    for (i=1; i<MAX_KEY_SLOTS; i++)
    {
	if (keystore[i].system_id == system_id)
	{
	    if (keystore[i].key_id == key_id)
	    {
		if (keystore[i].key_type == key_type)
		{
		    //match found
		    return i;
		}
	    }
	}

    }
    return 0;

}

uint8_t get_free_key_slot(void)
{

    uint8_t i = 0;

    for (i=1; i<MAX_KEY_SLOTS; i++)
    {
	if (keystore[i].key_type == 0)
	    return i;
    }
    //no more free slots
    return 0;

}

bool secret_key_write(uint8_t secret_key[32]) {
    HAL_FLASH_Unlock();

//    FLASH_EraseInitTypeDef erase_init;
//    erase_init.TypeErase = FLASH_TYPEERASE_PAGES;
//    erase_init.Banks = 0; /* only used for mass erase */
//    erase_init.PageAddress = reinterpret_cast<uint32_t>(&SECRET_KEY);
//    erase_init.NbPages = 1;

//    uint32_t page_error;
//    HAL_FLASHEx_Erase(&erase_init, &page_error);

/*    for (uint8_t i = 0; i < sizeof(SECRET_KEY)/2; i++) {
        HAL_FLASH_Program(
            FLASH_TYPEPROGRAM_HALFWORD,
            reinterpret_cast<uint32_t>(&SECRET_KEY) + i * 2,
            reinterpret_cast<uint16_t *>(secret_key)[i]
        );
    }
*/
    HAL_FLASH_Lock();

    return true;
}



