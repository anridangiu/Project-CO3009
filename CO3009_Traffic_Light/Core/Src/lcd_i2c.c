#include "lcd_i2c.h"
#include <string.h>

#define LCD_BACKLIGHT  0x08U
#define LCD_ENABLE     0x04U
#define LCD_RS         0x01U
#define LCD_TIMEOUT_MS 100U

static I2C_HandleTypeDef *lcd_i2c = NULL;

static HAL_StatusTypeDef LCD_ExpanderWrite(uint8_t value)
{
    return HAL_I2C_Master_Transmit(lcd_i2c, LCD_I2C_ADDR, &value, 1,
                                   LCD_TIMEOUT_MS);
}

static HAL_StatusTypeDef LCD_PulseEnable(uint8_t value)
{
    HAL_StatusTypeDef result;

    result = LCD_ExpanderWrite(value | LCD_ENABLE);
    if (result != HAL_OK) return result;
    result = LCD_ExpanderWrite(value & (uint8_t)~LCD_ENABLE);
    return result;
}

static HAL_StatusTypeDef LCD_Write4Bits(uint8_t value)
{
    HAL_StatusTypeDef result = LCD_ExpanderWrite(value | LCD_BACKLIGHT);
    if (result != HAL_OK) return result;
    return LCD_PulseEnable(value | LCD_BACKLIGHT);
}

static HAL_StatusTypeDef LCD_Send(uint8_t value, uint8_t mode)
{
    uint8_t high = (value & 0xF0U) | mode | LCD_BACKLIGHT;
    uint8_t low  = ((value << 4) & 0xF0U) | mode | LCD_BACKLIGHT;
    HAL_StatusTypeDef result;

    result = LCD_Write4Bits(high);
    if (result != HAL_OK) return result;
    return LCD_Write4Bits(low);
}

static HAL_StatusTypeDef LCD_Command(uint8_t command)
{
    HAL_StatusTypeDef result = LCD_Send(command, 0U);
    if (result == HAL_OK)
        HAL_Delay((command == 0x01U || command == 0x02U) ? 2U : 1U);
    return result;
}

HAL_StatusTypeDef LCD_Init(I2C_HandleTypeDef *hi2c)
{
    HAL_StatusTypeDef result;

    if (hi2c == NULL)
        return HAL_ERROR;

    lcd_i2c = hi2c;
    HAL_Delay(50);

    result = LCD_Write4Bits(0x30U);
    if (result != HAL_OK) return result;
    HAL_Delay(5);
    result = LCD_Write4Bits(0x30U);
    if (result != HAL_OK) return result;
    HAL_Delay(1);
    result = LCD_Write4Bits(0x30U);
    if (result != HAL_OK) return result;
    result = LCD_Write4Bits(0x20U);
    if (result != HAL_OK) return result;

    result = LCD_Command(0x28U); /* 4-bit, 2-line, 5x8 font */
    if (result != HAL_OK) return result;
    result = LCD_Command(0x0CU); /* Display on, cursor off */
    if (result != HAL_OK) return result;
    result = LCD_Command(0x06U); /* Increment cursor */
    if (result != HAL_OK) return result;

    return LCD_Clear();
}

HAL_StatusTypeDef LCD_Clear(void)
{
    return LCD_Command(0x01U);
}

HAL_StatusTypeDef LCD_SetCursor(uint8_t col, uint8_t row)
{
    static const uint8_t row_offsets[LCD_ROWS] = {0x00U, 0x40U};

    if ((lcd_i2c == NULL) || (row >= LCD_ROWS) || (col >= LCD_COLS))
        return HAL_ERROR;

    return LCD_Command((uint8_t)(0x80U | (row_offsets[row] + col)));
}

HAL_StatusTypeDef LCD_Print(const char *text)
{
    if ((lcd_i2c == NULL) || (text == NULL))
        return HAL_ERROR;

    while (*text != '\0')
    {
        HAL_StatusTypeDef result = LCD_Send((uint8_t)*text, LCD_RS);
        if (result != HAL_OK) return result;
        text++;
    }

    return HAL_OK;
}

HAL_StatusTypeDef LCD_PrintAt(uint8_t col, uint8_t row, const char *text)
{
    HAL_StatusTypeDef result = LCD_SetCursor(col, row);
    if (result != HAL_OK) return result;
    return LCD_Print(text);
}
