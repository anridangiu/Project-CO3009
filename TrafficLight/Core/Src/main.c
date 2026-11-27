/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "button.h"
#include "traffic_light.h"
#include "traffic_light_config.h"
#include "scheduler.h"
#include "software_timer.h"
#include "fsm_automatic.h"
#include "fsm_manual.h"
#include "fsm_tuning.h"
#include "dht20.h"
#include "lcd_i2c.h"
#include "global.h"
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
static const uint8_t TL_CODES[2][LIGHT_COUNT] = {
    /* OFF, RED, AMBER, GREEN */
    {0, 3, 2, 1}, /* Road 1: calibrated to the observed A/B mapping */
    {0, 3, 2, 1}  /* Road 2: calibrated to the observed A/B mapping */
};

typedef enum {
  APP_MODE_AUTOMATIC = 0,
  APP_MODE_MANUAL,
  APP_MODE_TUNING
} AppMode;

static AppMode app_mode = APP_MODE_AUTOMATIC;
static bool lcd_ready;
static bool dht20_initialized;
static bool dht20_measurement_pending;
static char dht20_refresh_mark = '*';
static uint32_t dht20_next_ms;
static uint32_t dht20_read_after_ms;
static uint32_t dht20_measurement_started_ms;
static char lcd_last_line[2][LCD_COLS + 1U];
static bool lcd_line_cached[2];

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static bool time_reached(uint32_t now, uint32_t deadline)
{
  return (int32_t)(now - deadline) >= 0;
}

static void lcd_invalidate_cache(void)
{
  lcd_line_cached[0] = false;
  lcd_line_cached[1] = false;
}

static void lcd_write_line(uint8_t row, const char *text)
{
  char line[LCD_COLS + 1U];
  uint8_t i;

  if (!lcd_ready || row >= LCD_ROWS || text == NULL) return;

  memset(line, ' ', LCD_COLS);
  line[LCD_COLS] = '\0';
  for (i = 0U; i < LCD_COLS && text[i] != '\0'; ++i) {
    line[i] = text[i];
  }

  if (!lcd_line_cached[row] ||
      memcmp(lcd_last_line[row], line, LCD_COLS + 1U) != 0) {
    if (LCD_PrintAt(0U, row, line) == HAL_OK) {
      memcpy(lcd_last_line[row], line, LCD_COLS + 1U);
      lcd_line_cached[row] = true;
    }
  }
}

static void format_tenths(char *out, size_t out_size, float value)
{
  int32_t tenths = (int32_t)(value * 10.0f + ((value >= 0.0f) ? 0.5f : -0.5f));
  uint32_t magnitude = (tenths < 0) ? (uint32_t)(-tenths) : (uint32_t)tenths;

  (void)snprintf(out, out_size, "%s%lu.%lu",
                 (tenths < 0) ? "-" : "",
                 (unsigned long)(magnitude / 10U),
                 (unsigned long)(magnitude % 10U));
}

static char traffic_color_code(LightColor color)
{
  switch (color) {
  case LIGHT_RED:   return 'R';
  case LIGHT_AMBER: return 'A';
  case LIGHT_GREEN: return 'G';
  case LIGHT_OFF:   return '-';
  default:          return '?';
  }
}

static void format_short_seconds(char out[3], uint32_t milliseconds)
{
  uint32_t seconds = milliseconds / 1000U;
  if ((milliseconds % 1000U) != 0U) ++seconds;

  if (seconds > 99U) {
    out[0] = '+';
    out[1] = '+';
    out[2] = '\0';
  } else {
    (void)snprintf(out, 3U, "%02lu", (unsigned long)seconds);
  }
}

static void lcd_status_task(void)
{
  char line0[32];
  char line1[32];
  char temperature[10];
  char humidity[10];
  char road1_seconds[3];
  char road2_seconds[3];

  if (!lcd_ready) return;
  if (app_mode == APP_MODE_TUNING) return;

  if (app_mode == APP_MODE_AUTOMATIC) {
    uint32_t road1_remaining_ms;
    uint32_t road2_remaining_ms;

    fsm_automatic_get_road_remaining_ms(&road1_remaining_ms,
                                        &road2_remaining_ms);
    format_short_seconds(road1_seconds, road1_remaining_ms);
    format_short_seconds(road2_seconds, road2_remaining_ms);
    (void)snprintf(line0, sizeof(line0), "AUTO 1:%c%s 2:%c%s",
                   traffic_color_code(traffic_get(1U)), road1_seconds,
                   traffic_color_code(traffic_get(2U)), road2_seconds);
  } else {
    (void)snprintf(line0, sizeof(line0), "MANUAL 1:%c 2:%c",
                   traffic_color_code(traffic_get(1U)),
                   traffic_color_code(traffic_get(2U)));
  }

  if (dht20DataValid == 0U) {
    (void)snprintf(line1, sizeof(line1), "DHT20: NO DATA");
  } else {
    format_tenths(temperature, sizeof(temperature), dht20Temperature);
    format_tenths(humidity, sizeof(humidity), dht20Humidity);
    (void)snprintf(line1, sizeof(line1), "T:%sC H:%s%%%c", temperature,
                   humidity, dht20_refresh_mark);
  }

  lcd_write_line(0U, line0);
  lcd_write_line(1U, line1);
}

static void dht20_task(void)
{
  const uint32_t now = HAL_GetTick();
  DHT20_Data_t data;
  HAL_StatusTypeDef status;

  if (dht20_measurement_pending) {
    if (!time_reached(now, dht20_read_after_ms)) return;

    status = DHT20_Read(&hi2c1, &data);
    if (status == HAL_BUSY &&
        (uint32_t)(now - dht20_measurement_started_ms) < 250U) {
      return;
    }

    dht20_measurement_pending = false;
    dht20_next_ms = now + 2000U;
    if (status == HAL_OK) {
      dht20Temperature = data.temperature_c;
      dht20Humidity = data.humidity_rh;
      dht20DataValid = 1U;
      dht20_refresh_mark = (dht20_refresh_mark == '*') ? '+' : '*';
    } else {
      dht20DataValid = 0U;
      dht20_initialized = false;
    }
    return;
  }

  if (!time_reached(now, dht20_next_ms)) return;

  if (!dht20_initialized) {
    if (DHT20_Init(&hi2c1) != HAL_OK) {
      dht20DataValid = 0U;
      dht20_next_ms = now + 2000U;
      return;
    }
    dht20_initialized = true;
    dht20_next_ms = HAL_GetTick();
    return;
  }

  if (DHT20_StartMeasurement(&hi2c1) == HAL_OK) {
    dht20_measurement_pending = true;
    dht20_measurement_started_ms = HAL_GetTick();
    dht20_read_after_ms = dht20_measurement_started_ms + DHT20_MEASURE_DELAY_MS;
  } else {
    dht20DataValid = 0U;
    dht20_initialized = false;
    dht20_next_ms = now + 2000U;
  }
}

static void apply_timing_settings(void)
{
  uint32_t green_ms = (uint32_t)greenDuration * 1000U;
  uint32_t yellow_ms = (uint32_t)yellowDuration * 1000U;
  uint32_t red_clearance_ms = (uint32_t)redDuration * 1000U;

  if (!fsm_automatic_set_durations(green_ms, green_ms, yellow_ms) ||
      !fsm_automatic_set_all_red_duration(red_clearance_ms)) {
    Error_Handler();
  }
}

static void enter_tuning_mode(void)
{
  if (!traffic_set_pair(LIGHT_RED, LIGHT_RED) || !fsm_tuning_enter()) {
    Error_Handler();
  }

  app_mode = APP_MODE_TUNING;
  lcd_invalidate_cache();
  fsm_tuning_run();
}

/* AUTO: BUTTON_1 enters Manual; BUTTON_3 enters Timing Adjustment.
 * MANUAL: BUTTON_2 advances a phase; BUTTON_1 returns to Auto;
 *         BUTTON_3 enters Timing Adjustment.
 * TUNING: BUTTON_1 selects a color; BUTTON_3/4 adjust; USER saves;
 *         BUTTON_2 returns to Auto.
 */
static void mode_task(void)
{
  switch (app_mode) {
  case APP_MODE_AUTOMATIC:
    if (button_take_press(BUTTON_1)) {
      button_flush();
      fsm_automatic_stop();
      if (!fsm_manual_enter()) Error_Handler();
      app_mode = APP_MODE_MANUAL;
      lcd_invalidate_cache();
    } else if (button_take_press(BUTTON_3)) {
      button_flush();
      fsm_automatic_stop();
      enter_tuning_mode();
    } else {
      fsm_automatic_run();
    }
    break;

  case APP_MODE_MANUAL:
    if (button_take_press(BUTTON_1)) {
      button_flush();
      fsm_manual_exit();
      apply_timing_settings();
      if (!fsm_automatic_start()) Error_Handler();
      app_mode = APP_MODE_AUTOMATIC;
      lcd_invalidate_cache();
    } else if (button_take_press(BUTTON_3)) {
      button_flush();
      fsm_manual_exit();
      enter_tuning_mode();
    } else {
      fsm_manual_run();
    }
    break;

  case APP_MODE_TUNING:
    if (button_take_press(BUTTON_2)) {
      button_flush();
      fsm_tuning_exit();
      apply_timing_settings();
      if (!fsm_automatic_start()) Error_Handler();
      app_mode = APP_MODE_AUTOMATIC;
      lcd_invalidate_cache();
    } else {
      fsm_tuning_run();
    }
    break;

  default:
    fsm_automatic_stop();
    app_mode = APP_MODE_AUTOMATIC;
    break;
  }
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_I2C1_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
  SCH_Init();
  software_timer_init();
  button_init(); /* Captures the released level for each installed button module. */

  lcd_ready = (LCD_Init(&hi2c1) == HAL_OK);
  lcd_invalidate_cache();
  dht20_initialized = false;
  dht20_measurement_pending = false;
  dht20DataValid = 0U;
  dht20_next_ms = HAL_GetTick();

  if (!traffic_init(TL_CODES)) Error_Handler();
  if (!fsm_automatic_init((uint32_t)greenDuration * 1000U,
                          (uint32_t)greenDuration * 1000U,
                          (uint32_t)yellowDuration * 1000U) ||
      !fsm_automatic_set_all_red_duration((uint32_t)redDuration * 1000U)) {
    Error_Handler();
  }

  if (SCH_Add_Task(button_scan, 1U, 1U) == 0U) Error_Handler();
  if (SCH_Add_Task(software_timer_tick, 1U, 1U) == 0U) Error_Handler();
  if (SCH_Add_Task(mode_task, 1U, 1U) == 0U) Error_Handler();
  if (SCH_Add_Task(dht20_task, 1U, 1U) == 0U) Error_Handler();
  if (SCH_Add_Task(lcd_status_task, 1U, 25U) == 0U) Error_Handler();

  if (!fsm_automatic_start()) Error_Handler();
  if (HAL_TIM_Base_Start_IT(&htim2) != HAL_OK) Error_Handler();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    SCH_Dispatch_Tasks();
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
   
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL16;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 6399;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 99;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, TL2_B_Pin|TL1_A_Pin|TL2_A_Pin|TL1_B_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED_PC9_GPIO_Port, LED_PC9_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : BTN_USER_Pin */
  GPIO_InitStruct.Pin = BTN_USER_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(BTN_USER_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : TL2_B_Pin TL1_A_Pin TL2_A_Pin TL1_B_Pin */
  GPIO_InitStruct.Pin = TL2_B_Pin|TL1_A_Pin|TL2_A_Pin|TL1_B_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : BTN_3_Pin */
  GPIO_InitStruct.Pin = BTN_3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(BTN_3_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LED_PC9_Pin */
  GPIO_InitStruct.Pin = LED_PC9_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_PC9_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : BTN_1_Pin BTN_2_Pin */
  GPIO_InitStruct.Pin = BTN_1_Pin|BTN_2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : BTN_4_Pin */
  GPIO_InitStruct.Pin = BTN_4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(BTN_4_GPIO_Port, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM2) {
    SCH_Update();
  }
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
