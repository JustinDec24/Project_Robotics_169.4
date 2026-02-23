/*============================================================================
 * drivers/timer.c — 1 ms system tick and timeout helpers
 *===========================================================================*/
#include "timer.h"
#include "../board/board.h"

static volatile uint32_t tick_count = 0;

void timer_init(void)
{
    tick_count = 0;
    board_timer_hw_init();
}

uint32_t millis(void)
{
    uint32_t t;
    global_int_disable();
    t = tick_count;
    global_int_enable();
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
    tick_count++;
}
