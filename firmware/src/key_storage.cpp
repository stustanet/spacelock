#include "key_storage.h"

#include "stm32f1xx_hal.h"
#include "main.h"

#include <cstring>

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
uint32_t keystore_revision;
uint32_t keystore_crc;
uint8_t used_store = 0; //this flash block was used



__attribute__((section(".owner_storage")))
const volatile uint32_t owner_system_id_flash = 0;
__attribute__((section(".owner_storage")))
const volatile uint32_t owner_door_id_flash = 0;



__attribute__((section(".key_storage0")))
const PubKey_T keystore_flash0[MAX_KEY_SLOTS] = {0}; //keyslot[0] is always empty
__attribute__((section(".key_storage0")))
const volatile uint32_t keystore_flash0_crc = 0;
__attribute__((section(".key_storage0")))
const volatile uint32_t keystore_flash0_revision = 0;



__attribute__((section(".key_storage1")))
const PubKey_T keystore_flash1[MAX_KEY_SLOTS] = {0}; //keyslot[0] is always empty
__attribute__((section(".key_storage1")))
const volatile uint32_t keystore_flash1_crc = 0;
__attribute__((section(".key_storage1")))
const volatile uint32_t keystore_flash1_revision = 0;




//load data from flash to ram at startup
void init_keystore(void)
{

    owner_system_id = owner_system_id_flash;
    owner_door_id = owner_door_id_flash;

    //check if unowned
    if ((owner_system_id == 0) && (owner_door_id == 0))
    {
	used_store = 1; //write to flash_store0
	return; //no need to load keystore, system is new
    }

    load_keystore();

    //check
    verify_keystore();

    return;
}



void load_keystore()
{
    //check both keystores

    volatile uint32_t store0_crc = HAL_CRC_Calculate(&hcrc1, (uint32_t*)keystore_flash0, sizeof(keystore_flash0)/4);
    store0_crc = HAL_CRC_Accumulate(&hcrc1, &keystore_flash0_revision, sizeof(keystore_flash0_revision)/4);

    volatile uint32_t store1_crc = HAL_CRC_Calculate(&hcrc1, (uint32_t*)keystore_flash1, sizeof(keystore_flash1)/4);
    store1_crc = HAL_CRC_Accumulate(&hcrc1, &keystore_flash1_revision, sizeof(keystore_flash1_revision)/4);

    if (((keystore_flash0_revision >= keystore_flash1_revision)||(store1_crc != keystore_flash1_crc)) && (store0_crc == keystore_flash0_crc)) //>= to catch case both are 0
    {
	memcpy(keystore, keystore_flash0, sizeof(keystore)); //flash0 is newer
	keystore_crc = keystore_flash0_crc;
	keystore_revision = keystore_flash0_revision;
	used_store = 0;
    }
    else if (store1_crc == keystore_flash1_crc)
    {
	memcpy(keystore, keystore_flash1, sizeof(keystore)); //flash1 is newer
	keystore_crc = keystore_flash1_crc;
	keystore_revision = keystore_flash1_revision;
	used_store = 1;
    }
    else
    {
	//oops
	used_store = 42;
    }

}



void verify_keystore(void)
{
    //check crc of store in RAM and load new from flash corrupted

    volatile uint32_t store_ram_crc = HAL_CRC_Calculate(&hcrc1, (uint32_t*)keystore, sizeof(keystore)/4);
    store_ram_crc = HAL_CRC_Accumulate(&hcrc1, &keystore_revision, sizeof(keystore_revision)/4);

    if (used_store == 0)
    {
	if (store_ram_crc != keystore_flash0_crc)
	{
	    load_keystore();
	}

    }
    else if (used_store == 1)
    {
	if (store_ram_crc != keystore_flash1_crc)
	{
	    load_keystore();
	}
    }
    else
    {
	//no keystore was loaded, pointless to check
    }

    return;
}



//find they key in the keystore and return the index
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


//find a free keyslot
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

//guess what this does...
uint8_t remove_system_except_one_key(uint32_t system_id, uint32_t key_id_retain)
{

    uint8_t index_to_retain = get_key_index(system_id, key_id_retain, '1');
    if (index_to_retain == 0)
    {
	//abort, no key to retain
	return 0;
    }

    uint8_t i = 0;
    uint8_t keys_removed = 1;
    
    for (i=1;i<MAX_KEY_SLOTS;i++)
    {
	if (i == index_to_retain)
	{
	    //don't touch this index
	    continue;
	}
	if (keystore[i].system_id == system_id)
	{
	    //other key from this system found
	    //-> remove
	    keystore[i].system_id = 0;
	    keystore[i].key_id = 0;
	    keystore[i].key_type = 0;
	    keystore[i].door_id = 0;
	    keys_removed++;

	}

    }
    return keys_removed;

}

//write keytore, switch between 0 and 1
bool key_store_write(void) {

    keystore_revision++;
    volatile uint32_t keystore_crc_dummy = HAL_CRC_Calculate(&hcrc1, (uint32_t*)keystore, sizeof(keystore)/4);
    keystore_crc = HAL_CRC_Accumulate(&hcrc1, &keystore_revision, sizeof(keystore_revision)/4);

    if (used_store == 0)
    {
    	key_store1_write();
	used_store = 1;
    }
    else if (used_store == 1)
    {
    	key_store0_write();
	used_store = 0;
    } 
    else
    {
	return 0;
    }

    return 1;

}

// write keystore to flash
// 1. clear page (1kByte)
// 2. write keystore
bool key_store0_write(void) {
    HAL_FLASH_Unlock();

    FLASH_EraseInitTypeDef erase_init;
    erase_init.TypeErase = FLASH_TYPEERASE_PAGES;
    erase_init.Banks = 0; /* only used for mass erase */
    erase_init.PageAddress = reinterpret_cast<uint32_t>(&keystore_flash0);
    erase_init.NbPages = 1;

    uint32_t page_error;
    HAL_FLASHEx_Erase(&erase_init, &page_error);

    for (uint16_t i = 0; i < sizeof(keystore)/2; i++) {
        HAL_FLASH_Program(
            FLASH_TYPEPROGRAM_HALFWORD,
            reinterpret_cast<uint32_t>(&keystore_flash0) + i * 2,
            reinterpret_cast<uint16_t *>(keystore)[i]
        );
    }

    //write crc
    for (uint8_t i = 0; i < sizeof(keystore_crc)/2; i++) {
        HAL_FLASH_Program(
            FLASH_TYPEPROGRAM_HALFWORD,
            reinterpret_cast<uint32_t>(&keystore_flash0_crc) + i * 2,
            reinterpret_cast<uint16_t *>(&keystore_crc)[i]
        );
    }


    //write revision
    for (uint8_t i = 0; i < sizeof(keystore_revision)/2; i++) {
        HAL_FLASH_Program(
            FLASH_TYPEPROGRAM_HALFWORD,
            reinterpret_cast<uint32_t>(&keystore_flash0_revision) + i * 2,
            reinterpret_cast<uint16_t *>(&keystore_revision)[i]
        );
    }

    HAL_FLASH_Lock();

    return true;
}

bool key_store1_write(void) {
    HAL_FLASH_Unlock();

    FLASH_EraseInitTypeDef erase_init;
    erase_init.TypeErase = FLASH_TYPEERASE_PAGES;
    erase_init.Banks = 0; /* only used for mass erase */
    erase_init.PageAddress = reinterpret_cast<uint32_t>(&keystore_flash1);
    erase_init.NbPages = 1;

    uint32_t page_error;
    HAL_FLASHEx_Erase(&erase_init, &page_error);

    for (uint16_t i = 0; i < sizeof(keystore)/2; i++) {
        HAL_FLASH_Program(
            FLASH_TYPEPROGRAM_HALFWORD,
            reinterpret_cast<uint32_t>(&keystore_flash1) + i * 2,
            reinterpret_cast<uint16_t *>(keystore)[i]
        );
    }

    //write crc
    for (uint8_t i = 0; i < sizeof(keystore_crc)/2; i++) {
        HAL_FLASH_Program(
            FLASH_TYPEPROGRAM_HALFWORD,
            reinterpret_cast<uint32_t>(&keystore_flash1_crc) + i * 2,
            reinterpret_cast<uint16_t *>(&keystore_crc)[i]
        );
    }
    //
    //write revision
    for (uint8_t i = 0; i < sizeof(keystore_revision)/2; i++) {
        HAL_FLASH_Program(
            FLASH_TYPEPROGRAM_HALFWORD,
            reinterpret_cast<uint32_t>(&keystore_flash1_revision) + i * 2,
            reinterpret_cast<uint16_t *>(&keystore_revision)[i]
        );
    }

    HAL_FLASH_Lock();

    return true;
}

// write owner
// no delete because this is only done once

bool owner_write(void) {

    HAL_FLASH_Unlock();

    HAL_StatusTypeDef status;

    FLASH_EraseInitTypeDef erase_init;
    erase_init.TypeErase = FLASH_TYPEERASE_PAGES;
    erase_init.Banks = 0; /* only used for mass erase */
    erase_init.PageAddress = reinterpret_cast<uint32_t>(&owner_door_id_flash);
    erase_init.NbPages = 1;

    uint32_t page_error;
    HAL_FLASHEx_Erase(&erase_init, &page_error);

    //write owner info
    for (uint8_t i = 0; i < sizeof(owner_door_id)/2; i++) {
        HAL_FLASH_Program(
            FLASH_TYPEPROGRAM_HALFWORD,
            reinterpret_cast<uint32_t>(&owner_door_id_flash) + i * 2,
            reinterpret_cast<uint16_t *>(&owner_door_id)[i]
        );
    }
    for (uint8_t i = 0; i < sizeof(owner_system_id)/2; i++) {
        HAL_FLASH_Program(
            FLASH_TYPEPROGRAM_HALFWORD,
            reinterpret_cast<uint32_t>(&owner_system_id_flash) + i * 2,
            reinterpret_cast<uint16_t *>(&owner_system_id)[i]
        );
    }
/*
    status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, (uint32_t)&owner_system_id_flash, owner_system_id);
    if (status != HAL_OK)
    {
	HAL_FLASH_Lock();
	return false;
    }

    status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, (uint32_t)&owner_door_id_flash, owner_door_id);
*/
    HAL_FLASH_Lock();
//    if (status != HAL_OK)
//    {
//	return false;
//    }

    return true;
}

