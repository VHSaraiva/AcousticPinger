/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
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
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdbool.h>
#include <time.h>
#include <math.h>
#include "arm_math.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define BUFFER_SIZE 1024
#define RX_BUFFER 256
#define NUM_TAPS 381
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc3;
DMA_HandleTypeDef hdma_adc3;

RTC_HandleTypeDef hrtc;

UART_HandleTypeDef huart3;

PCD_HandleTypeDef hpcd_USB_OTG_FS;

osThreadId defaultTaskHandle;
osThreadId Send_DataHandle;
osThreadId ADC_StarterHandle;
osThreadId UART_ReadHandle;
/* USER CODE BEGIN PV */

QueueHandle_t Messages_RX_UART;
SemaphoreHandle_t xSemaphore;

uint16_t ADC3ConvertedBuffer[BUFFER_SIZE];
uint16_t ADC3ConvertedBuffer_Filtered[BUFFER_SIZE];
bool ADC_60_90 = false;
char UART_RX_BUFFER[RX_BUFFER];


//FIR Filter implemation

uint32_t blocksize = BUFFER_SIZE;
arm_fir_instance_f32 S;
static float32_t FIRStateF32[BUFFER_SIZE+NUM_TAPS-1];
static float32_t FIRCoeffs[NUM_TAPS] = {0.00021818,2.6276e-06,-1.5429e-05,3.7284e-05,-6.8778e-05,0.00011052,-0.00016308,0.0002269,-0.00030225,0.00038916,-0.00048741,0.00059647,-0.00071552,0.00084339,-0.00097862,0.0011194,-0.0012637,0.0014093,-0.0015536,0.001694,-0.0018279,0.0019526,-0.0020655,0.0021644,-0.0022472,0.0023121,-0.0023578,0.0023836,-0.0023893,0.0023751,-0.0023421,0.0022918,-0.0022265,0.0021488,-0.0020622,0.0019702,-0.0018769,0.0017865,-0.0017033,0.0016317,-0.0015762,0.0015399,-0.0015268,0.0015393,-0.0015797,0.0016491,-0.001748,0.0018758,-0.002031,0.0022111,-0.0024127,0.0026317,-0.0028629,0.0031009,-0.0033396,0.0035726,-0.0037937,0.0039965,-0.0041751,0.0043241,-0.004439,0.0045161,-0.0045528,0.0045477,-0.0045009,0.0044136,-0.0042887,0.0041302,-0.0039434,0.0037347,-0.0035117,0.0032824,-0.0030555,0.0028398,-0.0026443,0.0024772,-0.0023463,0.0022584,-0.0022191,0.0022324,-0.0023007,0.0024247,-0.002603,0.0028323,-0.0031076,0.0034219,-0.0037665,0.0041316,-0.0045059,0.0048776,-0.0052343,0.0055637,-0.0058538,0.0060935,-0.0062729,0.0063837,-0.0064196,0.0063765,-0.0062528,0.0060494,-0.0057702,0.0054214,-0.0050119,0.0045531,-0.0040583,0.0035425,-0.0030219,0.0025137,-0.0020351,0.0016029,-0.001233,0.00093991,-0.00073585,0.00063061,-0.00063095,0.00074028,-0.00095847,0.0012816,-0.0017021,0.0022086,-0.0027865,0.0034181,-0.0040831,0.0047593,-0.0054229,0.0060498,-0.006616,0.0070983,-0.0074757,0.0077293,-0.0078438,0.007808,-0.0076149,0.0072626,-0.0067544,0.006099,-0.0053102,0.0044072,-0.0034135,0.0023569,-0.0012688,0.00018273,0.00086591,-0.0018413,0.0027083,-0.0034334,0.0039862,-0.0043402,0.0044743,-0.0043729,0.0040277,-0.0034375,0.002609,-0.0015571,0.00030452,0.0011183,-0.0026736,0.0043171,-0.005999,0.0076652,-0.0092583,0.010719,-0.01199,0.013011,-0.01373,0.014097,-0.014068,0.013607,-0.012689,0.011295,-0.0094208,0.0070707,-0.0042617,0.0010222,0.0026085,-0.0065803,0.010833,-0.0153,0.019905,-0.024567,0.029204,-0.033728,0.038054,-0.0421,0.045786,-0.04904,0.051797,-0.054001,0.055608,-0.056585,0.056914,-0.056585,0.055608,-0.054001,0.051797,-0.04904,0.045786,-0.0421,0.038054,-0.033728,0.029204,-0.024567,0.019905,-0.0153,0.010833,-0.0065803,0.0026085,0.0010222,-0.0042617,0.0070707,-0.0094208,0.011295,-0.012689,0.013607,-0.014068,0.014097,-0.01373,0.013011,-0.01199,0.010719,-0.0092583,0.0076652,-0.005999,0.0043171,-0.0026736,0.0011183,0.00030452,-0.0015571,0.002609,-0.0034375,0.0040277,-0.0043729,0.0044743,-0.0043402,0.0039862,-0.0034334,0.0027083,-0.0018413,0.00086591,0.00018273,-0.0012688,0.0023569,-0.0034135,0.0044072,-0.0053102,0.006099,-0.0067544,0.0072626,-0.0076149,0.007808,-0.0078438,0.0077293,-0.0074757,0.0070983,-0.006616,0.0060498,-0.0054229,0.0047593,-0.0040831,0.0034181,-0.0027865,0.0022086,-0.0017021,0.0012816,-0.00095847,0.00074028,-0.00063095,0.00063061,-0.00073585,0.00093991,-0.001233,0.0016029,-0.0020351,0.0025137,-0.0030219,0.0035425,-0.0040583,0.0045531,-0.0050119,0.0054214,-0.0057702,0.0060494,-0.0062528,0.0063765,-0.0064196,0.0063837,-0.0062729,0.0060935,-0.0058538,0.0055637,-0.0052343,0.0048776,-0.0045059,0.0041316,-0.0037665,0.0034219,-0.0031076,0.0028323,-0.002603,0.0024247,-0.0023007,0.0022324,-0.0022191,0.0022584,-0.0023463,0.0024772,-0.0026443,0.0028398,-0.0030555,0.0032824,-0.0035117,0.0037347,-0.0039434,0.0041302,-0.0042887,0.0044136,-0.0045009,0.0045477,-0.0045528,0.0045161,-0.004439,0.0043241,-0.0041751,0.0039965,-0.0037937,0.0035726,-0.0033396,0.0031009,-0.0028629,0.0026317,-0.0024127,0.0022111,-0.002031,0.0018758,-0.001748,0.0016491,-0.0015797,0.0015393,-0.0015268,0.0015399,-0.0015762,0.0016317,-0.0017033,0.0017865,-0.0018769,0.0019702,-0.0020622,0.0021488,-0.0022265,0.0022918,-0.0023421,0.0023751,-0.0023893,0.0023836,-0.0023578,0.0023121,-0.0022472,0.0021644,-0.0020655,0.0019526,-0.0018279,0.001694,-0.0015536,0.0014093,-0.0012637,0.0011194,-0.00097862,0.00084339,-0.00071552,0.00059647,-0.00048741,0.00038916,-0.00030225,0.0002269,-0.00016308,0.00011052,-6.8778e-05,3.7284e-05,-1.5429e-05,2.6276e-06,0.00021818
};

float32_t  *inputF32, *outputF32;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_DMA_Init(void);
static void MX_ADC3_Init(void);
static void MX_USB_OTG_FS_PCD_Init(void);
static void MX_RTC_Init(void);
void StartDefaultTask(void const * argument);
void UART_Send(void const * argument);
void ADC_Task(void const * argument);
void Read_Data(void const * argument);

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
  MX_USART3_UART_Init();
  MX_DMA_Init();
  MX_ADC3_Init();
  MX_USB_OTG_FS_PCD_Init();
  MX_RTC_Init();
  /* USER CODE BEGIN 2 */

  //FIR Implementation
  //uint32_t block_size = BUFFER_SIZE;
  arm_fir_init_f32(&S,NUM_TAPS,(float32_t*)&FIRCoeffs[0],(float32_t*)&FIRStateF32[0],BUFFER_SIZE);

  /* USER CODE END 2 */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  xSemaphore = xSemaphoreCreateMutex();
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  Messages_RX_UART = xQueueCreate(10,sizeof(uint8_t));
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 128);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* definition and creation of Send_Data */
  osThreadDef(Send_Data, UART_Send, osPriorityLow, 0, 512);
  Send_DataHandle = osThreadCreate(osThread(Send_Data), NULL);

  /* definition and creation of ADC_Starter */
  osThreadDef(ADC_Starter, ADC_Task, osPriorityLow, 0, 256);
  ADC_StarterHandle = osThreadCreate(osThread(ADC_Starter), NULL);

  /* definition and creation of UART_Read */
  osThreadDef(UART_Read, Read_Data, osPriorityLow, 0, 256);
  UART_ReadHandle = osThreadCreate(osThread(UART_Read), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 120;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 5;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC3_Init(void)
{

  /* USER CODE BEGIN ADC3_Init 0 */

  /* USER CODE END ADC3_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC3_Init 1 */

  /* USER CODE END ADC3_Init 1 */
  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc3.Instance = ADC3;
  hadc3.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
  hadc3.Init.Resolution = ADC_RESOLUTION_12B;
  hadc3.Init.ScanConvMode = DISABLE;
  hadc3.Init.ContinuousConvMode = ENABLE;
  hadc3.Init.DiscontinuousConvMode = DISABLE;
  hadc3.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc3.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc3.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc3.Init.NbrOfConversion = 1;
  hadc3.Init.DMAContinuousRequests = ENABLE;
  hadc3.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc3) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_14;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_28CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc3, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC3_Init 2 */

  /* USER CODE END ADC3_Init 2 */

}

/**
  * @brief RTC Initialization Function
  * @param None
  * @retval None
  */
static void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */
  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 576000;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}

/**
  * @brief USB_OTG_FS Initialization Function
  * @param None
  * @retval None
  */
static void MX_USB_OTG_FS_PCD_Init(void)
{

  /* USER CODE BEGIN USB_OTG_FS_Init 0 */

  /* USER CODE END USB_OTG_FS_Init 0 */

  /* USER CODE BEGIN USB_OTG_FS_Init 1 */

  /* USER CODE END USB_OTG_FS_Init 1 */
  hpcd_USB_OTG_FS.Instance = USB_OTG_FS;
  hpcd_USB_OTG_FS.Init.dev_endpoints = 4;
  hpcd_USB_OTG_FS.Init.speed = PCD_SPEED_FULL;
  hpcd_USB_OTG_FS.Init.dma_enable = DISABLE;
  hpcd_USB_OTG_FS.Init.phy_itface = PCD_PHY_EMBEDDED;
  hpcd_USB_OTG_FS.Init.Sof_enable = ENABLE;
  hpcd_USB_OTG_FS.Init.low_power_enable = DISABLE;
  hpcd_USB_OTG_FS.Init.lpm_enable = DISABLE;
  hpcd_USB_OTG_FS.Init.vbus_sensing_enable = ENABLE;
  hpcd_USB_OTG_FS.Init.use_dedicated_ep1 = DISABLE;
  if (HAL_PCD_Init(&hpcd_USB_OTG_FS) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USB_OTG_FS_Init 2 */

  /* USER CODE END USB_OTG_FS_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA2_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);

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
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOG, CTRL_Pin_Pin|USB_PowerSwitchOn_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LED3_Pin|LED4_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : RMII_MDC_Pin RMII_RXD0_Pin RMII_RXD1_Pin */
  GPIO_InitStruct.Pin = RMII_MDC_Pin|RMII_RXD0_Pin|RMII_RXD1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : LED2_Pin */
  GPIO_InitStruct.Pin = LED2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED2_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : RMII_REF_CLK_Pin RMII_MDIO_Pin RMII_CRS_DV_Pin */
  GPIO_InitStruct.Pin = RMII_REF_CLK_Pin|RMII_MDIO_Pin|RMII_CRS_DV_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : LED1_Pin */
  GPIO_InitStruct.Pin = LED1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : CTRL_Pin_Pin USB_PowerSwitchOn_Pin */
  GPIO_InitStruct.Pin = CTRL_Pin_Pin|USB_PowerSwitchOn_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

  /*Configure GPIO pins : LED3_Pin LED4_Pin */
  GPIO_InitStruct.Pin = LED3_Pin|LED4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : RMII_TXD1_Pin */
  GPIO_InitStruct.Pin = RMII_TXD1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
  HAL_GPIO_Init(RMII_TXD1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : USB_OverCurrent_Pin */
  GPIO_InitStruct.Pin = USB_OverCurrent_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(USB_OverCurrent_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : RMII_TX_EN_Pin RMII_TXD0_Pin */
  GPIO_InitStruct.Pin = RMII_TX_EN_Pin|RMII_TXD0_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* AdcHandle){

	char lo = 0;
	char high = 0;

	if(ADC_60_90 == true){
		inputF32 = &ADC3ConvertedBuffer[0];
		outputF32 = &ADC3ConvertedBuffer_Filtered[0];

		for(int counterFilter = 0; counterFilter < (BUFFER_SIZE/2);counterFilter++){
		//signal_in = (float32_t) ADC3ConvertedBuffer[counterFilter];
		//arm_fir_f32(&S,inputF32+(counterFilter * blocksize),outputF32+(counterFilter * blocksize),(BUFFER_SIZE/2));
		arm_fir_f32(&S,inputF32,outputF32,(BUFFER_SIZE/2));
		//ADC3ConvertedBuffer_Filtered[counterFilter] = (uint32_t)signal_out;
		}

		for(int i = 0 ;i < (BUFFER_SIZE/2);i++){
			lo = ADC3ConvertedBuffer_Filtered[i] & 0xFF;
			high = ADC3ConvertedBuffer_Filtered[i] >>8;
			HAL_UART_Transmit(&huart3,(uint8_t*)&lo,1,100);
			HAL_UART_Transmit(&huart3,(uint8_t*)&high,1,100);
			//HAL_UART_Transmit(&huart3,"\r\n",2,100);
			HAL_GPIO_TogglePin(LED2_GPIO_Port,LED2_Pin);
		}
	//HAL_GPIO_TogglePin(GPIOB,LED3_Pin);
	//ADC_Ended = true;
	}
	else{
		for(int i = 0 ;i < (BUFFER_SIZE/2);i++){
			lo = ADC3ConvertedBuffer[i] & 0xFF;
			high = ADC3ConvertedBuffer[i] >>8;
			HAL_UART_Transmit(&huart3,(uint8_t*)&lo,1,100);
			HAL_UART_Transmit(&huart3,(uint8_t*)&high,1,100);
			//HAL_UART_Transmit(&huart3,"\r\n",2,100);
			HAL_GPIO_TogglePin(LED2_GPIO_Port,LED2_Pin);
			}
	}

}



void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* AdcHandle){

	char lo = 0;
	char high = 0;

	if(ADC_60_90 == true){
		inputF32 = &ADC3ConvertedBuffer[BUFFER_SIZE/2];
		outputF32 = &ADC3ConvertedBuffer_Filtered[BUFFER_SIZE/2];

		for(int counterFilter = (BUFFER_SIZE/2); counterFilter < BUFFER_SIZE;counterFilter++){
			//signal_in = (float32_t) ADC3ConvertedBuffer[counterFilter];
			//arm_fir_f32(&S,inputF32+(counterFilter * blocksize),outputF32+(counterFilter * blocksize),(BUFFER_SIZE/2));
		arm_fir_f32(&S,inputF32,outputF32,(BUFFER_SIZE/2));
			//ADC3ConvertedBuffer_Filtered[counterFilter] = (uint32_t)signal_out;
		}

		for(int i = 0 ;i < (BUFFER_SIZE/2);i++){
			lo = ADC3ConvertedBuffer_Filtered[i] & 0xFF;
			high = ADC3ConvertedBuffer_Filtered[i] >>8;
			HAL_UART_Transmit(&huart3,(uint8_t*)&lo,1,100);
			HAL_UART_Transmit(&huart3,(uint8_t*)&high,1,100);
			//HAL_UART_Transmit(&huart3,"\r\n",2,100);
			HAL_GPIO_TogglePin(LED2_GPIO_Port,LED2_Pin);
			}
	}
	else{
		for(int i = (BUFFER_SIZE/2);i < BUFFER_SIZE;i++){
			lo = ADC3ConvertedBuffer[i] & 0xFF;
			high = ADC3ConvertedBuffer[i] >>8;
			HAL_UART_Transmit(&huart3,(uint8_t*)&lo,1,100);
			HAL_UART_Transmit(&huart3,(uint8_t*)&high,1,100);
			//HAL_UART_Transmit(&huart3,"\r\n",2,100);
			HAL_GPIO_TogglePin(LED2_GPIO_Port,LED2_Pin);
			}
	}
}



void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart){



}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	//static BaseType_t pxHigherPriorityTaskWoken;
    char temp_some;

    temp_some = UART_RX_BUFFER[0];

    if(temp_some == 2){
   	  		  HAL_GPIO_WritePin(GPIOG,CTRL_Pin_Pin,SET);
   	  		  ADC_60_90 = true;
   	  	  }
   	  	  else
   	  	  {
   	  		  HAL_GPIO_WritePin(GPIOG,CTRL_Pin_Pin,RESET);
   	  	//ADC_60_90 = false;
   	  	  }
   	  	// Pino ativo, A cadeia de 60k-90k fica ativa

   // xQueueSendToBackFromISR(Messages_RX_UART,&temp_some,&pxHigherPriorityTaskWoken);
    //if( pxHigherPriorityTaskWoken == pdTRUE )
    //	taskYIELD(); /* Forces a context switch before exit the ISR. */


}


/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
  /* USER CODE BEGIN 5 */
  /* Infinite loop */
  for(;;)
  {
	vTaskDelay(1000);
	HAL_GPIO_TogglePin(LED1_GPIO_Port,LED1_Pin);
    osDelay(1);
  }
  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_UART_Send */
/**
* @brief Function implementing the Send_Data thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_UART_Send */
void UART_Send(void const * argument)
{
  /* USER CODE BEGIN UART_Send */

	//uint16_t temp = 24502;
	//char temp_2[] = "Muito";
	//char lo;
	//char high;

  /* Infinite loop */
  for(;;)
  {

    osDelay(1);
  }
  /* USER CODE END UART_Send */
}

/* USER CODE BEGIN Header_ADC_Task */
/**
* @brief Function implementing the ADC_Starter thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_ADC_Task */
void ADC_Task(void const * argument)
{
  /* USER CODE BEGIN ADC_Task */
  HAL_ADC_MspInit(&hadc3);
  HAL_ADC_Start_DMA(&hadc3,(uint32_t*)ADC3ConvertedBuffer,BUFFER_SIZE);
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END ADC_Task */
}

/* USER CODE BEGIN Header_Read_Data */
/**
* @brief Function implementing the UART_Read thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Read_Data */
void Read_Data(void const * argument)
{
  /* USER CODE BEGIN Read_Data */
	char temp_value;
	HAL_UART_Receive_IT(&huart3,UART_RX_BUFFER,1);
  /* Infinite loop */
  for(;;)
  {

	 // if(Messages_RX_UART != 0){
	 // 	xQueueReceive(Messages_RX_UART,&temp_value,(TickType_t) portMAX_DELAY);


	  //}
    osDelay(1);
  }
  /* USER CODE END Read_Data */
}

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

