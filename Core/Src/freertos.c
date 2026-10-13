/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "cmsis_os2.h"
#include "main.h"
#include <stdio.h>
#include <string.h>
#include "usart.h"
#include "App_data.h"
#include "adc.h"
#include "Int_aht20.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
//PD�?么意思：Private define=私定义宏定义
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
osMessageQueueId_t SensorDataQueueHandle;//传感器数据队列句�?

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
// LED 任务
void LED_Task(void *argument);
// Debug 任务
void Debug_Task(void *argument);
// Sensor 任务
void Sensor_Task(void *argument);

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  const osMessageQueueAttr_t SensorDataQueue_attributes = {
    .name = "SensorDataQueue"
  };//传感器数据队列属�?

  SensorDataQueueHandle = osMessageQueueNew(
      8,
      sizeof(SensorData_t),
      &SensorDataQueue_attributes
  );;//创建传感器数据队�?
  if (SensorDataQueueHandle == NULL)
  {
      /* 队列创建失败，可在此设置错误标志 */
  }

  const osThreadAttr_t sensorTask_attributes = {
      .name = "Sensor_Task",
      .stack_size = 768 * 4,
      .priority = (osPriority_t)osPriorityNormal
  };


  const osThreadAttr_t ledTask_attributes = {
    .name = "LED_Task",
    .stack_size = 256 * 4,
    .priority = (osPriority_t)osPriorityLow
  };

  const osThreadAttr_t debugTask_attributes = {
      .name = "Debug_Task",
      .stack_size = 768 * 4,
      .priority = (osPriority_t)osPriorityNormal
  };


  osThreadNew(LED_Task, NULL, &ledTask_attributes);
  osThreadNew(Debug_Task, NULL, &debugTask_attributes);
  osThreadNew(Sensor_Task,NULL,&sensorTask_attributes);

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
void LED_Task(void *argument)
{
  (void)argument;

  for (;;)
  {
      HAL_GPIO_TogglePin(LED_GPIO_Port,
                          LED_Pin);

      osDelay(500);
  }
}
void Debug_Task(void *argument)
{
    SensorData_t data;
    char msg[160];

    (void)argument;

    for (;;)
    {
        /*
         * 等待队列数据�?
         * osWaitForever 表示没有数据时阻塞等待，
         * 而不是不断空转占�? CPU�?
         */
        if (osMessageQueueGet(
                SensorDataQueueHandle,
                &data,
                NULL,
                osWaitForever) == osOK)
        {
            int len;

            if (data.aht20_valid != 0U)//
            {
                len = snprintf(
                    msg,
                    sizeof(msg),
                    "ADC=%lu V=%.3f T=%.2fC H=%.2f%%RH\r\n",
                    (unsigned long)data.adc_raw,
                    (double)data.adc_voltage,
                    (double)data.temperature,
                    (double)data.humidity
                );
            }
            else
            {
                len = snprintf(
                    msg,
                    sizeof(msg),
                    "ADC=%lu V=%.3f AHT20=ERROR\r\n",
                    (unsigned long)data.adc_raw,
                    (double)data.adc_voltage
                );
            }

            if (len > 0 && len < (int)sizeof(msg))
            {
                HAL_UART_Transmit(
                    &huart1,
                    (uint8_t *)msg,
                    (uint16_t)len,
                    1000
                );
            }
        }
    }
}

static uint8_t ADC_ReadRaw(uint32_t *value)
{
    if (value == NULL)
    {
        return 1U;
    }

    /* 启动�?�? ADC 转换 */
    if (HAL_ADC_Start(&hadc1) != HAL_OK)
    {
        return 2U;
    }

    /* 等待转换完成，最多等�? 100ms */
    if (HAL_ADC_PollForConversion(&hadc1, 100) != HAL_OK)
    {
        HAL_ADC_Stop(&hadc1);
        return 3U;
    }

    /* 读取 12 �? ADC 原始�? */
    *value = HAL_ADC_GetValue(&hadc1);

    HAL_ADC_Stop(&hadc1);

    return 0U;
}

void Sensor_Task(void *argument)
{
    SensorData_t data;
    AHT20_Data_t aht;

    (void)argument;

    for (;;)
    {
        data.adc_raw = 0;
        data.adc_voltage = 0.0f;
        data.temperature = 0.0f;
        data.humidity = 0.0f;
        data.aht20_valid = 0U;

        /* 读取 PA0 对应�? ADC 通道 */
        if (ADC_ReadRaw(&data.adc_raw) == 0U)
        {
            data.adc_voltage =
                (float)data.adc_raw * 3.3f / 4095.0f;
        }

        /* 读取 AHT20 */
        if (AHT20_ReadData(&aht) == AHT20_OK)
        {
            data.temperature = aht.temperature;
            data.humidity = aht.humidity;
            data.aht20_valid = 1U;
        }

        /*
         * 这里使用 HAL_GetTick() 作为时间标记�?
         * 后续可按项目�?要统�?时间基准�?
         */
        data.timestamp_ms = HAL_GetTick();

        /*
         * 队列满时，等待最�? 10ms�?
         * 如果仍无法发送，就丢弃本次数据并继续下一轮�??
         */
        (void)osMessageQueuePut(
            SensorDataQueueHandle,
            &data,
            0U,
            10U
        );

        osDelay(500);
    }
}
/* USER CODE END Application */

