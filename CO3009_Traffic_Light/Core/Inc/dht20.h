#ifndef DHT20_H
#define DHT20_H

#include "main.h"

#define DHT20_I2C_ADDR          (0x38U << 1)
#define DHT20_MEASURE_DELAY_MS  80U

typedef struct
{
    float temperature_c;
    float humidity_rh;
} DHT20_Data_t;

/* Call once after MX_I2C1_Init(). Initialization delay occurs only at startup. */
HAL_StatusTypeDef DHT20_Init(I2C_HandleTypeDef *hi2c);

/* Start a measurement. Call DHT20_Read() at least 80 ms later. */
HAL_StatusTypeDef DHT20_StartMeasurement(I2C_HandleTypeDef *hi2c);

/* Returns HAL_BUSY if the sensor is still measuring. */
HAL_StatusTypeDef DHT20_Read(I2C_HandleTypeDef *hi2c, DHT20_Data_t *data);

#endif
