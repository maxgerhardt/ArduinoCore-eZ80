#pragma once

#include <stdint.h>

extern const uint8_t num_to_bitmask_table[8];

#define BIT(n) num_to_bitmask_table[n]