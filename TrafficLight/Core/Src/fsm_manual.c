#include "fsm_manual.h"
#include "traffic_light.h"

/* Manual mode: one road is Green and the other Red until the user presses the
   switch button. Both roads then show Amber for the Amber duration and the
   Green moves to the other road:

   GREEN1_RED2 --button--> SWITCH_TO_2 --timeout--> RED1_GREEN2
   RED1_GREEN2 --button--> SWITCH_TO_1 --timeout--> GREEN1_RED2            */

static bool active;
static ManualState state;
static uint32_t amber_ticks;
static uint32_t ticks_left;

bool fsm_manual_enter(uint32_t amber_ms)
{
    if (amber_ms == 0U) amber_ms = FSM_MANUAL_DEFAULT_AMBER_MS;
    amber_ticks = (amber_ms + FSM_MANUAL_TICK_MS - 1U) / FSM_MANUAL_TICK_MS;
    ticks_left = 0;
    button_flush(); /* presses made in the previous mode do not count here */

    /* Keep the road that is Green right now; otherwise start with road 1. */
    if (traffic_get(2U) == LIGHT_GREEN) {
        state = MAN_RED1_GREEN2;
        active = traffic_set_pair(LIGHT_RED, LIGHT_GREEN);
    } else {
        state = MAN_GREEN1_RED2;
        active = traffic_set_pair(LIGHT_GREEN, LIGHT_RED);
    }
    return active;
}

static void start_switch(ManualState next)
{
    if (!traffic_set_pair(LIGHT_AMBER, LIGHT_AMBER)) return;
    state = next;
    ticks_left = amber_ticks;
}

static void finish_switch(ManualState next, LightColor road1, LightColor road2)
{
    if (ticks_left > 0U) --ticks_left;
    if (ticks_left == 0U && traffic_set_pair(road1, road2)) state = next;
}

void fsm_manual_run(void)
{
    if (!active) return;
    /* Read the event in every state: a press during the Amber wait is dropped,
       it must not fire again as soon as the switch is finished. */
    bool pressed = button_take_press(FSM_MANUAL_SWITCH_BUTTON);

    switch (state) {
    case MAN_GREEN1_RED2:
        if (pressed) start_switch(MAN_SWITCH_TO_2);
        break;
    case MAN_SWITCH_TO_2:
        finish_switch(MAN_RED1_GREEN2, LIGHT_RED, LIGHT_GREEN);
        break;
    case MAN_RED1_GREEN2:
        if (pressed) start_switch(MAN_SWITCH_TO_1);
        break;
    case MAN_SWITCH_TO_1:
        finish_switch(MAN_GREEN1_RED2, LIGHT_GREEN, LIGHT_RED);
        break;
    default:
        break;
    }
}

void fsm_manual_exit(void)
{
    if (!active) return;
    active = false;
    ticks_left = 0;
    (void)traffic_set_pair(LIGHT_RED, LIGHT_RED);
}

bool fsm_manual_is_active(void) { return active; }

ManualState fsm_manual_state(void) { return state; }

bool fsm_manual_is_switching(void)
{
    return active && (state == MAN_SWITCH_TO_1 || state == MAN_SWITCH_TO_2);
}

uint32_t fsm_manual_wait_seconds(void)
{
    if (!fsm_manual_is_switching()) return 0U;
    return (ticks_left * FSM_MANUAL_TICK_MS + 999U) / 1000U;
}
