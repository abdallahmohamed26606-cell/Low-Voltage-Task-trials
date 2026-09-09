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
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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
SPI_HandleTypeDef hspi1;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */

uint8_t uart_rx;

uint16_t case_id;

uint8_t spi_tx[3];
uint8_t spi_rx[3];

uint8_t voltage_x10;
uint16_t wheel_speed;

char msg[100];

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART1_UART_Init(void);

/* USER CODE BEGIN PFP */

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

  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SPI1_Init();
  MX_USART1_UART_Init();

  /* USER CODE BEGIN 2 */

  /* Make sure Slave starts deselected */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

    /* Wait for Case ID 1-5 from Virtual Terminal */
    HAL_UART_Receive(&huart1,
                     &uart_rx,
                     1,
                     HAL_MAX_DELAY);

    /* Only accept characters '1' to '5' */
    if (uart_rx >= '1' && uart_rx <= '5')
    {
        /*
         * Convert:
         *
         * '1' -> 0x1001
         * '2' -> 0x1002
         * '3' -> 0x1003
         * '4' -> 0x1004
         * '5' -> 0x1005
         */
        case_id = 0x1000 + (uart_rx - '0');


        /*
         * Prepare the 3-byte SPI message.
         *
         * Example:
         *
         * 0x1003
         *
         * Byte 0 = 0x10
         * Byte 1 = 0x03
         * Byte 2 = 0x00
         */
        spi_tx[0] = (case_id >> 8) & 0xFF;
        spi_tx[1] = case_id & 0xFF;
        spi_tx[2] = 0x00;


        /* =====================================================
         * SPI CYCLE 1
         *
         * Send Case ID to Slave.
         *
         * The response received during this cycle is ignored
         * because the Slave has not prepared the new response yet.
         * =====================================================
         */

        /* CS LOW -> select Slave */
        HAL_GPIO_WritePin(GPIOB,
                          GPIO_PIN_0,
                          GPIO_PIN_RESET);

        HAL_SPI_TransmitReceive(&hspi1,
                                spi_tx,
                                spi_rx,
                                3,
                                HAL_MAX_DELAY);

        /* CS HIGH -> deselect Slave */
        HAL_GPIO_WritePin(GPIOB,
                          GPIO_PIN_0,
                          GPIO_PIN_SET);


        /*
         * Give Slave time to decode Case ID
         * and prepare the new 24-bit response.
         */
        HAL_Delay(10);


        /* =====================================================
         * SPI CYCLE 2
         *
         * Send again.
         *
         * Now the Slave sends the response that corresponds
         * to the Case ID selected above.
         * =====================================================
         */

        /* CS LOW */
        HAL_GPIO_WritePin(GPIOB,
                          GPIO_PIN_0,
                          GPIO_PIN_RESET);

        HAL_SPI_TransmitReceive(&hspi1,
                                spi_tx,
                                spi_rx,
                                3,
                                HAL_MAX_DELAY);

        /* CS HIGH */
        HAL_GPIO_WritePin(GPIOB,
                          GPIO_PIN_0,
                          GPIO_PIN_SET);


        /*
         * 24-bit response format from Slave:
         *
         * spi_rx[0] = Voltage x10
         * spi_rx[1] = Wheel Speed high byte
         * spi_rx[2] = Wheel Speed low byte
         */

        voltage_x10 = spi_rx[0];

        wheel_speed =
            ((uint16_t)spi_rx[1] << 8)
            |
            spi_rx[2];


        /*
         * Example:
         *
         * voltage_x10 = 123
         *
         * 123 / 10 = 12
         * 123 % 10 = 3
         *
         * Displays 12.3 V
         */
        sprintf(msg,
                "Case %c | Voltage = %u.%u V | Wheel Speed = %u\r\n",
                uart_rx,
                voltage_x10 / 10,
                voltage_x10 % 10,
                wheel_speed);


        /* Send result to Virtual Terminal */
        HAL_UART_Transmit(&huart1,
                          (uint8_t *)msg,
                          strlen(msg),
                          HAL_MAX_DELAY);
    }

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

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;

  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType =
      RCC_CLOCKTYPE_HCLK |
      RCC_CLOCKTYPE_SYSCLK |
      RCC_CLOCKTYPE_PCLK1 |
      RCC_CLOCKTYPE_PCLK2;

  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct,
                          FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */

  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;

  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */
}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */

  huart1.Instance = USART1;
  huart1.Init.BaudRate = 9600;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;

  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /* PB0 starts HIGH because CS is active LOW */
  HAL_GPIO_WritePin(GPIOB,
                    GPIO_PIN_0,
                    GPIO_PIN_SET);

  /* Configure PB0 as GPIO output */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

  HAL_GPIO_Init(GPIOB,
                &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */

  __disable_irq();

  while (1)
  {
  }

  /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT

void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */

  /* USER CODE END 6 */
}

#endif /* USE_FULL_ASSERT */
