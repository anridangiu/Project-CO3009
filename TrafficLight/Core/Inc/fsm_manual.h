#ifndef FSM_MANUAL_H
#define FSM_MANUAL_H

#include <stdbool.h>
#include <stdint.h>
#include "button.h"

#define FSM_MANUAL_TICK_MS 10U
#define FSM_MANUAL_SWITCH_BUTTON BUTTON_2

bool fsm_manual_enter(void);
void fsm_manual_run(void);
void fsm_manual_exit(void);

#endif
