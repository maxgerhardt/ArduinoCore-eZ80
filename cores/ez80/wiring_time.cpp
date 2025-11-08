#include <Arduino.h>
#include <stdint.h>
#include "vectors.h"

#define TIMER_FREQ_HZ      1000UL       // 1 kHz (1 ms period)

static volatile unsigned int elapsed_ms = 0;  // millisecond counter
extern "C" void PRT0_Handler(void);

static inline uint16_t compute_reload(void)
{
    const uint32_t divider = 64;
    uint32_t reload = (F_CPU / divider) / TIMER_FREQ_HZ;
    return (uint16_t)reload;
}

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
    uint16_t reload = compute_reload();
    IO(TMR0_RR_L) = (uint8_t)(reload & 0xFF);
    IO(TMR0_RR_H) = (uint8_t)(reload >> 8);

    // 5. Configure control register:
    //    - Enable reload
    //    - Continuous mode
    //    - /64 clock divider
    //    - Interrupt enable
    //    - Start timer
    IO(TMR0_CTL) = TMR_CTL_RST_EN |
                   //TMR_CTL_MODE_SP  |
                   TMR_CTL_MODE_CONT |
                   //TMR_CTL_CLKDIV_256 |
                   TMR_CTL_CLKDIV_64 |
                   TMR_CTL_IRQ_EN |
                   TMR_CTL_PRT_EN;
}

uint16_t get_timer_cnt() {
    uint8_t low = IO(TMR0_DR_L);
    uint8_t high = IO(TMR0_DR_H);
    return (uint16_t)((high << 16u) | low);
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
    return elapsed_ms * 1000UL;
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
