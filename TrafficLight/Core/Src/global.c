#include "global.h"

/* Starter values only. RED is the all-red clearance between directions. */
volatile uint16_t redDuration = 1U;
volatile uint16_t yellowDuration = 3U;
volatile uint16_t greenDuration = 10U;

volatile float dht20Temperature = 0.0f;
volatile float dht20Humidity = 0.0f;
volatile uint8_t dht20DataValid = 0U;

uint16_t Global_GetLightDuration(LightType_t light)
{
    switch (light)
    {
        case TIMING_LIGHT_RED:    return redDuration;
        case TIMING_LIGHT_YELLOW: return yellowDuration;
        case TIMING_LIGHT_GREEN:  return greenDuration;
        default:           return 0U;
    }
}

uint8_t Global_SetLightDuration(LightType_t light, uint16_t duration)
{
    if ((duration < LIGHT_TIME_MIN_SEC) || (duration > LIGHT_TIME_MAX_SEC))
        return 0U;

    switch (light)
    {
        case TIMING_LIGHT_RED:    redDuration = duration;    return 1U;
        case TIMING_LIGHT_YELLOW: yellowDuration = duration; return 1U;
        case TIMING_LIGHT_GREEN:  greenDuration = duration;  return 1U;
        default: return 0U;
    }
}
