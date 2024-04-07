#include "stm32f1xx_hal.h"

#include "base64.h"
#include "config.h"
#include "dcf77.h"
#include "deserialize.h"
#include "hardware.h"
#include "hmac.h"
#include "motor.h"
#include "key_storage.h"
#include "sha256.h"
#include "time.h"
#include "monocypher-ed25519.h"
#include "utf8.h"

#include <cstring>


static void cpp_main_in_cpp();
static void open_door(StepperMotor &motor);

static void open_door(StepperMotor &motor)
{
    // uart_writeline("opening door \U0001f308");
    motor.set_mode(1);
    // TODO: find the correct number which does a full rotation
    uart_writeline("L");
    auto [end_stop_state, urevs_since_endstop] = motor.rotate(5000000, 1000000 * 4, 400000);
    if (end_stop_state == EndStopState::RISING_EDGE)
    {
        // keep moving for a few extra usteps
        uart_writeline("K");
        if (urevs_since_endstop < ENDSTOP_ROTATE_AFTER_EDGE_USTEPS)
        {
            motor.rotate(ENDSTOP_ROTATE_AFTER_EDGE_USTEPS - urevs_since_endstop, 1000000 * 2, -1);
        }
    }
    else
    {
        uart_writeline("N");
    }

    motor.set_mode(0);
     sleep_us(1000000);
    motor.set_mode(-1);
    // TODO: find the correct number which does a quarter or so backrotation
    uart_writeline("B");
    motor.rotate(1000000, 1000000 * 4, -1);
    uart_writeline("BD");
    motor.set_mode(0);
}

static bool check_info(const uint8_t *info, uint32_t info_size)
{
    if (info_size < 1)
    {
        return false;
    }

    while (info_size)
    {
        if (*info < 0x20 || *info >= 0x7f)
        {
            // illegal character in info string
            return false;
        }

        info++;
        info_size -= 1;
    }
    return true;
}

/**
 * This function is not used in the code.
 * It exists so that it can be comfortably called
 * from a gdb session in order to reset the key to a known state.
 */
__attribute__((used))
void reset_secret_key()
{
    //secret_key_write((unsigned char *)"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00");
}

static void cpp_main_in_cpp()
{
    StepperMotor motor(
        OutputPin(GPIOA, GPIO_PIN_5), // step
        OutputPin(GPIOA, GPIO_PIN_6), // sleep
        OutputPin(GPIOA, GPIO_PIN_4), // direction
        {
            OutputPin(GPIOB, GPIO_PIN_10), // microstep modesel 0
            OutputPin(GPIOB, GPIO_PIN_1),  // microstep modesel 1
            OutputPin(GPIOB, GPIO_PIN_0)   // microstep modesel 2
        },
        OutputPin(GPIOA, GPIO_PIN_7), // reset
        InputPin(GPIOA, GPIO_PIN_3)   // fault
    );

    // door state LED, always on because we always have a door.
    OutputPin led_doorstate(GPIOC, GPIO_PIN_15);
    led_doorstate.set();
    // TIME SIG is connected directly to the DCF77 signal,
    // it doesn't exist here
    // TIME STATE tells us whether a correct time has been received.
    OutputPin led_timestate(GPIOC, GPIO_PIN_14);

    InputPin dcf77_pin(GPIOA, GPIO_PIN_8);
    dcf77_init(&dcf77_pin, &led_timestate);

    init_keystore();

    uart_writeline("Spacelock initialized! \U0001F389");

    while (1)
    {
        UARTRxBuffer *message = uart_poll_message();
        if (message == nullptr)
        {
            // no new message is ready
            continue;
        }
        if (message->buf_pos == 0)
        {
            // the received message is empty
            continue;
        }

        // super-secret backdoor. don't tell anybody.
        if (
            (message->buf[0] == 'b') &&
            (message->buf[1] == 'a') &&
            (message->buf[2] == 'c') &&
            (message->buf[3] == 'k') &&
            (message->buf[4] == 'd') &&
            (message->buf[5] == 'o') &&
            (message->buf[6] == 'o') &&
            (message->buf[7] == 'r'))
        {
            // nothing to see here
#if WITH_BACKDOOR
            uart_writeline("you used the \x1b[32;1;5msuper-secret\x1b[m backdoor!");
#else
            uart_writeline("lol noob");
#endif
#if WITH_BACKDOOR
            open_door(motor);
#endif
            continue;
        }

        // base64-decode the message.
        uint32_t size = base64_decode(message->buf.data(), message->buf_pos);
        if (size == 0)
        {
            // the base64-decoded message is empty
            uart_writeline("base64-decoded message is empty");
            continue;
        }

	//parse message

        // all messages have the following format:
        //    uint8_t   signature[64]
        //    uint32_t  valid_from (unixtime/60)
        //    uint16_t  minutes_valid
        //    1x utf-8   system_id
        //    1x utf-8   key_id
        //    1x utf-8   msg_type
	//
        //    uint8_t   payload[]       (variable length)
	//
#define SYSTEM_ID_OFFSET SIG_SIZE+6

        if (size <= SIG_SIZE + 9)
        {
            // the message is too small
            uart_writeline("message is too small");
            continue;
        }


	//check signature


	//get the signature key
	uint16_t offset = SYSTEM_ID_OFFSET;
	
	uint32_t system_id = deserialize_utf8(message->buf.data(), &offset, 1);
	uint32_t key_id = deserialize_utf8(message->buf.data(), &offset, 1);

	if (!system_id | !key_id)
	{
	    uart_writeline("invalid system or key");
	    continue;
	}

	uint8_t current_key_index = get_key_index(system_id, key_id,'1'); //key type must be '1' for now, TODO

	if (current_key_index == 0)
	{
	    //we don't have that key in our store

	    //check if we are owned
	    if (owner_system_id != 0 || owner_door_id != 0)
	    {
		uart_writeline("unknown system or key");
		continue;
	    }

	    //special case, system is new and not yet owned, so we don't abort and don't check the signature
	    //check if this is an "init" type message
	    if (deserialize_utf8(message->buf.data(), &offset, 0) != 'I')
	    {
		//not an init type message, abort
		continue;
	    }
	}
	else
	{
	    //check ed25519 signature
	    if (crypto_ed25519_check(message->buf.data(), keystore[current_key_index].key, message->buf.data() + SIG_SIZE, size-SIG_SIZE)) {
	    // Message is corrupted, do not trust it
		uart_writeline("signature check fail");
		continue;
	    } 
	    //else Message is genuine

	    // see if the timestamp is valid.
	    uint32_t valid_from = deserialize_u32(&message->buf[SIG_SIZE]);
	    uint16_t valid_time = deserialize_u16(&message->buf[SIG_SIZE+4]);

	    uint64_t current_timestamp = get_timestamp();

	    if (valid_from > current_timestamp)
	    {
		// message is not yet valid
		uart_writeline("message is not yet valid, internal clock 0x", &current_timestamp);
#ifndef DEBUG_EN
		continue;
#endif
	    }
	    if (valid_from + valid_time < current_timestamp)
	    {
		// mesage is no longer valid
		uart_writeline("message is no longer valid, internal clock 0x", &current_timestamp);
#ifndef DEBUG_EN
		continue;
#endif
	    }
	}

	uint32_t message_type = deserialize_utf8(message->buf.data(), &offset, 1);

	uint32_t target_door_id = 0;

        // the message is valid, do its bidding.
        switch (message_type)
        {
        case 'I':
        {
            // an 'init' message. set the owner of a door.
            // payload:
            //    1x utf-8 system_id
	    //    1x utf-8 key_id
	    //    1x utf-8 key_type
	    //    uint8_t[32] key
	    //    1x utf-8 door_id
	    //    uint8_t 0x00 stop

	    // check if we are owned
	    if (owner_system_id != 0 || owner_door_id != 0)
	    {
		//system is already owned, so ignore message
		uart_writeline("door is already initialized");
		break;
	    }

	    //update keyslot 1 and owner info
	    owner_system_id = deserialize_utf8(message->buf.data(), &offset, 1);
	    keystore[1].system_id = owner_system_id;
	    keystore[1].key_id = deserialize_utf8(message->buf.data(), &offset, 1);
	    keystore[1].key_type = deserialize_utf8(message->buf.data(), &offset, 1);

	    memcpy(keystore[1].key, message->buf.data()+offset,32); //key is 32 bytes
	    offset += 32;
	    
	    owner_door_id = deserialize_utf8(message->buf.data(), &offset, 1);
	    keystore[1].door_id = owner_door_id;


	    //write owner and keystore to flash
	    owner_write();
	    key_store_write();
	    uart_writeline("door initialized");


	    break;
	}
	case 'O':
	{
	    // "open" message, open doors with provided ids in this system_id
	    // payload:
	    //   1x utf-8 door_id1
	    //   (1x utf-8 door_id2)
	    //   .
	    //   .
	    //   .
	    //   uint8_t 0x00 stop

	    // get our doorid for this key and check if in list
	    do
	    {
		target_door_id = deserialize_utf8(message->buf.data(), &offset, 1);
		if (target_door_id == keystore[current_key_index].door_id)
		{
		    // it seems like you're in luck.
		    uart_writeline("opening door");
		    open_door(motor);
		}
	    } while (target_door_id != 0);

	    uart_writeline("end of door_string");
            break;
        }
        case 'U':
        {
            // an 'Update key' message. add new key to keystore. only owner may do this
            // payload:
            //    1x utf-8 system_id (of the new key)
	    //    1x utf-8 key_id (of the new key)
	    //    1x utf-8 key_type (of the new key)
	    //    uint8_t[32] key
	    //    2x utf-8 doortupel
	    //    (2x utf-8 doortupel)
	    //    .
	    //    .
	    //    .
	    //    uint8_t 0x00 stop
	   uint32_t read_owner_door_id = 0;

	    // check, that the message is signed by the owner! only the owner may add key!

	    if (keystore[current_key_index].system_id != owner_system_id)
	    {
		uart_writeline("add key not allowed");
		break;
	    }

	    uint32_t new_system_id = deserialize_utf8(message->buf.data(), &offset, 1);
	    uint32_t new_key_id = deserialize_utf8(message->buf.data(), &offset, 1);
	    uint32_t new_key_type = deserialize_utf8(message->buf.data(), &offset, 1);

	    uint16_t key_offset = offset;
	    //jump over key
	    offset += 32;

	    //check if we are in the list of doors
	    do
	    {
		//get two utf-8 chars
		target_door_id = deserialize_utf8(message->buf.data(), &offset, 1);
		if (target_door_id == 0)
		{
		    //end of list
		    uart_writeline("local door id not in list");
		    break;
		}
		read_owner_door_id = deserialize_utf8(message->buf.data(), &offset, 1);
		if (read_owner_door_id == 0)
		{
		    //irregular end of list
		    uart_writeline("error in door id list");
		    break;
		}

		if (read_owner_door_id != owner_door_id)
		{
		    //we are not meant with this tupel
#ifdef DEBUG_EN
		    uart_writeline(".");
#endif
		    continue; //next tupel
		}

		//we found a tupel for us
		
		uart_writeline("trying to add new key");
		uint8_t new_keyslot = get_free_key_slot();
		if (new_keyslot == 0)
		{
		    uart_writeline("no more free keyslots available");
		    break;
		}
		keystore[new_keyslot].door_id = target_door_id;
		keystore[new_keyslot].key_id = new_key_id;
		keystore[new_keyslot].key_type = new_key_type ;
		keystore[new_keyslot].system_id = new_system_id;
		memcpy(keystore[new_keyslot].key, message->buf.data()+key_offset,32); //key is 32 bytes

		uart_writeline("new key added");

		//write keystore to flash
		key_store_write();

		//sorry, we only add one door id
		break;

	    } while (target_door_id != 0);
	    uart_writeline("command complete");

            break;
        }
	case 'F':
	{
            // 'flush' message. remove all keys of system, except specified key. only owner may do this
            // payload:
            //    1x utf-8 system_id
	    //    1x utf-8 key_id
	    //    1x utf-8 door_id1
	    //    (1y utf-8 door_id2)
	    //    .
	    //    .
	    //    .
	    //    uint8_t 0x00 stop


	    // check, that the message is signed by the owner! only the owner may add and remove key!

	    if (keystore[current_key_index].system_id != owner_system_id)
	    {
		uart_writeline("add key not allowed");
		break;
	    }
	    uint32_t target_system_id = deserialize_utf8(message->buf.data(), &offset, 1); //from this system we flush keys
	    uint32_t target_key_id = deserialize_utf8(message->buf.data(), &offset, 1); //except this one we keep

	    //check door_ids contains own door_id
	    do
	    {
		target_door_id = deserialize_utf8(message->buf.data(), &offset, 1);
		if (target_door_id == keystore[current_key_index].door_id)
		{
		    // it seems like you're in luck.
		    uart_writeline("door action is requested");
		    //if in door list: check if target_system_id|target_key_id in keystore

		    if (get_key_index(target_system_id, target_key_id, '1') != 0)
		    {
			//if yes remove all other keys for target_system_id, retain target_key_id
			remove_system_except_one_key(target_system_id, target_key_id);
			// write keystore to flash
			key_store_write();
		    }
		    else
		    {
			//if not abort with audible error
			//TODO make noise
			uart_writeline("error: key to retain not in keystore");
			break;
		    }
		}
	    } while (target_door_id != 0);
	    uart_writeline("command complete");

	    break;
	}
        default:
        {
            // unknown message type
            uart_writeline("unknown message type");
            continue;

            break;
        }
        }
    }
}

extern "C"
{

    void cpp_main()
    {
        cpp_main_in_cpp();
    }
}
