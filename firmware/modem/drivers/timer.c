/*============================================================================
 * drivers/timer.c — 1 ms system tick and timeout helpers
 *
 * Implementation note:
 *   millis() reads a 32-bit counter incremented in ISR. On the 8-bit PIC18,
 *   the read is non-atomic, so we must protect it. We save the GIE bit, mask
 *   interrupts, copy the counter, then restore GIE -- this is safe whether
 *   the caller already had interrupts disabled or not.
 *===========================================================================*/
#include "timer.h"
#include "../board/board.h"

static volatile uint32_t tick_count_ = 0;

void timer_init(void)
{
    tick_count_ = 0;
    board_timer_hw_init();
}

uint32_t millis(void)
{
    uint32_t t;
    uint8_t  ie;

    ie = board_int_save_disable();
    t  = tick_count_;
    board_int_restore(ie);
    return t;
}

uint32_t deadline_from_now(uint32_t ms)
{
    return millis() + ms;
}

bool deadline_expired(uint32_t deadline)
{
    /* Handles single 32-bit wrap correctly as long as timeouts < ~24 days. */
    return ((int32_t)(millis() - deadline) >= 0);
}

void isr_timer_tick_1ms(void)
{
    tick_count_++;
}
