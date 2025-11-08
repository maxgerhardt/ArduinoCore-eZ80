#include "Arduino.h"
#include "pins_api.h"
#include "software_pwm.h"

void analogWrite(pin_size_t pinNumber, int val)
{
    const uint8_t port = GET_PORT(pinNumber);
    const uint8_t pin = GET_PIN(pinNumber);
    if (port == PORTC) {
        pwm_write(pin, (uint8_t) val);
    }
}