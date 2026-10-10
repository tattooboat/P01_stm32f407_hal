/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <stdint.h>
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

/* USER CODE BEGIN PV */
volatile uint32_t timer_count = 0;
uint32_t last_count = 0;
char uart_buf[64];

volatile uint8_t key_event = 0;
uint32_t last_key_time = 0;

#define AHT20_ADDR       (0x38 << 1)

uint8_t aht20_data[7];
float temperature;
float humidity;
uint8_t aht20_status;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
uint8_t AHT20_Init(void);
uint8_t AHT20_Read(float *temp, float *humi);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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
  MX_USART1_UART_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_ADC1_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */
  
  HAL_TIM_Base_Start_IT(&htim2);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
  /* PA0 �? EXTI 已由 CubeMX 配置 */

  uint32_t adc_value;
  float voltage;
  char uart_buf[64];
  int len;

  if (AHT20_Init())
{
    const char msg[] = "AHT20 Init OK\r\n";
    HAL_UART_Transmit(&huart1,
                      (uint8_t *)msg,
                      sizeof(msg) - 1,
                      100);
}
else
{
    const char msg[] = "AHT20 Init FAIL\r\n";
    HAL_UART_Transmit(&huart1,
                      (uint8_t *)msg,
                      sizeof(msg) - 1,
                      100);
}
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    if (AHT20_Read(&temperature, &humidity))
    {
        int32_t t100 = (int32_t)(temperature * 100.0f);
        uint32_t h100 = (uint32_t)(humidity * 100.0f);

        uint32_t t_abs = (t100 < 0) ?
                        (uint32_t)(-t100) : (uint32_t)t100;

        char uart_buf[80];

        int len = snprintf(
            uart_buf,
            sizeof(uart_buf),
            "Temp=%s%lu.%02lu C, RH=%lu.%02lu%%\r\n",
            (t100 < 0) ? "-" : "",
            (unsigned long)(t_abs / 100),
            (unsigned long)(t_abs % 100),
            (unsigned long)(h100 / 100),
            (unsigned long)(h100 % 100)
        );

        if (len > 0 && len < (int)sizeof(uart_buf))
        {
            HAL_UART_Transmit(
                &huart1,
                (uint8_t *)uart_buf,
                (uint16_t)len,
                100
            );
        }
    }
    else
    {
        const char msg[] = "AHT20 Read FAIL\r\n";
        HAL_UART_Transmit(&huart1,
                          (uint8_t *)msg,
                          sizeof(msg) - 1,
                          100);
    }

    HAL_Delay(1000);
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
// 处理 TIM2 中断
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2)
    {
        timer_count++;
    }
}

// 处理 EXTI 中断
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_0)
    {
        key_event = 1;
    }
}

uint8_t AHT20_Init(void)
{
    uint8_t status;
    uint8_t cmd[3] = {0xBE, 0x08, 0x00};

    HAL_Delay(40);

    /* ????? */
    if (HAL_I2C_Master_Receive(
            &hi2c1, AHT20_ADDR, &status, 1, 100) != HAL_OK)
    {
        return 0;
    }

    /* Bit3 ? 0 ????????? */
    if ((status & 0x08) == 0)
    {
        if (HAL_I2C_Master_Transmit(
                &hi2c1, AHT20_ADDR, cmd, 3, 100) != HAL_OK)
        {
            return 0;
        }

        HAL_Delay(10);
    }

    return 1;
}
uint8_t AHT20_Read(float *temp, float *humi)
{
    uint8_t cmd[3] = {0xAC, 0x33, 0x00};

    /* ?????? */
    if (HAL_I2C_Master_Transmit(
            &hi2c1, AHT20_ADDR, cmd, 3, 100) != HAL_OK)
    {
        return 0;
    }

    /* ?????? */
    HAL_Delay(80);

    /* ??????????? CRC ?? */
    if (HAL_I2C_Master_Receive(
            &hi2c1, AHT20_ADDR, aht20_data, 7, 100) != HAL_OK)
    {
        return 0;
    }

    /* Bit7 ? 1 ????????? */
    if (aht20_data[0] & 0x80)
    {
        return 0;
    }

    /* ?? 20 ?????? */
    uint32_t raw_humidity =
        ((uint32_t)aht20_data[1] << 12) |
        ((uint32_t)aht20_data[2] << 4) |
        ((uint32_t)aht20_data[3] >> 4);

    /* ?? 20 ?????? */
    uint32_t raw_temperature =
        (((uint32_t)aht20_data[3] & 0x0F) << 16) |
        ((uint32_t)aht20_data[4] << 8) |
        (uint32_t)aht20_data[5];

    *humi = (raw_humidity * 100.0f) / 1048576.0f;
    *temp = (raw_temperature * 200.0f) / 1048576.0f - 50.0f;

    return 1;
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
