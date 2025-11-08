#include <Arduino.h>
#include <ez80f92.h>
#include <uart.h>
#include <software_pwm.h>

extern void init_millis(void);

void init(void) {
    uart0_init();
    init_millis();
    pwm_init(); /* for software PWM on PORTC */
}

int main(void) {
    /* do low level init like starting millis timer..*/
    init();
    setup();
    while(1) {
        loop();
    }
    return 0;
}