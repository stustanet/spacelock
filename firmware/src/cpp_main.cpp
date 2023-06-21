#include "stm32f1xx_hal.h"

#include "base64.h"
#include "config.h"
#include "dcf77.h"
#include "deserialize.h"
#include "hardware.h"
#include "hmac.h"
#include "motor.h"
#include "secret_key.h"
#include "sha256.h"
#include "time.h"

static void cpp_main_in_cpp();
static void current_controlled_open(StepperMotor &motor);
static void forced_open(StepperMotor &motor);
static void lock_door(StepperMotor &motor);
static void advanced_door_unlocking(StepperMotor &motor, InputPin & door_state);

static void current_controlled_open(StepperMotor &motor)
{
    // uart_writeline("opening door \U0001f308");
    motor.set_mode(1);
    
    auto [end_stop_state, urevs_since_endstop] = motor.rotate(UNLOCKING_UREVS, UNLOCK_SPEED_UREVS_PER_SEC, ENDSTOP_DEAD_WINDOW_UREVS);
    if (end_stop_state == EndStopState::RISING_EDGE)
    {
        // keep moving for a few extra usteps
        motor.set_mode(32);
        // keep moving for a few extra usteps
        if (urevs_since_endstop < ENDSTOP_ROTATE_AFTER_EDGE_USTEPS)
        {
            motor.rotate(ENDSTOP_ROTATE_AFTER_EDGE_USTEPS - urevs_since_endstop, UNLOCK_FINAL_SPEED_UREVS_PER_SEC, -1);
        }
    }
    else
    {
        uart_writeline("over current detected");
    }

    motor.set_mode(0);
    sleep_us(UNLOCK_HOLD_DELAY_US);

    // back up a little bit
    motor.set_mode(-1);
    motor.rotate(UNLOCK_BACKING_UREVS, UNLOCK_SPEED_UREVS_PER_SEC, -1);
    motor.set_mode(0);
}

static void forced_open(StepperMotor &motor){


    // try to open the door by force
    motor.set_mode(1);
    motor.rotate(FORCE_UNLOCK_UREVS, UNLOCK_SPEED_UREVS_PER_SEC, -1);
    motor.set_mode(32);
    motor.rotate(ENDSTOP_ROTATE_AFTER_EDGE_USTEPS*2, UNLOCK_FINAL_SPEED_UREVS_PER_SEC, -1);
    motor.set_mode(0);
    sleep_us(UNLOCK_HOLD_DELAY_US);
    motor.set_mode(-1);
    motor.rotate(UNLOCK_BACKING_UREVS, UNLOCK_SPEED_UREVS_PER_SEC, -1);
    motor.set_mode(0);
}

static void lock_door(StepperMotor &motor){
    //rotate backwards
    motor.set_mode(-1);

    motor.rotate(LOCKING_UREVS, LOCKING_SPEED_UREVS_PER_SEC, 2*ENDSTOP_DEAD_WINDOW_UREVS);
    uart_writeline("locking door");
    motor.set_mode(0);
}

/*
 * This function retries opening, if a door sensor is installed.
 * It checks if the door is opened and if not it tries again.
 */
static void advanced_door_unlocking(StepperMotor &motor, InputPin & door_state){
    uart_writeline("opening door");
    
    // try opening 3 times
    for(int i = 0;i<3 && door_state.get() != DOOR_OPEN_STATE;i++){
        current_controlled_open(motor);
    }

    forced_open(motor); // ignore endstop
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
    secret_key_write((unsigned char *)"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00");
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

#if WITH_QRCODE
    // TIME SIG is connected directly to the DCF77 signal,
    // it doesn't exist here
    // TIME STATE tells us whether a correct time has been received.
    OutputPin led_timestate(GPIOC, GPIO_PIN_14);

    InputPin dcf77_pin(GPIOA, GPIO_PIN_8);
    dcf77_init(&dcf77_pin, &led_timestate);
#endif // WITH_QRCODE

#if WITH_DOOR_SENSOR
    InputPin door_state(DOOR_PORT, DOOR_PIN);
    bool lock_armed = false;
    uint32_t lock_counter = 0;
    uint64_t lock_target_time = 0;
    bool old_door_state = door_state.get();
#endif // WITH_DOOR_SENSOR

    uart_writeline("Spacelock initialized! \U0001F389");

    while (1)
    {
#if WITH_DOOR_SENSOR
        if (door_state.get() == DOOR_OPEN_STATE)
        {
            // door is open
            led_doorstate.set();
            lock_armed = true;
            lock_target_time = time_get_64_isr();
            if(old_door_state != DOOR_OPEN_STATE){
                old_door_state = DOOR_OPEN_STATE;
                uart_writeline("xxxOpenxxx");
            }
        }
        else
        {
            // door is closed
            led_doorstate.reset();
            if (lock_armed && (time_get_64_isr() > LOCK_DELAY + lock_target_time))
            {
                uint64_t val = time_get_64_isr();
                lock_door(motor);
                lock_armed = false;
            }
            if(old_door_state == DOOR_OPEN_STATE){
                old_door_state = !DOOR_OPEN_STATE;
                uart_writeline("xxxClosexxx");
            }
        }
#endif // WITH_DOOR_SENSOR

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
            (message->buf[0] == 'o') &&
            (message->buf[1] == 'p') &&
            (message->buf[2] == 'e') &&
            (message->buf[3] == 'n'))
        {
            // nothing to see here
#if WITH_BACKDOOR
            uart_writeline("requested opening on UART");

    #if WITH_DOOR_SENSOR
            advanced_door_unlocking(motor, door_state);
    #else
            current_controlled_open(motor);
    #endif
#else
            uart_writeline("backdoor disabled");
#endif
            continue;
        }

#if WITH_QRCODE
        // base64-decode the message.
        uint32_t size = base64_decode(message->buf.data(), message->buf_pos);
        if (size == 0)
        {
            // the base64-decoded message is empty
            uart_writeline("base64-decoded message is empty");
            continue;
        }

        // all messages have the following format:
        //    uint8_t   hmac_signature[HMAC_SIZE]
        //    uint64_t  valid_from
        //    uint64_t  valid_until
        //    uint8_t   type
        //    uint8_t   payload[]       (variable length)

        if (size <= HMAC_SIZE + 17)
        {
            // the message is too small
            uart_writeline("message is too small");
            continue;
        }

        // calculate the message HMAC
        uint8_t digest[32];
        hmac(message->buf.data() + HMAC_SIZE, size - HMAC_SIZE, digest);

        // prevent timing side-channel attacks through the use of 'volatile'
        volatile bool signature_ok = true;
        for (uint32_t i = 0; i < HMAC_SIZE; i++)
        {
            signature_ok &= (digest[i] == message->buf[i]);
        }
        if (!signature_ok)
        {
            uart_writeline("HMAC fail");
            continue;
        }

        // see if the timestamp is valid.
        uint64_t valid_from = deserialize_u64(&message->buf[HMAC_SIZE]);
        uint64_t valid_until = deserialize_u64(&message->buf[HMAC_SIZE + 8]);

        uint64_t current_timestamp = get_timestamp();

        if (valid_from > current_timestamp)
        {
            // message is not yet valid
            uart_writeline("message is not yet valid, internal clock 0x", &current_timestamp);
            continue;
        }
        if (valid_until < current_timestamp)
        {
            // mesage is no longer valid
            uart_writeline("message is no longer valid, internal clock 0x", &current_timestamp);
            continue;
        }

        const uint8_t message_type = message->buf[HMAC_SIZE + 16];
        const uint8_t *payload = &(message->buf[HMAC_SIZE + 17]);
        uint8_t payload_size = size - HMAC_SIZE - 17;

        // the message is valid, do its bidding.
        switch (message_type)
        {
        case 0x01:
        {
            // an 'open the door' message.
            // payload:
            //    char *    uid             (variable length)

            if (!check_info(payload, payload_size))
            {
                // info is not valid
                uart_writeline("message info is not valid");
                continue;
            }

            // it seems like you're in luck.
            uart_writeline("opening door");
    #if WITH_DOOR_SENSOR
            advanced_door_unlocking(motor, door_state);
    #else
            current_controlled_open(motor);
    #endif
            break;
        }
        case 0x02:
        {
            // an 'new SECRET_KEY' message.
            // payload:
            //    uint8_t *    new_key_seed            (variable length)

            if (payload_size < 1)
            {
                uart_writeline("payload is not valid");
                continue;
            }

            // calculate the new secret key
            SHA256 hash;
            hash.update(SECRET_KEY, sizeof(SECRET_KEY));
            hash.update(payload, payload_size);
            uint8_t digest[32];
            hash.calculate_digest(digest);

            uart_writeline("writing new secret key");

            // write the new secret key
            secret_key_write(digest);

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
#endif // WITH_QRCODE
    }
}

extern "C"
{

    void cpp_main()
    {
        cpp_main_in_cpp();
    }
}
