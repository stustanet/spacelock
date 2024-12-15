#pragma once

#include <cstdint>

#define MAX_KEY_SLOTS 20

typedef struct {
    uint32_t system_id;
    uint32_t key_id;
    uint32_t door_id;
    uint32_t key_type;
    uint8_t key[32];
} PubKey_T;

extern uint32_t owner_system_id;
extern uint32_t owner_door_id;
extern PubKey_T keystore[MAX_KEY_SLOTS];
extern uint8_t used_store;

void init_keystore(void);
void load_keystore(void);
uint8_t verify_keystore(void);

uint8_t get_key_index(uint32_t system_id, uint32_t key_id, uint32_t key_type);
uint8_t get_free_key_slot(void);
uint8_t remove_system_except_one_key(uint32_t system_id, uint32_t key_id_retain);


bool key_store_write(void);
bool key_store0_write(void);
bool key_store1_write(void);
bool owner_write(void);



