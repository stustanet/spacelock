#include "stm32f1xx_hal.h"

#include "base64.h"
#include "dcf77.h"
#include "deserialize.h"
#include "hardware.h"
#include "hmac.h"
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
        OutputPin(GPIOA, GPIO_PIN_7),          // step
        OutputPin(GPIOA, GPIO_PIN_6),          // sleep
        OutputPin(GPIOA, GPIO_PIN_5),          // direction
        {
            OutputPin(GPIOA, GPIO_PIN_4),      // microstep modesel 0
            OutputPin(GPIOA, GPIO_PIN_3),      // microstep modesel 1
            OutputPin(GPIOA, GPIO_PIN_2)       // microstep modesel 2
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

        // calculate the message HMAC
        if (size <= HMAC_SIZE) {
            // the HMAC-signed message is empty
            continue;
        }

        SHA256 hash;
        hash.update(SECRET, sizeof(SECRET));
        hash.update(message->buf.data() + HMAC_SIZE, size - HMAC_SIZE);
        uint8_t digest[32];
        hash.calculate_digest(digest);
        for (uint32_t i = 0; i < HMAC_SIZE; i++) {
            if (digest[i] != message->buf[i]) {
                // the HMAC signature is wrong
                continue;
            }
        }

        // looks like the message is valid.
        // check its type.

        switch (message->buf[HMAC_SIZE]) {
        case 0x01: {
            // this message consists of:
            //    uint8_t   type            = 0x01
            //    uint64_t  valid_from
            //    uint64_t  valid_until
            //    char *    info            (variable length)

            if (size < HMAC_SIZE + 17) {
                // the message is too small.
                continue;
            }

            uint64_t valid_from = deserialize_u64(&message->buf[HMAC_SIZE + 1]);
            uint64_t valid_until = deserialize_u64(&message->buf[HMAC_SIZE + 9]);

            uint64_t current_timestamp = get_timestamp();

            if (valid_from > current_timestamp) {
                // token is not yet valid
                continue;
            }
            if (valid_until < current_timestamp) {
                // token is no longer valid
                continue;
            }

            uint8_t *info = &message->buf[HMAC_SIZE + 17];
            uint32_t info_size = size - HMAC_SIZE - 17;
            if (!check_info(info, info_size)) {
                // info is not valid
                continue;
            }

            // it seems like you're in luck.
            open_door(motor);

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
