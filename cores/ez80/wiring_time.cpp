#include <Arduino.h>
#include <stdint.h>
#include "vectors.h"

#define TIMER_FREQ_HZ   1000UL       // 1 kHz (1 ms period)
#define TIMER_RELOAD_VAL ((uint16_t)((F_CPU / 4) / TIMER_FREQ_HZ))
// If F_CPU 18.432 MHz then F_CPU/4000/1000 = 4608 (0x1200)
// max value of ticks can be 0x1200 when no counting has occurred and minimal value 0
// so ticks * 1000 can be at maximum 0x465000 (fits in 3 byte int)
#define TICKS_TO_US(ticks) (((unsigned)(ticks) * (1000)) / (F_CPU / 4000))

volatile unsigned int elapsed_ms = 0;  // millisecond counter
extern "C" void PRT0_Handler(void);

void PRT0_Init(void)
{
    // 1. Disable timer during setup
    IO(TMR0_CTL) = 0x00;

    // 2. Hook ISR to vector
    _set_vector(VECTOR_PRT_0, PRT0_Handler);
    __asm("ei");

    // 3. Select system clock as input source for TMR0 (bits [1:0] = 00)
    IO(TMR_ISS) &= ~0x03;

    // 4. Compute and load reload value
    const uint16_t reload = TIMER_RELOAD_VAL;
    IO(TMR0_RR_L) = (uint8_t)(reload & 0xFF);
    IO(TMR0_RR_H) = (uint8_t)(reload >> 8);

    // 5. Configure control register:
    //    - Enable reload
    //    - Continuous mode
    //    - /4 clock divider for maximum accuracy
    //    - Interrupt enable
    //    - Start timer
    IO(TMR0_CTL) = TMR_CTL_RST_EN |
                   TMR_CTL_MODE_CONT |
                   TMR_CTL_CLKDIV_4 |
                   TMR_CTL_IRQ_EN |
                   TMR_CTL_PRT_EN;
}

static inline uint16_t get_timer_cnt() {
    uint8_t low = IO(TMR0_DR_L);
    uint8_t high = IO(TMR0_DR_H);
    return (uint16_t)((high << 8u) | low);
}

//==============================================================
// Interrupt Service Routine for PRT0
//==============================================================

__attribute__((interrupt))
void PRT0_Handler(void)
{
    // Reading TMR0_CTL clears the interrupt flag
    // (this IO read will not be optimized away (good)!)
    IO(TMR0_CTL);

    // Increment the millisecond counter
    elapsed_ms++;
    // interrupts are automatically reenabled before leaving the function
    // through EI RETI instruction
}
 

void init_millis(void) {
    PRT0_Init();
}

unsigned long millis(void) {
    return elapsed_ms;
}
unsigned long micros(void) {
    __asm("di");
    // capture the slowest moving element
    unsigned int local_ms = elapsed_ms;
    // then the fastest moving element
    uint16_t curr_timer = get_timer_cnt();
    __asm("ei");
    // since it's a downcounting timer, we can get the elapsed time by taking the current timer value
    // and subtracting the minimum. If they're equal, no time has passed.
    uint16_t elapsed_ticks = (TIMER_RELOAD_VAL - curr_timer);
    // this can be at maximum 1000 elapsed micros (aka 1ms)
    unsigned int elapsed_us = TICKS_TO_US(elapsed_ticks);
    unsigned long ret =  (local_ms * 1000UL) + (unsigned long)(elapsed_us);
    return ret;
}

void delay(unsigned long ms) {
    unsigned int start = elapsed_ms;
    while(elapsed_ms - start < (unsigned int)ms) {
        ;
    } 
}

void delayMicroseconds(unsigned int us) {
    // todo better cylce delay
}
