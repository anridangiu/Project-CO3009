#include "traffic_light.h"
#include "main.h"
#include <string.h>

static uint8_t color_code[2][LIGHT_COUNT];
static LightColor current[2];
static bool ready;

static uint32_t select_pins(uint8_t code, uint16_t pin_a, uint16_t pin_b)
{
    uint32_t selected = 0;
    if (code & 2U) selected |= pin_a;
    if (code & 1U) selected |= pin_b;
    return selected;
}

bool traffic_set_pair(LightColor road1, LightColor road2)
{
    if (!ready || (unsigned)road1 >= LIGHT_COUNT || (unsigned)road2 >= LIGHT_COUNT)
        return false;
    if (road1 == LIGHT_GREEN && road2 == LIGHT_GREEN) return false;

    /* All four traffic signals on the assigned kit share GPIOB.
       A single BSRR write changes all four without intermediate color codes. */
    uint32_t all = TL1_A_Pin | TL1_B_Pin | TL2_A_Pin | TL2_B_Pin;
    uint32_t high = select_pins(color_code[0][road1], TL1_A_Pin, TL1_B_Pin)
                  | select_pins(color_code[1][road2], TL2_A_Pin, TL2_B_Pin);
    TL1_A_GPIO_Port->BSRR = high | ((all & ~high) << 16U);
    current[0] = road1;
    current[1] = road2;
    return true;
}

bool traffic_init(const uint8_t codes[2][LIGHT_COUNT])
{
    if (!codes || TL1_A_GPIO_Port != GPIOB || TL1_B_GPIO_Port != GPIOB ||
        TL2_A_GPIO_Port != GPIOB || TL2_B_GPIO_Port != GPIOB) return false;
    for (unsigned road = 0; road < 2; ++road) {
        unsigned used = 0;
        for (unsigned color = 0; color < LIGHT_COUNT; ++color) {
            unsigned code = codes[road][color];
            if (code > 3U || (used & (1U << code))) return false;
            used |= 1U << code;
        }
    }
    memcpy(color_code, codes, sizeof(color_code));
    ready = true;
    return traffic_set_pair(LIGHT_RED, LIGHT_RED);
}

bool traffic_set(unsigned road, LightColor color)
{
    if (road == 1U) return traffic_set_pair(color, current[1]);
    if (road == 2U) return traffic_set_pair(current[0], color);
    return false;
}

LightColor traffic_get(unsigned road)
{
    if (!ready) return LIGHT_OFF;

    if (road == 1U) return current[0];
    if (road == 2U) return current[1];

    return LIGHT_OFF;
}
