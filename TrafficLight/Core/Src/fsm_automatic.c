#include "fsm_automatic.h"

#include "software_timer.h"
#include "traffic_light.h"
#include <stddef.h>

static uint32_t road1_green_ms;
static uint32_t road2_green_ms;
static uint32_t amber_ms;
static uint32_t all_red_ms = FSM_AUTOMATIC_DEFAULT_ALL_RED_MS;
static AutomaticState state = AUTO_STOPPED;
static bool initialized;
static bool active;

static bool duration_valid(uint32_t duration_ms)
{
    uint64_t ticks = ((uint64_t)duration_ms + SOFTWARE_TIMER_TICK_MS - 1U)
                   / SOFTWARE_TIMER_TICK_MS;
    return duration_ms > 0U && ticks <= UINT32_MAX;
}

bool fsm_automatic_set_durations(uint32_t road1_ms,
                                 uint32_t road2_ms,
                                 uint32_t amber_duration_ms)
{
    if (!duration_valid(road1_ms) || !duration_valid(road2_ms) ||
        !duration_valid(amber_duration_ms)) {
        return false;
    }

    road1_green_ms = road1_ms;
    road2_green_ms = road2_ms;
    amber_ms = amber_duration_ms;
    return true;
}

bool fsm_automatic_set_all_red_duration(uint32_t duration_ms)
{
    if (!duration_valid(duration_ms)) {
        return false;
    }

    all_red_ms = duration_ms;
    return true;
}

bool fsm_automatic_init(uint32_t road1_ms,
                        uint32_t road2_ms,
                        uint32_t amber_duration_ms)
{
    active = false;
    initialized = false;
    state = AUTO_STOPPED;
    all_red_ms = FSM_AUTOMATIC_DEFAULT_ALL_RED_MS;
    software_timer_cancel(FSM_AUTOMATIC_TIMER_ID);

    if (!fsm_automatic_set_durations(road1_ms, road2_ms, amber_duration_ms)) {
        return false;
    }

    initialized = true;
    return true;
}

static bool enter_phase(AutomaticState next_state,
                        LightColor road1,
                        LightColor road2,
                        uint32_t duration_ms)
{
    if (!traffic_set_pair(road1, road2)) {
        software_timer_cancel(FSM_AUTOMATIC_TIMER_ID);
        active = false;
        state = AUTO_STOPPED;
        return false;
    }

    if (!software_timer_set(FSM_AUTOMATIC_TIMER_ID, duration_ms)) {
        software_timer_cancel(FSM_AUTOMATIC_TIMER_ID);
        active = false;
        state = AUTO_STOPPED;
        return false;
    }

    state = next_state;
    active = true;
    return true;
}

bool fsm_automatic_start(void)
{
    if (!initialized) {
        return false;
    }
    if (active) {
        return true;
    }

    return enter_phase(AUTO_RED1_GREEN2, LIGHT_RED, LIGHT_GREEN, road2_green_ms);
}

void fsm_automatic_run(void)
{
    if (!active || !software_timer_take_expired(FSM_AUTOMATIC_TIMER_ID)) {
        return;
    }

    switch (state) {
    case AUTO_RED1_GREEN2:
        (void)enter_phase(AUTO_RED1_AMBER2, LIGHT_RED, LIGHT_AMBER, amber_ms);
        break;
    case AUTO_RED1_AMBER2:
        (void)enter_phase(AUTO_ALL_RED_AFTER_ROAD2,
                          LIGHT_RED, LIGHT_RED, all_red_ms);
        break;
    case AUTO_ALL_RED_AFTER_ROAD2:
        (void)enter_phase(AUTO_GREEN1_RED2,
                          LIGHT_GREEN, LIGHT_RED, road1_green_ms);
        break;
    case AUTO_GREEN1_RED2:
        (void)enter_phase(AUTO_AMBER1_RED2,
                          LIGHT_AMBER, LIGHT_RED, amber_ms);
        break;
    case AUTO_AMBER1_RED2:
        (void)enter_phase(AUTO_ALL_RED_AFTER_ROAD1,
                          LIGHT_RED, LIGHT_RED, all_red_ms);
        break;
    case AUTO_ALL_RED_AFTER_ROAD1:
        (void)enter_phase(AUTO_RED1_GREEN2,
                          LIGHT_RED, LIGHT_GREEN, road2_green_ms);
        break;
    default:
        fsm_automatic_stop();
        break;
    }
}

void fsm_automatic_stop(void)
{
    software_timer_cancel(FSM_AUTOMATIC_TIMER_ID);
    active = false;
    state = AUTO_STOPPED;
    /* Leave the current lights unchanged; the next mode owns the transition. */
}

bool fsm_automatic_is_active(void)
{
    return active;
}

AutomaticState fsm_automatic_state(void)
{
    return state;
}

void fsm_automatic_get_road_remaining_ms(uint32_t *road1_ms,
                                         uint32_t *road2_ms)
{
    uint32_t phase_ms = active
                      ? software_timer_get_remaining_ms(FSM_AUTOMATIC_TIMER_ID)
                      : 0U;

    if (road1_ms == NULL || road2_ms == NULL) {
        return;
    }

    *road1_ms = 0U;
    *road2_ms = 0U;

    switch (state) {
    case AUTO_RED1_GREEN2:
        *road1_ms = phase_ms + amber_ms + all_red_ms;
        *road2_ms = phase_ms;
        break;
    case AUTO_RED1_AMBER2:
        *road1_ms = phase_ms + all_red_ms;
        *road2_ms = phase_ms;
        break;
    case AUTO_ALL_RED_AFTER_ROAD2:
        *road1_ms = phase_ms;
        *road2_ms = phase_ms + road1_green_ms + amber_ms + all_red_ms;
        break;
    case AUTO_GREEN1_RED2:
        *road1_ms = phase_ms;
        *road2_ms = phase_ms + amber_ms + all_red_ms;
        break;
    case AUTO_AMBER1_RED2:
        *road1_ms = phase_ms;
        *road2_ms = phase_ms + all_red_ms;
        break;
    case AUTO_ALL_RED_AFTER_ROAD1:
        *road1_ms = phase_ms + road2_green_ms + amber_ms + all_red_ms;
        *road2_ms = phase_ms;
        break;
    default:
        break;
    }
}
