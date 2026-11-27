#include "software_timer.h"

#include <limits.h>

typedef struct {
    uint32_t remaining_ticks;
    bool active;
    bool expired;
} SoftwareTimer;

static SoftwareTimer timers[SOFTWARE_TIMER_COUNT];

static bool timer_id_valid(uint8_t timer_id)
{
    return timer_id < SOFTWARE_TIMER_COUNT;
}

void software_timer_init(void)
{
    for (uint8_t i = 0U; i < SOFTWARE_TIMER_COUNT; ++i) {
        timers[i].remaining_ticks = 0U;
        timers[i].active = false;
        timers[i].expired = false;
    }
}

bool software_timer_set(uint8_t timer_id, uint32_t duration_ms)
{
    if (!timer_id_valid(timer_id)) {
        return false;
    }

    uint64_t ticks = ((uint64_t)duration_ms + SOFTWARE_TIMER_TICK_MS - 1U)
                   / SOFTWARE_TIMER_TICK_MS;
    if (ticks > UINT32_MAX) {
        return false;
    }

    timers[timer_id].remaining_ticks = (uint32_t)ticks;
    timers[timer_id].active = (ticks > 0U);
    timers[timer_id].expired = (ticks == 0U);
    return true;
}

void software_timer_cancel(uint8_t timer_id)
{
    if (!timer_id_valid(timer_id)) {
        return;
    }
    timers[timer_id].remaining_ticks = 0U;
    timers[timer_id].active = false;
    timers[timer_id].expired = false;
}

void software_timer_tick(void)
{
    for (uint8_t i = 0U; i < SOFTWARE_TIMER_COUNT; ++i) {
        if (timers[i].active && timers[i].remaining_ticks > 0U) {
            --timers[i].remaining_ticks;
            if (timers[i].remaining_ticks == 0U) {
                timers[i].active = false;
                timers[i].expired = true;
            }
        }
    }
}

bool software_timer_take_expired(uint8_t timer_id)
{
    if (!timer_id_valid(timer_id)) {
        return false;
    }

    bool expired = timers[timer_id].expired;
    timers[timer_id].expired = false;
    return expired;
}

uint32_t software_timer_get_remaining_ms(uint8_t timer_id)
{
    if (!timer_id_valid(timer_id) || !timers[timer_id].active) {
        return 0U;
    }

    if (timers[timer_id].remaining_ticks > UINT32_MAX / SOFTWARE_TIMER_TICK_MS) {
        return UINT32_MAX;
    }

    return timers[timer_id].remaining_ticks * SOFTWARE_TIMER_TICK_MS;
}
