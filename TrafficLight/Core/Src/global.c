#include "global.h"

/* Starter values only. Replace with the values agreed by the group. */
volatile uint16_t redDuration = 10U;
volatile uint16_t yellowDuration = 3U;
volatile uint16_t greenDuration = 10U;

volatile float dht20Temperature = 0.0f;
volatile float dht20Humidity = 0.0f;
volatile uint8_t dht20DataValid = 0U;

uint16_t Global_GetLightDuration(LightType_t light)
{
    switch (light)
    {
        case LIGHT_RED:    return redDuration;
        case LIGHT_YELLOW: return yellowDuration;
        case LIGHT_GREEN:  return greenDuration;
        default:           return 0U;
    }
}

uint8_t Global_SetLightDuration(LightType_t light, uint16_t duration)
{
    if ((duration < LIGHT_TIME_MIN_SEC) || (duration > LIGHT_TIME_MAX_SEC))
        return 0U;

    switch (light)
    {
        case LIGHT_RED:    redDuration = duration;    return 1U;
        case LIGHT_YELLOW: yellowDuration = duration; return 1U;
        case LIGHT_GREEN:  greenDuration = duration;  return 1U;
        default: return 0U;
    }
}
