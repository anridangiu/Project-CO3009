#ifndef SOFTWARE_TIMER_H
#define SOFTWARE_TIMER_H

#include <stdbool.h>
#include <stdint.h>

#define SOFTWARE_TIMER_TICK_MS 10U
#define SOFTWARE_TIMER_COUNT 8U

void software_timer_init(void);

/* Start/restart a timer. Duration 0 expires immediately. */
bool software_timer_set(uint8_t timer_id, uint32_t duration_ms);
void software_timer_cancel(uint8_t timer_id);

/* Call once per scheduler tick (10 ms). */
void software_timer_tick(void);

/* Returns and clears the one-shot expiration event. */
bool software_timer_take_expired(uint8_t timer_id);
/* Remaining duration for status displays; returns 0 for invalid/inactive timers. */
uint32_t software_timer_get_remaining_ms(uint8_t timer_id);

#endif
