#include "stm32f1xx_hal.h"

#include "base64.h"
#include "dcf77.h"
#include "deserialize.h"
#include "hardware.h"
#include "secret_key.h"
#include "motor.h"
#include "sha256.h"
#include "time.h"

#define WITH_BACKDOOR 1

static void cpp_main_in_cpp();
static void open_door(StepperMotor &motor);

static void open_door(StepperMotor &motor) {
    motor.set_mode(1);
    // TODO: find the correct number which does a full rotation
    motor.rotate(2000000, 1000000 * 4);
    sleep_us(3000000);
    motor.set_mode(-1);
    // TODO: find the correct number which does a quarter or so backrotation
    motor.rotate(2000000, 1000000 * 4);
    motor.set_mode(0);
}

static bool check_info(const uint8_t *info, uint32_t info_size) {
    if (info_size < 1) {
        return false;
    }

    while (info_size) {
        if (*info < 0x20 || *info >= 0x7f) {
            // illegal character in info string
            return false;
        }

        info++;
        info_size -= 1;
    }
    return true;
}

static void cpp_main_in_cpp() {
    StepperMotor motor(
        OutputPin(GPIOA, GPIO_PIN_5),          // step
        OutputPin(GPIOA, GPIO_PIN_6),          // sleep
        OutputPin(GPIOA, GPIO_PIN_4),          // direction
        {
            OutputPin(GPIOB, GPIO_PIN_10),     // microstep modesel 0
            OutputPin(GPIOB, GPIO_PIN_1),      // microstep modesel 1
            OutputPin(GPIOB, GPIO_PIN_0)       // microstep modesel 2
        },
        InputPin(GPIOA, GPIO_PIN_1),           // clockwise end switch
        InputPin(GPIOA, GPIO_PIN_0)            // counterclockwise end switch
    );

    InputPin dcf77_pin(GPIOA, GPIO_PIN_8);
    dcf77_init(&dcf77_pin);

    while (1)
    {
        UARTRxBuffer *message = uart_poll_message();
        if (message == nullptr) {
            // no new message is ready
            continue;
        }
        if (message->buf_pos == 0) {
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
            (message->buf[7] == 'r')
        ) {
            // nothing to see here
#if WITH_BACKDOOR
            open_door(motor);
#endif
            continue;
        }

        // base64-decode the message.
        uint32_t size = base64_decode(message->buf.data(), message->buf_pos);
        if (size == 0) {
            // the base64-decoded message is empty
            continue;
        }

        // all messages have the following format:
        //    uint8_t   hmac_signature[HMAC_SIZE]
        //    uint64_t  valid_from
        //    uint64_t  valid_until
        //    uint8_t   type
        //    uint8_t   payload[]       (variable length)

        if (size <= HMAC_SIZE + 17) {
            // the message is too small
            continue;
        }

        // calculate the message HMAC
        SHA256 hash;
        hash.update(SECRET_KEY, sizeof(SECRET_KEY));
        hash.update(message->buf.data() + HMAC_SIZE, size - HMAC_SIZE);
        uint8_t digest[32];
        hash.calculate_digest(digest);
    
        // prevent timing side-channel attacks through the use of 'volatile'
        volatile bool signature_ok = true;
        for (uint32_t i = 0; i < HMAC_SIZE; i++) {
            signature_ok &= (digest[i] == message->buf[i]);
        }
        if (!signature_ok) { continue; }

        // see if the timestamp is valid.
        uint64_t valid_from = deserialize_u64(&message->buf[HMAC_SIZE]);
        uint64_t valid_until = deserialize_u64(&message->buf[HMAC_SIZE + 8]);

        uint64_t current_timestamp = get_timestamp();

        if (valid_from > current_timestamp) {
            // message is not yet valid
            continue;
        }
        if (valid_until < current_timestamp) {
            // mesage is no longer valid
            continue;
        }

        const uint8_t message_type = message->buf[HMAC_SIZE + 16];
        const uint8_t *payload = &(message->buf[HMAC_SIZE + 17]);
        uint8_t payload_size = size - HMAC_SIZE - 17;

        // the message is valid, do its bidding.
        switch (message_type) {
        case 0x01: {
            // an 'open the door' message.
            // payload:
            //    char *    uid             (variable length)

            if (!check_info(payload, payload_size)) {
                // info is not valid
                continue;
            }

            // it seems like you're in luck.
            open_door(motor);
            break;
        }
        case 0x02: {
            // an 'new SECRET_KEY' message.
            // payload:
            //    uint8_t *    new_key_seed            (variable length)

            if (payload_size < 1) { continue; }

            // calculate the new secret key
            SHA256 hash;
            hash.update(SECRET_KEY, sizeof(SECRET_KEY));
            hash.update(payload, payload_size);
            uint8_t digest[32];
            hash.calculate_digest(digest);

            // write the new secret key
            secret_key_write(digest);

            break;
        }
        default: {
            // unknown message type
            continue;

            break;
        }
        }
    }
}

extern "C" {

void cpp_main() {
    cpp_main_in_cpp();
}

}
