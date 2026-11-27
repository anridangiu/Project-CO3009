#ifndef LCD_I2C_H
#define LCD_I2C_H

#include "main.h"

/*
 * Generic HD44780 16x2 LCD with PCF8574 I2C backpack.
 * Change LCD_I2C_ADDR if your backpack uses another 7-bit address.
 */
#define LCD_I2C_ADDR       (0x27U << 1)
#define LCD_COLS           16U
#define LCD_ROWS           2U

HAL_StatusTypeDef LCD_Init(I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef LCD_Clear(void);
HAL_StatusTypeDef LCD_SetCursor(uint8_t col, uint8_t row);
HAL_StatusTypeDef LCD_Print(const char *text);
HAL_StatusTypeDef LCD_PrintAt(uint8_t col, uint8_t row, const char *text);

#endif
