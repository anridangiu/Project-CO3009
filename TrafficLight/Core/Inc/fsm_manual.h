#ifndef FSM_MANUAL_H
#define FSM_MANUAL_H

#include <stdbool.h>
#include <stdint.h>
#include "button.h"

#define FSM_MANUAL_TICK_MS 10U                 /* period of fsm_manual_run() */
#define FSM_MANUAL_SWITCH_BUTTON BUTTON_2      /* button that switches the lane */
#define FSM_MANUAL_DEFAULT_AMBER_MS 2000U

typedef enum {
    MAN_GREEN1_RED2, /* stable: road 1 Green, road 2 Red   -> LCD "L1:GRN L2:RED" */
    MAN_SWITCH_TO_2, /* timed:  both Amber, then road 2 Green -> LCD "SWITCHING LANE.." */
    MAN_RED1_GREEN2, /* stable: road 1 Red, road 2 Green   -> LCD "L1:RED L2:GRN" */
    MAN_SWITCH_TO_1  /* timed:  both Amber, then road 1 Green */
} ManualState;

/* Call once when the system switches to manual mode.
   amber_ms = Amber duration used while switching lanes (0 = default 2 s). */
bool fsm_manual_enter(uint32_t amber_ms);
void fsm_manual_run(void); /* Schedule once per 10 ms. */
void fsm_manual_exit(void);

/* For the LCD task. */
bool fsm_manual_is_active(void);
ManualState fsm_manual_state(void);
bool fsm_manual_is_switching(void);
uint32_t fsm_manual_wait_seconds(void); /* "YELLOW WAIT" seconds left, rounded up; 0 when stable */

#endif
