#ifndef FSM_TUNING_H
#define FSM_TUNING_H

#include <stdbool.h>
#include <stdint.h>
#include "global.h"

/* Timing-adjustment FSM. The mode controller calls enter/exit when the
 * system changes mode, and schedules fsm_tuning_run() periodically.
 * button_scan() is owned by the shared Button task; do not call it here.
 */
typedef enum {
    TUNING_IDLE = 0,
    TUNING_ACTIVE
} TuningState;

/* Enter Timing Adjustment Mode. Returns true when initialized. */
bool fsm_tuning_enter(void);

/* Run periodically as a scheduler task while Timing Adjustment Mode is active. */
void fsm_tuning_run(void);

/* Exit Timing Adjustment Mode. */
void fsm_tuning_exit(void);

TuningState fsm_tuning_state(void);
LightType_t fsm_tuning_selected_light(void);
uint16_t fsm_tuning_draft_duration(void);

#endif /* FSM_TUNING_H */
