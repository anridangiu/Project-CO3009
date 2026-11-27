#ifndef TRAFFIC_LIGHT_CONFIG_H
#define TRAFFIC_LIGHT_CONFIG_H

#include <stdint.h>
#include "traffic_light.h"

/* Calibrated from the observed A/B output: OFF, RED, AMBER, GREEN. */
static const uint8_t traffic_light_codes[2][LIGHT_COUNT] = {
    {0U, 3U, 2U, 1U}, /* Road 1: D3-D4 */
    {0U, 3U, 2U, 1U}  /* Road 2: D5-D6 */
};

#endif
