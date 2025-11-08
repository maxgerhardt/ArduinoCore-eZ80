#pragma once

//==============================================================
// Bit masks for TMRx_CTL (from product spec)
//==============================================================

enum {
    TMR_CTL_PRT_EN     = (1 << 0),  // Enable timer
    TMR_CTL_RST_EN     = (1 << 1),  // Auto-reload enable
    TMR_CTL_CLKDIV_4   = (0 << 2),  // input clock dividers
    TMR_CTL_CLKDIV_16  = (1 << 2),
    TMR_CTL_CLKDIV_64  = (2 << 2),
    TMR_CTL_CLKDIV_256 = (3 << 2),
    TMR_CTL_MODE_SP    = (0 << 4),  // single pass mode
    TMR_CTL_MODE_CONT  = (1 << 4),  // Continuous mode
    TMR_CTL_IRQ_EN     = (1 << 6),  // Interrupt enable
    TMR_CTL_PRT_IRQ    = (1 << 7)   // timer has reached end of count? (readonly)
};
