#ifndef TRAFFIC_LIGHT_CONFIG_H
#define TRAFFIC_LIGHT_CONFIG_H

#include <stdint.h>
#include "traffic_light.h"

/*
 * TEMPORARY PLACEHOLDER: replace these two rows with the measured A/B codes
 * for OFF, RED, AMBER, GREEN before running the lights on the board.
 * Code bits: A is bit 1, B is bit 0.
 */
static const uint8_t traffic_light_codes[2][LIGHT_COUNT] = {
    {0U, 1U, 2U, 3U},
    {0U, 1U, 2U, 3U}
};

#endif