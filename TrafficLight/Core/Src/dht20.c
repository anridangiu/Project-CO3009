#include "dht20.h"

#define DHT20_TIMEOUT_MS 100U

HAL_StatusTypeDef DHT20_Init(I2C_HandleTypeDef *hi2c)
{
    uint8_t status = 0;
    uint8_t init_cmd[3] = {0xBE, 0x08, 0x00};

    if (hi2c == NULL)
        return HAL_ERROR;

    /* AHT20/DHT20 requires a short power-up settling time. */
    HAL_Delay(40);

    if (HAL_I2C_Master_Receive(hi2c, DHT20_I2C_ADDR, &status, 1,
                               DHT20_TIMEOUT_MS) != HAL_OK)
        return HAL_ERROR;

    /* Bit 3 indicates calibration enable. Set calibration if needed. */
    if ((status & 0x08U) == 0U)
    {
        if (HAL_I2C_Master_Transmit(hi2c, DHT20_I2C_ADDR, init_cmd, 3,
                                    DHT20_TIMEOUT_MS) != HAL_OK)
            return HAL_ERROR;

        HAL_Delay(10);
    }

    return HAL_OK;
}

HAL_StatusTypeDef DHT20_StartMeasurement(I2C_HandleTypeDef *hi2c)
{
    uint8_t command[3] = {0xAC, 0x33, 0x00};

    if (hi2c == NULL)
        return HAL_ERROR;

    return HAL_I2C_Master_Transmit(hi2c, DHT20_I2C_ADDR, command, 3,
                                   DHT20_TIMEOUT_MS);
}

HAL_StatusTypeDef DHT20_Read(I2C_HandleTypeDef *hi2c, DHT20_Data_t *data)
{
    uint8_t raw[7] = {0};
    uint32_t humidity_raw;
    uint32_t temperature_raw;
    uint8_t status;

    if ((hi2c == NULL) || (data == NULL))
        return HAL_ERROR;

    if (HAL_I2C_Master_Receive(hi2c, DHT20_I2C_ADDR, raw, 7,
                               DHT20_TIMEOUT_MS) != HAL_OK)
        return HAL_ERROR;

    status = raw[0];

    /* Bit 7 = busy; caller should retry on the next scheduler tick. */
    if ((status & 0x80U) != 0U)
        return HAL_BUSY;

    humidity_raw = ((uint32_t)raw[1] << 12)
                 | ((uint32_t)raw[2] << 4)
                 | ((uint32_t)raw[3] >> 4);

    temperature_raw = (((uint32_t)raw[3] & 0x0FU) << 16)
                    | ((uint32_t)raw[4] << 8)
                    | (uint32_t)raw[5];

    data->humidity_rh = ((float)humidity_raw * 100.0f) / 1048576.0f;
    data->temperature_c = (((float)temperature_raw * 200.0f) / 1048576.0f)
                        - 50.0f;

    return HAL_OK;
}
