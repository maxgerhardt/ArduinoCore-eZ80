#include <ez80f92.h>
#include <Arduino.h>

extern void init_millis(void);

void init(void) {
    init_millis();
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