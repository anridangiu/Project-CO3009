#include "button.h"
#include "main.h"
#include <string.h>

typedef struct {
    GPIO_TypeDef *port;
    uint16_t pin;
} ButtonPin;

static const ButtonPin pins[BUTTON_COUNT] = {
    [BUTTON_1] = {BTN_1_GPIO_Port, BTN_1_Pin},
    [BUTTON_2] = {BTN_2_GPIO_Port, BTN_2_Pin},
    [BUTTON_3] = {BTN_3_GPIO_Port, BTN_3_Pin},
    [BUTTON_4] = {BTN_4_GPIO_Port, BTN_4_Pin},
    [BUTTON_USER] = {BTN_USER_GPIO_Port, BTN_USER_Pin}
};

typedef struct {
    bool pressed;              /* debounced state */
    uint8_t different_samples; /* consecutive samples that differ from it */
    uint8_t held_ticks;        /* how long it has been pressed, saturates */
    bool press_event;
    bool long_event;
} ButtonState;

static ButtonState buttons[BUTTON_COUNT];
static uint8_t high_mask;

void button_init(uint8_t active_high_mask)
{
    high_mask = active_high_mask;
    memset(buttons, 0, sizeof(buttons));
}

void button_scan(void)
{
    for (unsigned i = 0; i < BUTTON_COUNT; ++i) {
        ButtonState *s = &buttons[i];
        bool high = HAL_GPIO_ReadPin(pins[i].port, pins[i].pin) == GPIO_PIN_SET;
        bool pressed = high == ((high_mask & (1U << i)) != 0U);
        if (pressed != s->pressed) {
            /* Accept the new level only after it has been stable long enough. */
            if (++s->different_samples == BUTTON_DEBOUNCE_SAMPLES) {
                s->pressed = pressed;
                s->different_samples = 0;
                s->held_ticks = 0;
                if (pressed) s->press_event = true;
            }
        } else {
            s->different_samples = 0;
            if (pressed && s->held_ticks < BUTTON_LONG_PRESS_TICKS &&
                ++s->held_ticks == BUTTON_LONG_PRESS_TICKS) {
                s->long_event = true;
            }
        }
    }
}

bool button_take_press(ButtonId id)
{
    if ((unsigned)id >= BUTTON_COUNT) return false;
    bool value = buttons[id].press_event;
    buttons[id].press_event = false;
    return value;
}

bool button_take_long(ButtonId id)
{
    if ((unsigned)id >= BUTTON_COUNT) return false;
    bool value = buttons[id].long_event;
    buttons[id].long_event = false;
    return value;
}

void button_flush(void)
{
    for (unsigned i = 0; i < BUTTON_COUNT; ++i) {
        buttons[i].press_event = false;
        buttons[i].long_event = false;
    }
}
