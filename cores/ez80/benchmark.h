#pragma once

#include <Arduino.h>
#include <stdint.h>
#include <uart.h>

extern volatile unsigned int elapsed_ms;
/* purposefully allocate new static variable per included file */
/* allows nested benchmarking if function calls are across compilation units */
static volatile unsigned int saved_start_ms = 0;
static volatile uint8_t saved_start_tim_low = 0;
static volatile uint8_t saved_start_tim_high = 0;

#define TIMER_FREQ_HZ   1000UL       // 1 kHz (1 ms period)
#define TIMER_RELOAD_VAL ((uint16_t)((F_CPU / 4) / TIMER_FREQ_HZ))
#define TICKS_TO_US(ticks) (((ticks) * (1000u)) / (F_CPU / 4000u))

/* functions which are inlined and give highly accurate timing results. */

static inline void benchmark_start() {
    /* only quickly save in variables, do not convert them in any way. */
    __asm("di");
    saved_start_tim_low = IO(TMR0_DR_L);
    saved_start_tim_high = IO(TMR0_DR_H);
    saved_start_ms = elapsed_ms;
    __asm("ei");
}

/* returns elapsed time since benchmark_start() in microseconds */
static inline unsigned long benchmark_stop() {
    __asm("di");
    /* capture state now */
    volatile uint8_t end_low = IO(TMR0_DR_L);
    volatile uint8_t end_high = IO(TMR0_DR_H);
    volatile unsigned int end_ms = elapsed_ms;
    __asm("ei");
    // convert both in microseconds and subtract them
    uint16_t ticks_start = (uint16_t)((saved_start_tim_high << 8u) | saved_start_tim_low);
    uint16_t ticks_end = (uint16_t)((end_high << 8u) | end_low);
    unsigned long elapsed_ticks_start = (TIMER_RELOAD_VAL - ticks_start);
    unsigned long elapsed_ticks_end = (TIMER_RELOAD_VAL - ticks_end);
    unsigned long elapsed_us_start = TICKS_TO_US(elapsed_ticks_start);
    unsigned long elapsed_us_end = TICKS_TO_US(elapsed_ticks_end);
    unsigned long micros_start = (saved_start_ms * 1000UL) + (unsigned long)(elapsed_us_start);
    unsigned long micros_end = (end_ms * 1000UL) + (unsigned long)(elapsed_us_end);
    return micros_end - micros_start;
}