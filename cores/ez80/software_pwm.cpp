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
#define PWM_TIMER_FREQ_HZ  60
#define PWM_RESOLUTION     256

//==============================================================
// Globals
//==============================================================

// Interleaved table: pwm_table[tick][channel] = (1<<channel) if active
static uint8_t pwm_table[PWM_RESOLUTION][MAX_PWM_CHANNELS];

static volatile uint8_t pwm_counter = 0;
static volatile uint8_t pwm_mask = 0;
static volatile uint8_t active_channels = 0;

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
    IO(TMR_ISS) &= ~0x0C; // system clock

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
// ISR — copy volatile variables first, 8 reads, 7 ORs, 1 port write
//==============================================================

__attribute__((interrupt))
void PRT1_Handler(void)
{
    IO(TMR1_CTL); // clear interrupt flag

    uint8_t ctr = pwm_counter;
    const uint8_t mask = pwm_mask;

    // Read 8 channels for this tick
    const uint8_t bits =
        pwm_table[ctr][0] |
        pwm_table[ctr][1] |
        pwm_table[ctr][2] |
        pwm_table[ctr][3] |
        pwm_table[ctr][4] |
        pwm_table[ctr][5] |
        pwm_table[ctr][6] |
        pwm_table[ctr][7];

    // Update port
    uint8_t port = PWM_PORT_DR;
    port = (port & ~mask) | (bits & mask);
    PWM_PORT_DR = port;

    pwm_counter = ctr + 1;
}

//==============================================================
// Internal helpers
//==============================================================

static void fill_pwm_table(uint8_t ch, uint8_t duty, uint8_t val)
{
    for(uint16_t i = 0; i < duty; i++)
        pwm_table[i][ch] = val;

    for(uint16_t i = duty; i < PWM_RESOLUTION; i++)
        pwm_table[i][ch] = 0;
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
    active_channels--;

    if (active_channels == 0)
        timer_stop();
}

void pwm_write(uint8_t channel, uint8_t duty)
{
    if (channel >= MAX_PWM_CHANNELS)
        return;

    uint8_t bit = (1u << channel);

    // Special case: static output
    if (duty == 0 || duty == 255) {
        pwm_disable(channel);
        digitalWrite(MAKE_PIN(PORTC, channel), duty == 0 ? LOW : HIGH);
        return;
    }

    // Fill the channel's lookup table
    fill_pwm_table(channel, duty, bit);

    // Activate channel if not already
    if (!(pwm_mask & bit)) {
        pwm_mask |= bit;
        pinMode(MAKE_PIN(PORTC, channel), OUTPUT);
        active_channels++;
        timer_start();
    }
}
