#pragma once

/* for eZ80 devices, we only have three controllable GPIO ports with pins ranging from 0 to 7 (3 bits) */
#define PORTB 0
#define PORTC 1
#define PORTD 2
#define MAKE_PIN(port, pin) (((port) << 3) | (pin))
#define GET_PORT(portpin) ((portpin) >> 3)
#define GET_PIN(portpin) ((portpin) & 0x07)