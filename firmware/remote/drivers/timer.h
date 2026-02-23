/*============================================================================
 * drivers/timer.h — 1 ms system tick and timeout helpers
 *===========================================================================*/
#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>
#include <stdbool.h>

void     timer_init(void);
uint32_t millis(void);

/* Deadline-based timeout helpers.
 * Usage:
 *   uint32_t deadline = deadline_from_now(200);
 *   while (!deadline_expired(deadline)) { ... }
 */
uint32_t deadline_from_now(uint32_t ms);
bool     deadline_expired(uint32_t deadline);

/* Called from ISR — increments the millisecond counter. */
void isr_timer_tick_1ms(void);

#endif /* TIMER_H */
