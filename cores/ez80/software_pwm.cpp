#include <Arduino.h>
#include <stdint.h>
#include <string.h>
#include "ez80f92.h"
#include "ez80f92_peripherals.h"
#include "vectors.h"
#include "pins_api.h"

//==============================================================
// Configuration
//==============================================================
#define MAX_PWM_CHANNELS 8
#define PWM_RESOLUTION   256
//#define PWM_BASE_FREQ_HZ 60
#define PWM_BASE_FREQ_HZ 1125
//#define PWM_BASE_FREQ_HZ (8*2250)
#define MAX_EVENTS       (MAX_PWM_CHANNELS + 1)

#define PWM_PORT_DR      IO(PC_DR)

//==============================================================
// Timer helpers
//==============================================================
static inline uint16_t ticks_per_step(void) {
    const uint32_t divider = 64;
    return (uint16_t)((F_CPU / divider) / (PWM_BASE_FREQ_HZ * PWM_RESOLUTION));
}

static inline void timer_arm_oneshot(uint16_t ticks) {
    IO(TMR1_RR_L) = (uint8_t)(ticks & 0xFF);
    IO(TMR1_RR_H) = (uint8_t)(ticks >> 8);
    IO(TMR1_CTL) = TMR_CTL_MODE_SP | TMR_CTL_RST_EN |
                   TMR_CTL_CLKDIV_64 |
                   TMR_CTL_IRQ_EN |
                   TMR_CTL_PRT_EN;
}

static bool timerRunning = false;
static inline void timer_stop(void) { IO(TMR1_CTL) = 0x00; timerRunning = false;}

//==============================================================
// Structures and globals
//==============================================================
typedef struct {
    uint8_t set_mask;
    uint8_t inv_clr_mask;
    uint16_t ticks;
} pwm_event_t;

static pwm_event_t schedule_a[MAX_EVENTS];
static pwm_event_t schedule_b[MAX_EVENTS];
static pwm_event_t *active_schedule = schedule_a;
static pwm_event_t *build_schedule = schedule_b;

static uint8_t active_schedule_count = 0;
static uint8_t build_schedule_count = 0;
static uint8_t current_event = 0;
static uint8_t pwm_duties[MAX_PWM_CHANNELS] = {0};
static uint8_t pwm_active_mask = 0;

// Single synchronization flag: 0 = no wait needed, 1 = waiting for cycle completion
static volatile uint8_t schedule_dirty = 0;

static const uint8_t channel_masks[8] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};

//==============================================================
// ISR - Optimized
//==============================================================
__attribute__((interrupt))
void PRT1_Handler(void) {
    IO(TMR1_CTL); // Clear interrupt flag
    
    // Execute the current event
    const pwm_event_t ev = active_schedule[current_event];
    PWM_PORT_DR = (PWM_PORT_DR & ev.inv_clr_mask) | ev.set_mask;

    // Move to next event
    current_event++;
    if (current_event >= active_schedule_count) {
        current_event = 0;
        // Cycle completed - Apply new schedule if available
        if (schedule_dirty) {
            pwm_event_t *temp = active_schedule;
            active_schedule = build_schedule;
            build_schedule = temp;
            active_schedule_count = build_schedule_count;
            schedule_dirty = 0;
        }
    }

    // Schedule timer using the ticks from the event we just executed
    timer_arm_oneshot(ev.ticks);
}

//==============================================================
// Event table builder
//==============================================================
static void rebuild_schedule(void) {
    uint8_t count = 0;
    uint8_t prev_step = 0;
    const uint16_t step_ticks = ticks_per_step();
    
    // Always build into a local buffer first
    static pwm_event_t local_schedule[MAX_EVENTS];
    
    if (pwm_active_mask == 0) {
        build_schedule_count = 0;
        schedule_dirty = 1;  // Now ISR can see it
        return;
    }

    // Build into local buffer
    // Event 0: Turn ON all active channels at step 0
    local_schedule[count].set_mask = pwm_active_mask;
    local_schedule[count].inv_clr_mask = (uint8_t)~0;
    local_schedule[count].ticks = 0;
    count++;

    // Scan steps 1-254
    for (uint8_t step = 1; step < 255; step++) {
        uint8_t clear_mask = 0;
        
        for (uint8_t ch = 0; ch < MAX_PWM_CHANNELS; ch++) {
            if ((pwm_active_mask & channel_masks[ch]) && (pwm_duties[ch] == step)) {
                clear_mask |= channel_masks[ch];
            }
        }
        
        if (clear_mask != 0) {
            uint16_t delta_ticks = (step - prev_step) * step_ticks;
            local_schedule[count-1].ticks = delta_ticks;
            
            local_schedule[count].set_mask = 0;
            local_schedule[count].inv_clr_mask = (uint8_t)~clear_mask;
            local_schedule[count].ticks = 0;
            count++;
            
            prev_step = step;
        }
    }
    
    // Handle final wrap-around
    uint16_t final_ticks = (255 - prev_step) * step_ticks;
    local_schedule[count-1].ticks = final_ticks;

    // Atomic update: copy to build_schedule and set dirty flag
    schedule_dirty = 0; // we are temporarily modifying the next outstanding buffer 
    memcpy(build_schedule, local_schedule, count * sizeof(pwm_event_t));
    build_schedule_count = count;
    schedule_dirty = 1;  // ISR can now safely swap this in
}

//==============================================================
// API
//==============================================================
void pwm_init(void) {
    timer_stop();
    _set_vector(VECTOR_PRT_1, PRT1_Handler);
    schedule_dirty = 0;
    __asm__("ei");
}

void pwm_write(uint8_t ch, uint8_t duty) {
    uint8_t mask = channel_masks[ch];
    
    // Handle 0% duty cycle - take out of PWM control
    if (duty == 0) {
        // Remove from active mask and rebuild schedule WITHOUT this channel
        pwm_active_mask &= ~mask;
        pwm_duties[ch] = 0;
        rebuild_schedule();
        if (timerRunning)
        while (schedule_dirty) {
            // Wait for current cycle to finish (ISR will clear dirty flag)
        }
        if(timerRunning && pwm_active_mask == 0)
            timer_stop();
        // Now safely set pin low (ISR is no longer controlling this channel)
        PWM_PORT_DR &= ~mask;
        return;
    }
    
    // Handle 100% duty cycle - take out of PWM control  
    if (duty == 255) {
        // Remove from active mask and rebuild schedule WITHOUT this channel
        pwm_active_mask &= ~mask;
        pwm_duties[ch] = 0;
        rebuild_schedule();
        // Wait for ISR to finish current cycle with the new schedule
        if (timerRunning)
        while (schedule_dirty) {
            // Wait for current cycle to finish (ISR will clear dirty flag)
        }
        if(timerRunning && pwm_active_mask == 0)
            timer_stop();
        // Now safely set pin high (ISR is no longer controlling this channel)
        PWM_PORT_DR |= mask;
        return;
    }
    
    // Normal PWM duty cycle - no need to wait for cycle completion
    pwm_duties[ch] = duty;
    // Add to active mask if not already
    pwm_active_mask |= mask;
    rebuild_schedule();
    // start the timer if we need it
    if (!timerRunning) {
        PWM_PORT_DR |= pwm_active_mask;
        current_event = 1;
        timer_arm_oneshot(active_schedule[0].ticks);
        timerRunning = true;
    }
}