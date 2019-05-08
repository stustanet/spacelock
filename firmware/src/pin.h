#pragma once

#include "stm32f1xx_hal.h"

#ifndef __cplusplus
#error lolnope
#endif

/**
 * Class for handling GPIO input pins. Is trivially copyable.
 */
class InputPin {
public:
    inline InputPin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
        :
        gpiox{GPIOx},
        gpio_pin{GPIO_Pin}
    {}

    inline bool get() {
        return HAL_GPIO_ReadPin(this->gpiox, this->gpio_pin);
    }

private:
    GPIO_TypeDef *gpiox;
    uint16_t gpio_pin;
};

/**
 * Class for handling GPIO output pins. Is trivially copyable.
 */
class OutputPin {
public:
    inline OutputPin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
        :
        gpiox{GPIOx},
        gpio_pin{GPIO_Pin}
    {}

    /** sets the pin to the given value */
    inline void set(bool value=true) {
        if (value)
        {
            HAL_GPIO_WritePin(this->gpiox, this->gpio_pin, GPIO_PIN_SET);
        }
        else
        {
            HAL_GPIO_WritePin(this->gpiox, this->gpio_pin, GPIO_PIN_RESET);
        }
    }

    /** sets the pin to low (0) */
    inline void reset() {
        this->set(false);
    }

    /** sets the pin to high (1) */
    inline void high() {
        this->set(true);
    }

    /** sets the pin to low (0) */
    inline void low() {
        this->set(false);
    }

    /** toggles the pin */
    inline void toggle() {
        HAL_GPIO_TogglePin(this->gpiox, this->gpio_pin);
    }

private:
    GPIO_TypeDef *gpiox;
    uint16_t gpio_pin;
};