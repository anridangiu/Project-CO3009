#include "fsm_tuning.h"
#include "button.h"
#include "lcd_i2c.h"
#include <stdio.h>

/* Button allocation agreed for this FSM:
 * BUTTON_1    : select Red -> Yellow -> Green
 * BUTTON_3    : increase duration
 * BUTTON_4    : decrease duration
 * BUTTON_USER : confirm/save the edited duration
 * BUTTON_2 is reserved by fsm_manual for lane switching.
 */

static TuningState tuning_state = TUNING_IDLE;
static LightType_t selected_light = LIGHT_RED;
static uint16_t draft_duration = 0U;
static bool display_dirty = true;
static bool just_saved = false;

static const char *light_name(LightType_t light)
{
    switch (light) {
        case LIGHT_RED:    return "RED";
        case LIGHT_YELLOW: return "YELLOW";
        case LIGHT_GREEN:  return "GREEN";
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
    selected_light = LIGHT_RED;
    draft_duration = Global_GetLightDuration(selected_light);
    just_saved = false;
    display_dirty = true;
    button_flush();
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

    /* Consume each event once. BUTTON_2 belongs to Manual Mode. */
    select_pressed = button_take_press(BUTTON_1);
    increase_pressed = button_take_press(BUTTON_3);
    decrease_pressed = button_take_press(BUTTON_4);
    confirm_pressed = button_take_press(BUTTON_USER);

    if (select_pressed) {
        selected_light = (LightType_t)((selected_light + 1U) % LIGHT_COUNT);
        draft_duration = Global_GetLightDuration(selected_light);
        just_saved = false;
        display_dirty = true;
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
