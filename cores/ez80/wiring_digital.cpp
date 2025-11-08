#include <Arduino.h>
#include <pins_api.h>

static inline uint8_t get_alt1_port(uint8_t port) {
    return PB_ALT1 + (port * 4);
    //return port == PORTB ? PB_ALT1 : (port == PORTC ? PC_ALT1 : PD_ALT1);
}

static inline uint8_t get_alt2_port(uint8_t port) {
    return PB_ALT2 + (port * 4);
    //return port == PORTB ? PB_ALT2 : (port == PORTC ? PC_ALT2 : PD_ALT2);
}

static inline uint8_t get_ddr_port(uint8_t port) {
    return PB_DDR + (port * 4);
    //return port == PORTB ? PB_DDR : (port == PORTC ? PC_DDR : PD_DDR);
}

static inline uint8_t get_dr_port(uint8_t port) {
    return PB_DR + (port * 4);
    //return port == PORTB ? PB_DR : (port == PORTC ? PC_DR : PD_DR);
}

void pinMode(pin_size_t pinNumber, PinMode pinMode) {
    // get port an pin number back
    const uint8_t port = GET_PORT(pinNumber);
    const uint8_t pin = GET_PIN(pinNumber);
    const uint8_t pin_mask = (uint8_t)(1 << pin);
    const int ioport_alt1 = get_alt1_port(port);
    const int ioport_alt2 = get_alt2_port(port);
    const int ioport_ddr = get_ddr_port(port);
    uint8_t alt1 = 0;
    uint8_t alt2 = 0;
    uint8_t ddr = 0;

    switch(pinMode) {
        case INPUT: {
            ddr = pin_mask;
            alt1 = 0;
            alt2 = 0;
            break;
        }
        case OUTPUT: {
            ddr = 0;
            alt1 = 0;
            alt2 = 0;
            break;
        }
        case OUTPUT_OPENDRAIN: {
            ddr = 0;
            alt1 = pin_mask;
            alt2 = 0;
            break;
        }
        /* we can also do OUTPUT_OPENSOURCE / OPENCOLLECTOR but this is not exposed in the Arduino API */
        default:
            /* unsupported */
            break;
    }
    // Delete the bit for the pin then set its desired value
    IO(ioport_alt1) = (IO(ioport_alt1) & ~(pin_mask)) | alt1;
    IO(ioport_alt2) = (IO(ioport_alt2) & ~(pin_mask)) | alt2;
    IO(ioport_ddr)  = (IO(ioport_ddr)  & ~(pin_mask)) | ddr;
}
void digitalWrite(pin_size_t pinNumber, PinStatus status) {
    const uint8_t port = GET_PORT(pinNumber);
    const uint8_t pin = GET_PIN(pinNumber);
    const uint8_t pin_mask = (uint8_t)(1u << pin);
    /* try unrolling */
    #if 0
    if(port == PORTB) {
        if(status == HIGH) {
            IO(PB_DR) |= pin_mask;
        } else {
            IO(PB_DR) &= ~pin_mask;
        }
    } else if(port == PORTC) {
        if(status == HIGH) {
            IO(PC_DR) |= pin_mask;
        } else {
            IO(PC_DR) &= ~pin_mask;
        }
    } else {
        if(status == HIGH) {
            IO(PD_DR) |= pin_mask;
        } else {
            IO(PD_DR) &= ~pin_mask;
        }
    }
    #else
    //const int ioport_dr = get_dr_port(port);
    /* memory adddresses are such that PB_DR = 0x9A, PC_DR = 0x9E, PD_DR = 0xA2 */
    /* so each port is just 0x4 bytes away from each other */
    /* meaning io_dr = 0x9A + (port * 4) */
    const uint8_t ioport_dr = PB_DR + (uint8_t)(port << 2u);
    //IO((int)ioport_dr) = (IO((int)ioport_dr) & (~pin_mask)) | (uint8_t)status;
    if(status == HIGH) {
        IO((int)ioport_dr) |= pin_mask;
    } else {
        IO((int)ioport_dr) &= ~pin_mask;
    }
    #endif
}

PinStatus digitalRead(pin_size_t pinNumber) {
    const uint8_t port = GET_PORT(pinNumber);
    const uint8_t pin = GET_PIN(pinNumber);
    const uint8_t pin_mask = (uint8_t)(1 << pin);
    const int ioport_dr = get_dr_port(port);
    return (IO(ioport_dr) & pin_mask) != 0 ? HIGH : LOW;
}
