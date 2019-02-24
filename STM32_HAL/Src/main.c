/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether 
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * Copyright (c) 2018 STMicroelectronics International N.V. 
  * All rights reserved.
  *
  * Redistribution and use in source and binary forms, with or without 
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice, 
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other 
  *    contributors to this software may be used to endorse or promote products 
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this 
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under 
  *    this license is void and will automatically terminate your rights under 
  *    this license. 
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS" 
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT 
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT 
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usbd_cdc_if.h"
#include "cjson/cJSON.h"
#include "eeprom/eeprom.h"
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
UART_HandleTypeDef huart1;

osThreadId defaultTaskHandle;
osThreadId LubeLimitCorrecHandle;
osThreadId BLETransmitTaskHandle;
osThreadId ledBlinkHandle;
/* USER CODE BEGIN PV */

uint16_t VirtAddVarTab[NB_OF_VAR] = {0, 1, 2};

uint8_t rx_data = 0;
uint8_t rx_buffer[32]; // where we build our string from characters coming in
int rx_index = 0; // index for going though rxString

const float lenghtWheel = 1.96;		//Длинна окружности колеса
const int ble_delay = 50;

uint16_t LubeCount;
uint16_t LubeDelay;
uint16_t WheelRotateCount = 0;
uint16_t WheelRotateCountPrev;
uint16_t WheelRotateLimitBase;
double WheelRotateLimit;
double WheelSpeed;
uint32_t SpeedDelay = 5;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
void StartDefaultTask(void const * argument);
void StartLubeLimitCorrectTask(void const * argument);
void StartBLETransmitTask(void const * argument);
void StartLedBlink(void const * argument);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

char *JSON_Transmit(char *name, uint16_t value) {
    char *str_json = NULL;
    cJSON *obj_json = cJSON_CreateObject();
    char str[20];
    sprintf(str, "%d", value);
    cJSON_AddStringToObject(obj_json, name, str);
    str_json = cJSON_PrintUnformatted(obj_json);
    sprintf(str_json, "%s\r\n", str_json);
    cJSON_Delete(obj_json);
    return str_json;
}


void JSON_Receive(const char * const s_json) {
    const cJSON *j_LubeDelay = NULL;
    const cJSON *j_WheelRotateLimitBase = NULL;
    cJSON *o_json = cJSON_Parse(s_json);
    if (o_json != NULL) {
        j_LubeDelay = cJSON_GetObjectItemCaseSensitive(o_json, "LD");
        if (j_LubeDelay != NULL) {
            LubeDelay = (uint16_t) j_LubeDelay->valueint;
            EE_WriteVariable(VirtAddVarTab[2], LubeDelay);
            cJSON_Delete(o_json);
        }

        j_WheelRotateLimitBase = cJSON_GetObjectItemCaseSensitive(o_json, "WRLB");
        if (j_WheelRotateLimitBase != NULL) {
            WheelRotateLimitBase = (uint16_t) j_WheelRotateLimitBase->valueint;
            EE_WriteVariable(VirtAddVarTab[1], WheelRotateLimitBase);
            cJSON_Delete(o_json);
        }
    }
    cJSON_Delete(o_json);
}


void LubeChain(void) {
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_SET);
    osDelay(LubeDelay);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_RESET);
    LubeCount++;

    EE_WriteVariable(VirtAddVarTab[0], LubeCount);    //Save lube count
}


void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    WheelRotateCount++;
}


void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {

        if (rx_data != 10){
            rx_buffer[rx_index++]= rx_data;
        }
        else {
            JSON_Receive((char*) rx_buffer);
            CDC_Transmit_FS(rx_buffer, (uint16_t) strlen((char *) &rx_buffer));
            rx_index = 0;
            for (int i=0; i<32; i++) rx_buffer[i]=0;
        }

        HAL_UART_Receive_IT (&huart1, &rx_data, 1);     // activate receive

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
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */

    HAL_UART_Receive_IT(&huart1, &rx_data, 1);

    HAL_FLASH_Unlock();
    EE_Init();

    EE_ReadVariable(VirtAddVarTab[0], &LubeCount);
    EE_ReadVariable(VirtAddVarTab[1], &WheelRotateLimitBase);
    EE_ReadVariable(VirtAddVarTab[2], &LubeDelay);

    if (WheelRotateLimitBase == 0)
    {
        EE_WriteVariable(VirtAddVarTab[0], 0);      //Write LubeCount if device clear
        EE_WriteVariable(VirtAddVarTab[1], 2400);   //Write WheelRotateLimitBase if device clear
        EE_WriteVariable(VirtAddVarTab[2], 1600);   //Write LubeDelay if device clear
    }

  /* USER CODE END 2 */

  /* USER CODE BEGIN RTOS_MUTEX */
    /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
    /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
    /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 128);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* definition and creation of LubeLimitCorrec */
  osThreadDef(LubeLimitCorrec, StartLubeLimitCorrectTask, osPriorityIdle, 0, 128);
  LubeLimitCorrecHandle = osThreadCreate(osThread(LubeLimitCorrec), NULL);

  /* definition and creation of BLETransmitTask */
  osThreadDef(BLETransmitTask, StartBLETransmitTask, osPriorityIdle, 0, 128);
  BLETransmitTaskHandle = osThreadCreate(osThread(BLETransmitTask), NULL);

  /* definition and creation of ledBlink */
  osThreadDef(ledBlink, StartLedBlink, osPriorityIdle, 0, 128);
  ledBlinkHandle = osThreadCreate(osThread(ledBlink), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
    /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_QUEUES */
    /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */
 

  /* Start scheduler */
  osKernelStart();
  
  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
    while (1) {
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /**Initializes the CPU, AHB and APB busses clocks 
  */
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
  /**Initializes the CPU, AHB and APB busses clocks 
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
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLL_DIV1_5;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
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
  huart1.Init.BaudRate = 115200;
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

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PA1 */
  GPIO_InitStruct.Pin = GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PA2 PA3 PA4 PA5 */
  GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PB11 */
  GPIO_InitStruct.Pin = GPIO_PIN_11;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI1_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);

}

/* USER CODE BEGIN 4 */


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
  /* init code for USB_DEVICE */
  MX_USB_DEVICE_Init();

  /* USER CODE BEGIN 5 */

    /* Infinite loop */
    for (;;) {
        if (WheelRotateCount > WheelRotateLimit) {
            WheelRotateCount = 0;
            WheelRotateCountPrev = 0;
            LubeChain();
        }
        osDelay(1);
    }
   
  /* USER CODE END 5 */ 
}

/* USER CODE BEGIN Header_StartLubeLimitCorrectTask */
/**
* @brief Function implementing the LubeLimitCorrec thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartLubeLimitCorrectTask */
void StartLubeLimitCorrectTask(void const * argument)
{
  /* USER CODE BEGIN StartLubeLimitCorrectTask */
  /* Infinite loop */
  for(;;)
  {
      WheelSpeed = ((WheelRotateCount - WheelRotateCountPrev) * lenghtWheel / SpeedDelay) * 3600 / 1000;
      WheelRotateLimit = WheelRotateLimitBase;
      if (WheelSpeed > 100) WheelRotateLimit = WheelRotateLimitBase * 0.95;
      if (WheelSpeed > 140) WheelRotateLimit = WheelRotateLimitBase * 0.9;
      if (WheelSpeed > 180) WheelRotateLimit = WheelRotateLimitBase * 0.85;
      if (WheelSpeed > 210) WheelRotateLimit = WheelRotateLimitBase * 0.8;
      if (WheelSpeed > 240) WheelRotateLimit = WheelRotateLimitBase * 0.75;
      WheelRotateCountPrev = WheelRotateCount;

      osDelay(SpeedDelay*1000);
  }
  /* USER CODE END StartLubeLimitCorrectTask */
}

/* USER CODE BEGIN Header_StartBLETransmitTask */
/**
* @brief Function implementing the BLETransmitTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartBLETransmitTask */
void StartBLETransmitTask(void const * argument)
{
  /* USER CODE BEGIN StartBLETransmitTask */
  /* Infinite loop */

  unsigned char *ble_transmit;

  for(;;)
  {
      ble_transmit = (unsigned char *) JSON_Transmit("LD", LubeDelay);
      CDC_Transmit_FS(ble_transmit, (uint16_t) strlen((char *) ble_transmit));
      HAL_UART_Transmit_IT(&huart1, ble_transmit, (uint16_t) strlen((char *) ble_transmit));
      free(ble_transmit);
      osDelay(ble_delay);

      ble_transmit = (unsigned char *) JSON_Transmit("LC", LubeCount);
      CDC_Transmit_FS(ble_transmit, (uint16_t) strlen((char *) ble_transmit));
      HAL_UART_Transmit_IT(&huart1, ble_transmit, (uint16_t) strlen((char *) ble_transmit));
      free(ble_transmit);
      osDelay(ble_delay);

      ble_transmit = (unsigned char *) JSON_Transmit("WRC", WheelRotateCount);
      CDC_Transmit_FS(ble_transmit, (uint16_t) strlen((char *) ble_transmit));
      HAL_UART_Transmit_IT(&huart1, ble_transmit, (uint16_t) strlen((char *) ble_transmit));
      free(ble_transmit);
      osDelay(ble_delay);

      ble_transmit = (unsigned char *) JSON_Transmit("WRL", (uint16_t) WheelRotateLimit);
      CDC_Transmit_FS(ble_transmit, (uint16_t) strlen((char *) ble_transmit));
      HAL_UART_Transmit_IT(&huart1, ble_transmit, (uint16_t) strlen((char *) ble_transmit));
      free(ble_transmit);
      osDelay(ble_delay);

      ble_transmit = (unsigned char *) JSON_Transmit("SP", (uint16_t) WheelSpeed);
      CDC_Transmit_FS(ble_transmit, (uint16_t) strlen((char *) ble_transmit));
      HAL_UART_Transmit_IT(&huart1, ble_transmit, (uint16_t) strlen((char *) ble_transmit));
      free(ble_transmit);
      osDelay(ble_delay);
  }
  /* USER CODE END StartBLETransmitTask */
}

/* USER CODE BEGIN Header_StartLedBlink */
/**
* @brief Function implementing the ledBlink thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartLedBlink */
void StartLedBlink(void const * argument)
{
  /* USER CODE BEGIN StartLedBlink */
  /* Infinite loop */

  for(;;)
  {
      HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
      osDelay(1000);
  }
  /* USER CODE END StartLedBlink */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */

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
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
