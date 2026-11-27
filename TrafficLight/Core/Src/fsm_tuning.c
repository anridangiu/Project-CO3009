#include "fsm_tuning.h"
#include "button.h"
#include "lcd_i2c.h"
#include "traffic_light.h"
#include <stdio.h>

/* Button allocation agreed for this FSM:
 * BUTTON_1    : select Red -> Amber -> Green
 * BUTTON_2    : exit to Automatic (handled by main's mode controller)
 * BUTTON_3    : increase duration
 * BUTTON_4    : decrease duration
 * BUTTON_USER : confirm/save the edited duration
 */

static TuningState tuning_state = TUNING_IDLE;
static LightType_t selected_light = TIMING_LIGHT_RED;
static uint16_t draft_duration = 0U;
static bool display_dirty = true;
static bool just_saved = false;

static bool preview_selected_light(LightType_t light)
{
    switch (light) {
        case TIMING_LIGHT_RED:
            return traffic_set_pair(LIGHT_RED, LIGHT_RED);
        case TIMING_LIGHT_YELLOW:
            return traffic_set_pair(LIGHT_AMBER, LIGHT_AMBER);
        case TIMING_LIGHT_GREEN:
            /* Keep the intersection safe: preview green on road 1 only. */
            return traffic_set_pair(LIGHT_GREEN, LIGHT_RED);
        default:
            return false;
    }
}

static const char *light_name(LightType_t light)
{
    switch (light) {
        case TIMING_LIGHT_RED:    return "RED";
        case TIMING_LIGHT_YELLOW: return "AMBER";
        case TIMING_LIGHT_GREEN:  return "GREEN";
        default:           return "UNKNOWN";
    }
}

static void display_active(void)
{
    char line[17];

    (void)LCD_Clear();
    (void)snprintf(line, sizeof(line), "SET %s", light_name(selected_light));
    (void)LCD_PrintAt(0U, 0U, line);

    if (just_saved) {
        (void)snprintf(line, sizeof(line), "SAVED: %3u sec", (unsigned int)draft_duration);
    } else {
        (void)snprintf(line, sizeof(line), "TIME:  %3u sec", (unsigned int)draft_duration);
    }
    (void)LCD_PrintAt(0U, 1U, line);
}

bool fsm_tuning_enter(void)
{
    selected_light = TIMING_LIGHT_RED;
    draft_duration = Global_GetLightDuration(selected_light);
    just_saved = false;
    display_dirty = true;
    button_flush();
    if (!preview_selected_light(selected_light)) {
        return false;
    }
    tuning_state = TUNING_ACTIVE;
    return true;
}

void fsm_tuning_run(void)
{
    bool select_pressed;
    bool increase_pressed;
    bool decrease_pressed;
    bool confirm_pressed;

    if (tuning_state != TUNING_ACTIVE) return;

    /* BUTTON_2 exits through the mode controller; BUTTON_1 selects here. */
    select_pressed = button_take_press(BUTTON_1);
    increase_pressed = button_take_press(BUTTON_3);
    decrease_pressed = button_take_press(BUTTON_4);
    confirm_pressed = button_take_press(BUTTON_USER);

    if (select_pressed) {
        LightType_t next_light =
            (LightType_t)((selected_light + 1U) % TIMING_LIGHT_COUNT);
        if (preview_selected_light(next_light)) {
            selected_light = next_light;
            draft_duration = Global_GetLightDuration(selected_light);
            just_saved = false;
            display_dirty = true;
        }
    }

    if (increase_pressed) {
        if (draft_duration <= (LIGHT_TIME_MAX_SEC - LIGHT_TIME_STEP_SEC)) {
            draft_duration = (uint16_t)(draft_duration + LIGHT_TIME_STEP_SEC);
        }
        just_saved = false;
        display_dirty = true;
    }

    if (decrease_pressed) {
        if (draft_duration >= (LIGHT_TIME_MIN_SEC + LIGHT_TIME_STEP_SEC)) {
            draft_duration = (uint16_t)(draft_duration - LIGHT_TIME_STEP_SEC);
        }
        just_saved = false;
        display_dirty = true;
    }

    if (confirm_pressed) {
        /* Global_SetLightDuration validates the configured min/max range. */
        if (Global_SetLightDuration(selected_light, draft_duration) != 0U) {
            just_saved = true;
        } else {
            just_saved = false;
        }
        display_dirty = true;
    }

    if (display_dirty) {
        display_active();
        display_dirty = false;
    }
}

void fsm_tuning_exit(void)
{
    if (tuning_state == TUNING_IDLE) return;

    tuning_state = TUNING_IDLE;
    just_saved = false;
    display_dirty = true;
    button_flush();
}

TuningState fsm_tuning_state(void)
{
    return tuning_state;
}

LightType_t fsm_tuning_selected_light(void)
{
    return selected_light;
}

uint16_t fsm_tuning_draft_duration(void)
{
    return draft_duration;
}
