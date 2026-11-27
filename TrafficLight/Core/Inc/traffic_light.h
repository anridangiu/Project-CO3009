#ifndef TRAFFIC_LIGHT_H
#define TRAFFIC_LIGHT_H

#include <stdbool.h>
#include <stdint.h>

typedef enum { LIGHT_OFF, LIGHT_RED, LIGHT_AMBER, LIGHT_GREEN, LIGHT_COUNT } LightColor;

/* Each row gives the measured codes for OFF, RED, AMBER, GREEN, in that order.
   A is the high bit. B is the low bit. Each row is a permutation of 0..3. */
bool traffic_init(const uint8_t codes[2][LIGHT_COUNT]);
bool traffic_set_pair(LightColor road1, LightColor road2);
bool traffic_set(unsigned road, LightColor color);
/* Colour currently shown on road 1 or 2 (LIGHT_OFF for any other road). */
LightColor traffic_get(unsigned road);

#endif
