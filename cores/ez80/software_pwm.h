#pragma once
#include <stdint.h>

void pwm_init(void);

void pwm_write(uint8_t channel, uint8_t duty);

void pwm_disable(uint8_t channel);