#ifndef GLOBAL_H
#define GLOBAL_H

#include "main.h"
#include <stdint.h>

/*
 * Shared traffic-light timing and sensor data.
 * Units: seconds for durations, degrees Celsius and percent RH for DHT20.
 * Initial durations are starter values; align them with the group's traffic FSM.
 */

#define LIGHT_TIME_MIN_SEC       1U
#define LIGHT_TIME_MAX_SEC       120U
#define LIGHT_TIME_STEP_SEC      1U

typedef enum
{
    TIMING_LIGHT_RED = 0,
    TIMING_LIGHT_YELLOW,
    TIMING_LIGHT_GREEN,
    TIMING_LIGHT_COUNT
} LightType_t;

/* Shared configured durations in seconds. RED means the both-directions-red
 * clearance interval; each road's full red time also includes the other
 * road's green and yellow phases. */
extern volatile uint16_t redDuration;
extern volatile uint16_t yellowDuration;
extern volatile uint16_t greenDuration;

/* Latest valid environmental readings. */
extern volatile float dht20Temperature;
extern volatile float dht20Humidity;
extern volatile uint8_t dht20DataValid;

/* Read/write helpers used by the tuning FSM. */
uint16_t Global_GetLightDuration(LightType_t light);
uint8_t Global_SetLightDuration(LightType_t light, uint16_t duration);

#endif /* GLOBAL_H */
