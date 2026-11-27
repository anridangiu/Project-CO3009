
#include "main.h"
#include "dht20.h"
#include "lcd_i2c.h"

#include <stdio.h>
#include <string.h>

/* Peripheral handles */
I2C_HandleTypeDef hi2c1;
UART_HandleTypeDef huart2;

/* DHT20 data */
static DHT20_Data_t sensorData;

/* Function prototypes */
void SystemClock_Config(void);
static void Display_SensorData(float temperature, float humidity);
static void Display_Error(const char *message);
static void Read_DHT20(void);

/* =========================================================
 * MAIN
 * ========================================================= */

int main(void)
{
    HAL_Init();

    SystemClock_Config();

    MX_GPIO_Init();
    MX_USART2_UART_Init();
    MX_I2C1_Init();

    /* Initialize LCD */
    if (LCD_Init(&hi2c1) != HAL_OK)
    {
        /* LCD initialization failed */
        while (1)
        {
        }
    }

    LCD_Clear();
    LCD_PrintAt(0, 0, "DHT20 + LCD");
    LCD_PrintAt(0, 1, "Initializing...");
    HAL_Delay(1000);

    /* Initialize DHT20 */
    if (DHT20_Init(&hi2c1) != HAL_OK)
    {
        Display_Error("DHT20 INIT ERR");

        while (1)
        {
        }
    }

    LCD_Clear();
    LCD_PrintAt(0, 0, "Sensor Ready");
    LCD_PrintAt(0, 1, "Reading data...");
    HAL_Delay(1000);

    /* Main loop */
    while (1)
    {
        Read_DHT20();

        /* Read sensor every 2 seconds */
        HAL_Delay(2000);
    }
}

/* =========================================================
 * READ DHT20
 * ========================================================= */

static void Read_DHT20(void)
{
    HAL_StatusTypeDef status;
    uint8_t retry = 0;

    /* Start measurement */
    status = DHT20_StartMeasurement(&hi2c1);

    if (status != HAL_OK)
    {
        Display_Error("START MEASURE ERR");
        return;
    }

    /* Wait for measurement */
    HAL_Delay(DHT20_MEASURE_DELAY_MS);

    /* Read measurement result */
    do
    {
        status = DHT20_Read(&hi2c1, &sensorData);

        if (status == HAL_BUSY)
        {
            HAL_Delay(10);
            retry++;
        }

    } while (status == HAL_BUSY && retry < 5U);

    if (status == HAL_OK)
    {
        Display_SensorData(
            sensorData.temperature_c,
            sensorData.humidity_rh
        );
    }
    else
    {
        Display_Error("SENSOR READ ERR");
    }
}

/* =========================================================
 * DISPLAY SENSOR DATA
 * ========================================================= */

static void Display_SensorData(float temperature, float humidity)
{
    char line1[17];
    char line2[17];

    /* Fill LCD lines with spaces */
    memset(line1, ' ', 16);
    memset(line2, ' ', 16);

    line1[16] = '\0';
    line2[16] = '\0';

    /* Format sensor values */
    snprintf(line1, sizeof(line1), "TEMP:%5.1f C",
             (double)temperature);

    snprintf(line2, sizeof(line2), "HUM :%5.1f %% ",
             (double)humidity);

    /* Display on LCD */
    LCD_PrintAt(0, 0, line1);
    LCD_PrintAt(0, 1, line2);
}

/* =========================================================
 * DISPLAY ERROR
 * ========================================================= */

static void Display_Error(const char *message)
{
    LCD_Clear();

    LCD_PrintAt(0, 0, "SENSOR ERROR");
    LCD_PrintAt(0, 1, message);
}
