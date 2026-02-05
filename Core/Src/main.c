/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : FreeRTOS Queue & Task Communication Demo
  * @author         : Abhishek
  ******************************************************************************
  * @attention
  *
  * This project demonstrates:
  *  - FreeRTOS multitasking
  *  - Inter-task communication using queues
  *  - LED control and UART output on STM32
  *
  ******************************************************************************
  */
/* USER CODE END Header */

#include "main.h"
#include "cmsis_os.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart2;

/* FreeRTOS Handles */
QueueHandle_t xQueue;
QueueHandle_t xQueue1;

/* Function prototypes -------------------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);

/* Task prototypes -----------------------------------------------------------*/
void led_task(void *argument);
void sender_task(void *argument);
void receiver_task(void *argument);
void rank_show_task(void *argument);

/* -------------------------------------------------------------------------- */
/*                                  TASKS                                     */
/* -------------------------------------------------------------------------- */

/**
 * @brief Blinks LED (PD12) periodically
 */
void led_task(void *argument)
{
    TickType_t lastWakeTime = xTaskGetTickCount();

    while (1)
    {
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_SET);
        vTaskDelay(pdMS_TO_TICKS(1000));

        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_RESET);
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(2000));
    }
}

/**
 * @brief Generates random numbers and sends them to queue
 */
void sender_task(void *argument)
{
    TickType_t lastWakeTime = xTaskGetTickCount();
    srand(HAL_GetTick());

    while (1)
    {
        for (int i = 0; i < 6; i++)
        {
            int value = rand() % 10000;
            xQueueSendToBack(xQueue, &value, portMAX_DELAY);
        }

        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(2000));
    }
}

/**
 * @brief Receives numbers, calculates rank of first element
 */
void receiver_task(void *argument)
{
    TickType_t lastWakeTime = xTaskGetTickCount();
    int arr[6];
    int rank;

    while (1)
    {
        for (int i = 0; i < 6; i++)
        {
            xQueueReceive(xQueue, &arr[i], portMAX_DELAY);
        }

        int target = arr[0];
        rank = 0;

        for (int i = 0; i < 6; i++)
        {
            if (arr[i] <= target)
                rank++;
        }

        xQueueSendToBack(xQueue1, &rank, portMAX_DELAY);
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(1000));
    }
}

/**
 * @brief Displays rank using LEDs and UART
 */
void rank_show_task(void *argument)
{
    TickType_t lastWakeTime = xTaskGetTickCount();
    int rank;
    char uart_buf[64];

    while (1)
    {
        xQueueReceive(xQueue1, &rank, portMAX_DELAY);

        /* Reset LEDs */
        HAL_GPIO_WritePin(GPIOD,
                          GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15,
                          GPIO_PIN_RESET);

        /* LED indication based on rank */
        switch (rank)
        {
            case 1:
                HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, GPIO_PIN_SET);
                break;
            case 2:
                HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_SET);
                break;
            case 3:
                HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13 | GPIO_PIN_14, GPIO_PIN_SET);
                break;
            case 4:
                HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, GPIO_PIN_SET);
                break;
            case 5:
                HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13 | GPIO_PIN_15, GPIO_PIN_SET);
                break;
            default:
                break;
        }

        sprintf(uart_buf, "Rank = %d\r\n", rank);
        HAL_UART_Transmit(&huart2,
                          (uint8_t *)uart_buf,
                          strlen(uart_buf),
                          HAL_MAX_DELAY);

        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(1000));
    }
}

/* -------------------------------------------------------------------------- */
/*                                  MAIN                                      */
/* -------------------------------------------------------------------------- */

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART2_UART_Init();

    /* Create queues */
    xQueue  = xQueueCreate(6, sizeof(int));
    xQueue1 = xQueueCreate(1, sizeof(int));

    /* Create tasks */
    xTaskCreate(led_task, "LED", 256, NULL, 2, NULL);
    xTaskCreate(sender_task, "Sender", 256, NULL, 4, NULL);
    xTaskCreate(receiver_task, "Receiver", 256, NULL, 3, NULL);
    xTaskCreate(rank_show_task, "RankShow", 256, NULL, 2, NULL);

    /* Start scheduler */
    vTaskStartScheduler();

    while (1) {}
}

/* -------------------------------------------------------------------------- */
/*                          HARDWARE CONFIG                                   */
/* -------------------------------------------------------------------------- */

static void MX_USART2_UART_Init(void)
{
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart2);
}

static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOD_CLK_ENABLE();

    HAL_GPIO_WritePin(GPIOD,
                      GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15,
                      GPIO_PIN_RESET);

    GPIO_InitStruct.Pin = GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
}

