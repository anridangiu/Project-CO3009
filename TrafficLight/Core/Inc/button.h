#ifndef BUTTON_H
#define BUTTON_H

#include <stdbool.h>
#include <stdint.h>

typedef enum { BUTTON_1, BUTTON_2, BUTTON_3, BUTTON_4, BUTTON_USER, BUTTON_COUNT } ButtonId;

#define BUTTON_DEBOUNCE_SAMPLES 3U   /* 3 x 10 ms of a stable new level */
#define BUTTON_LONG_PRESS_TICKS 100U /* held for 100 x 10 ms = 1 s */

/* Captures each released input level at startup, so either active polarity works. */
void button_init(void);
void button_scan(void); /* Schedule once per 10 ms. */
/* Each event is reported once: the call returns it and clears it. */
bool button_take_press(ButtonId id);
bool button_take_long(ButtonId id);
/* Drop every pending press / long-press event. Call when the mode changes,
   so that a press made in the old mode is not handled by the new one. */
void button_flush(void);

#endif
