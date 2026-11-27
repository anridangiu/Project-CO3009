#ifndef FSM_AUTOMATIC_H
#define FSM_AUTOMATIC_H

#include <stdbool.h>
#include <stdint.h>

#define FSM_AUTOMATIC_DEFAULT_GREEN_MS 5000U
#define FSM_AUTOMATIC_DEFAULT_AMBER_MS 2000U
#define FSM_AUTOMATIC_TIMER_ID 0U

typedef enum {
    AUTO_STOPPED = 0,
    AUTO_RED1_GREEN2,
    AUTO_RED1_AMBER2,
    AUTO_GREEN1_RED2,
    AUTO_AMBER1_RED2
} AutomaticState;

/* Durations are milliseconds; the timer module rounds up to 10 ms ticks. */
bool fsm_automatic_init(uint32_t road1_green_ms,
                        uint32_t road2_green_ms,
                        uint32_t amber_ms);
bool fsm_automatic_set_durations(uint32_t road1_green_ms,
                                 uint32_t road2_green_ms,
                                 uint32_t amber_ms);
bool fsm_automatic_start(void);
void fsm_automatic_run(void); /* Schedule every 10 ms. */
void fsm_automatic_stop(void);
bool fsm_automatic_is_active(void);
AutomaticState fsm_automatic_state(void);

#endif