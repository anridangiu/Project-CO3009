#include "fsm_manual.h"
#include "button.h"
#include "traffic_light.h"

typedef enum {
    MAN_R1_G2,
    MAN_R1_A2,
    MAN_G1_R2,
    MAN_A1_R2,
    MAN_ALL_AMBER,
    MAN_COUNT
} ManualPhase;

static const LightColor lights[MAN_COUNT][2] = {
    [MAN_R1_G2] = {LIGHT_RED,   LIGHT_GREEN},
    [MAN_R1_A2] = {LIGHT_RED,   LIGHT_AMBER},
    [MAN_G1_R2] = {LIGHT_GREEN, LIGHT_RED},
    [MAN_A1_R2] = {LIGHT_AMBER, LIGHT_RED},
    [MAN_ALL_AMBER] = {LIGHT_AMBER, LIGHT_AMBER},
};

static bool active;
static ManualPhase phase;

bool fsm_manual_enter(void)
{
    (void)button_take_press(BUTTON_2);   /* bỏ lần nhấn còn sót */
    phase  = MAN_R1_G2;
    active = traffic_set_pair(lights[phase][0], lights[phase][1]);
    return active;
}

void fsm_manual_run(void)
{
    if (!active || !button_take_press(BUTTON_2)) return;
    ManualPhase next = (ManualPhase)((phase + 1) % MAN_COUNT);
    if (traffic_set_pair(lights[next][0], lights[next][1])) phase = next;
}

void fsm_manual_exit(void)
{
    if (!active) return;
    active = false;
    (void)traffic_set_pair(LIGHT_RED, LIGHT_RED);
}
