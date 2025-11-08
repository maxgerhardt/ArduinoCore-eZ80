#include <Arduino.h>
#include <stdint.h>
#include "ez80f92.h"
#include "ez80f92_peripherals.h"
#include "vectors.h"
#include "uart.h"
#include "bit_utils.h"

//==============================================================
// Configuration
//==============================================================

#define PWM_PORT_DR      IO(PC_DR)
#define PWM_PORT_DDR     IO(PC_DDR)

#define MAX_PWM_CHANNELS   8
#define PWM_TIMER_FREQ_HZ  50
#define PWM_RESOLUTION     256

//==============================================================
// Globals
//==============================================================

// Flattened LUT: pwm_flat[row*MAX_PWM_CHANNELS + ch] = (1<<ch) if duty else 0
static uint8_t pwm_flat[PWM_RESOLUTION * MAX_PWM_CHANNELS];

// Pointer to current row in ISR
static uint8_t *pwm_row_ptr = pwm_flat;

// Active PWM mask
static uint8_t pwm_mask = 0;
static volatile uint8_t inv_pwm_mask = 0xff;
static uint8_t active_channels = 0;

//==============================================================
// Timer helpers
//==============================================================

static inline uint16_t compute_reload(void)
{
    const uint32_t divider = 4;
    uint32_t reload = (F_CPU / divider) / (PWM_TIMER_FREQ_HZ * PWM_RESOLUTION);
    if (reload == 0) reload = 1;
    return (uint16_t)reload;
}

static void timer_start(void)
{
    if (IO(TMR1_CTL) & TMR_CTL_PRT_EN)
        return; // already running

    uint16_t reload = compute_reload();
    IO(TMR1_RR_L) = (uint8_t)(reload & 0xFF);
    IO(TMR1_RR_H) = (uint8_t)(reload >> 8);
    IO(TMR_ISS) &= ~0x0C; // system clock for timer 1

    IO(TMR1_CTL) = TMR_CTL_MODE_CONT |
                   TMR_CTL_CLKDIV_4 |
                   TMR_CTL_IRQ_EN |
                   TMR_CTL_PRT_EN;
}

static inline void timer_stop(void)
{
    IO(TMR1_CTL) = 0x00;
}

//==============================================================
// ISR
//==============================================================

__attribute__((interrupt))
void PRT1_Handler(void)
{
    IO(TMR1_CTL); // clear interrupt flag

    // OR 8 channels
    uint8_t bits =
        pwm_row_ptr[0] | pwm_row_ptr[1] | pwm_row_ptr[2] | pwm_row_ptr[3] |
        pwm_row_ptr[4] | pwm_row_ptr[5] | pwm_row_ptr[6] | pwm_row_ptr[7];

    // write port
    PWM_PORT_DR = (PWM_PORT_DR & inv_pwm_mask) | (bits);

    // advance pointer and counter
    pwm_row_ptr += MAX_PWM_CHANNELS;

    // Wrap row pointer to beginning after 256 iterations
    if(pwm_row_ptr >= pwm_flat + sizeof(pwm_flat))
        pwm_row_ptr = pwm_flat;
}

//==============================================================
// Internal helpers
//==============================================================

static void fill_pwm_table(uint8_t channel, uint8_t duty, uint8_t val)
{
    uint8_t *p = &pwm_flat[channel];
    // Fill duty
    for (uint16_t i = 0; i < duty; i++, p += MAX_PWM_CHANNELS)
        *p = val;
    // Fill remainder with 0
    for (uint16_t i = duty; i < PWM_RESOLUTION; i++, p += MAX_PWM_CHANNELS)
        *p = 0;
}

//==============================================================
// API
//==============================================================

void pwm_init(void)
{
    IO(TMR1_CTL) = 0x00;
    _set_vector(VECTOR_PRT_1, PRT1_Handler);
    __asm__("ei");
}

void pwm_disable(uint8_t channel)
{
    if (channel >= MAX_PWM_CHANNELS)
        return;

    uint8_t bit = (1u << channel);
    pwm_mask &= ~bit;
    inv_pwm_mask = ~(pwm_mask);
    if (active_channels)
        active_channels--;
    if (active_channels == 0)
        timer_stop();
}

void pwm_write(uint8_t channel, uint8_t duty)
{
    if (channel >= MAX_PWM_CHANNELS)
        return;

    uint8_t bit = (1u << channel);

    // Handle static duty (0% or 100%)
    if (duty == 0 || duty == 255)
    {
        pwm_disable(channel);
        // additionally clear table to 0 for that channel:
        // saves ISR from doing a & mask
        uint8_t *p = &pwm_flat[channel];
        for (uint16_t i = 0; i < PWM_RESOLUTION; i++, p += MAX_PWM_CHANNELS)
            *p = 0;
        digitalWrite(MAKE_PIN(PORTC, channel), duty == 0 ? LOW : HIGH);
        return;
    }

    // Update lookup table
    fill_pwm_table(channel, duty, bit);

    // Activate channel if not already
    if (!(pwm_mask & bit))
    {
        pwm_mask |= bit;
        inv_pwm_mask = ~(pwm_mask);
        pinMode(MAKE_PIN(PORTC, channel), OUTPUT);
        active_channels++;
        timer_start();
    }
}
