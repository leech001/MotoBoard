/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
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
#include "tiny-json/tiny-json.h"
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
osThreadId LLCTaskHandle;
osThreadId BLETrTaskHandle;
osThreadId LBTaskHandle;
/* USER CODE BEGIN PV */

uint16_t VirtAddVarTab[NB_OF_VAR] = {0, 1, 2};

uint8_t rx_data = 0;
uint8_t rx_buffer[32];  // where we build our string from characters coming in
int rx_index = 0;       // index for going though rxString

const float lenghtWheel = 1.96;        //Длинна окружности колеса
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

void StartDefaultTask(void const *argument);

void StartLLCTask(void const *argument);

void StartBLETrTask(void const *argument);

void StartLBTask(void const *argument);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void JSON_Transmit(char *name, uint16_t value) {
    char ble_transmit[20] = {0};
    sprintf(ble_transmit, "%s%s%s%d%s", "{\"", name, "\" : ", value, "}\r\n");
    CDC_Transmit_FS((unsigned char *) ble_transmit, (uint16_t) strlen((char *) ble_transmit));
    HAL_UART_Transmit_IT(&huart1, (unsigned char *) ble_transmit, (uint16_t) strlen((char *) ble_transmit));
    osDelay(ble_delay);
}


void JSON_Receive(char *s_string) {
    json_t pool[2];
    char s_json[20] = {0};
    sprintf(s_json, "%s", s_string);
    json_t const *parent = json_create(s_json, pool, 2);

    json_t const *field = json_getProperty(parent, "LD");
    if (json_getType(field) == JSON_INTEGER) {
        LubeDelay = (uint16_t) json_getInteger(field);
    }

    field = json_getProperty(parent, "WRLB");
    if (json_getType(field) == JSON_INTEGER) {
        WheelRotateLimitBase = (uint16_t) json_getInteger(field);
    }
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

    if (rx_data != 10) {
        rx_buffer[rx_index++] = rx_data;
    } else {
        JSON_Receive((char *) rx_buffer);
        CDC_Transmit_FS(rx_buffer, (uint16_t) strlen((char *) &rx_buffer));
        rx_index = 0;
        for (int i = 0; i < 32; i++) rx_buffer[i] = 0;
    }

    HAL_UART_Receive_IT(&huart1, &rx_data, 1);     // activate receive

}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void) {
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

    if (WheelRotateLimitBase == 0) {
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

    /* USER CODE BEGIN RTOS_QUEUES */
    /* add queues, ... */
    /* USER CODE END RTOS_QUEUES */

    /* Create the thread(s) */
    /* definition and creation of defaultTask */
    osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 128);
    defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

    /* definition and creation of LLCTask */
    osThreadDef(LLCTask, StartLLCTask, osPriorityIdle, 0, 128);
    LLCTaskHandle = osThreadCreate(osThread(LLCTask), NULL);

    /* definition and creation of BLETrTask */
    osThreadDef(BLETrTask, StartBLETrTask, osPriorityIdle, 0, 128);
    BLETrTaskHandle = osThreadCreate(osThread(BLETrTask), NULL);

    /* definition and creation of LBTask */
    osThreadDef(LBTask, StartLBTask, osPriorityIdle, 0, 128);
    LBTaskHandle = osThreadCreate(osThread(LBTask), NULL);

    /* USER CODE BEGIN RTOS_THREADS */
    /* add threads, ... */
    /* USER CODE END RTOS_THREADS */

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
void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    /** Initializes the CPU, AHB and APB busses clocks
    */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }
    /** Initializes the CPU, AHB and APB busses clocks
    */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                  | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
        Error_Handler();
    }
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB;
    PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLL_DIV1_5;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
        Error_Handler();
    }
}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void) {

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
    if (HAL_UART_Init(&huart1) != HAL_OK) {
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
static void MX_GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5, GPIO_PIN_RESET);

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
    GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5;
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
void StartDefaultTask(void const *argument) {
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

/* USER CODE BEGIN Header_StartLLCTask */
/**
* @brief Function implementing the LLCTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartLLCTask */
void StartLLCTask(void const *argument) {
    /* USER CODE BEGIN StartLLCTask */
    /* Infinite loop */
    for (;;) {
        WheelSpeed = ((WheelRotateCount - WheelRotateCountPrev) * lenghtWheel / SpeedDelay) * 3600 / 1000;
        WheelRotateLimit = WheelRotateLimitBase;
        if (WheelSpeed > 240) WheelRotateLimit *= 0.75;
        else if (WheelSpeed > 210) WheelRotateLimit *= 0.8;
        else if (WheelSpeed > 180) WheelRotateLimit *= 0.85;
        else if (WheelSpeed > 140) WheelRotateLimit *= 0.9;
        else if (WheelSpeed > 100) WheelRotateLimit *= 0.95;
        WheelRotateCountPrev = WheelRotateCount;

        osDelay(SpeedDelay * 1000);
    }
    /* USER CODE END StartLLCTask */
}

/* USER CODE BEGIN Header_StartBLETrTask */
/**
* @brief Function implementing the BLETrTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartBLETrTask */
void StartBLETrTask(void const *argument) {
    /* USER CODE BEGIN StartBLETrTask */

    /* Infinite loop */
    for (;;) {
        JSON_Transmit("LD", LubeDelay);
        JSON_Transmit("LC", LubeCount);
        JSON_Transmit("WRC", WheelRotateCount);
        JSON_Transmit("WRL", WheelRotateLimit);
        JSON_Transmit("SP", WheelSpeed);
    }
    /* USER CODE END StartBLETrTask */
}

/* USER CODE BEGIN Header_StartLBTask */
/**
* @brief Function implementing the LBTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartLBTask */
void StartLBTask(void const *argument) {
    /* USER CODE BEGIN StartLBTask */
    /* Infinite loop */
    for (;;) {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        osDelay(100);
    }
    /* USER CODE END StartLBTask */
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM4 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    /* USER CODE BEGIN Callback 0 */

    /* USER CODE END Callback 0 */
    if (htim->Instance == TIM4) {
        HAL_IncTick();
    }
    /* USER CODE BEGIN Callback 1 */

    /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void) {
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
