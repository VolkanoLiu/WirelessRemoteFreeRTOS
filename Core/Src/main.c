/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
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
#include "i2c.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "queue.h"
#include "semphr.h"
#include "event_groups.h"
#include "AS69.h"
#include "ssd1306.h"
#include "matrix_key.h"
#include "state_machine.h"
#include "GUI.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct Message
{
  uint16_t addr;
  uint8_t count;
  uint32_t data;
} Message_TypeDef;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* FreeRTOS ------------------------------------------------------------------*/

//osSemaphoreDef(updateScreenSem);
//osThreadId as69TaskHandle;
//osThreadId displayTaskHandle;
//osThreadId keyTaskHandle;

EventGroupHandle_t xUpdateScreenEventGroup;
EventGroupHandle_t xReadyEventGroup;
QueueHandle_t xRecvMessageQueue;
QueueHandle_t xSendMessageQueue;
QueueHandle_t xKeyEventQueue;
TaskHandle_t xAS69TaskHandle;
TaskHandle_t xAS69SendTaskHandle;
TaskHandle_t xDisplayTaskHandle;
TaskHandle_t xMatrixTaskHandle;
//TaskHandle_t xMessageRouterTaskHandle;
TaskHandle_t xGUITaskHandle;

void UART_RxCpltCallback()
{
  BaseType_t xYieldRequired;
  xYieldRequired = xTaskResumeFromISR(xAS69TaskHandle);
  portYIELD_FROM_ISR(xYieldRequired);
}
void vAS69Task(void* arg)
{
  UART_HandleTypeDef* huart = (UART_HandleTypeDef*)arg;
  taskENTER_CRITICAL();
  AS69_Init(huart);
  taskEXIT_CRITICAL();
  
  uint8_t buffer;
  CommStateMachine_TypeDef state = Comm_HEAD;
  const uint8_t head[] = { 0xde, 0xad };
  const uint8_t addr_bytes = 2;
  const uint8_t len_bytes = 1;
  uint16_t addr = 0x0000;
  uint8_t len = 0;
  uint8_t count = 0;
  uint8_t byte_count = 0;
  uint32_t msg_buffer = 0;
  huart->RxCpltCallback = UART_RxCpltCallback;
  xEventGroupSetBits(xReadyEventGroup, BIT_AS69_READY);
  for (;;)
  {
    HAL_UART_Receive_IT(huart, &buffer, 1);
    vTaskSuspend(xAS69TaskHandle);
    switch (state)
    {
    case Comm_HEAD: {
      if (buffer == head[len])
      {
        count++;
        if (count == sizeof(head))
        {
          addr = 0;
          count = 0;
          msg_buffer = 0;
          state = Comm_ADDR;
        }
      }
      break;
    }
    case Comm_ADDR: {
      addr = addr + (buffer << (count << 3));
      count++;
      if (count == addr_bytes)
      {
        len = 0;
        count = 0;
        msg_buffer = 0;
        state = Comm_LEN;
      }
      break;
    }
    case Comm_LEN: {
      len = len + (buffer << (count << 3));
      count++;
      if (count == len_bytes)
      {
        msg_buffer = 0;
        count = 0;
        byte_count = 0;
        msg_buffer = 0;
        state = Comm_LEN;
      }
      break;
    }
    case Comm_MSG: {
      msg_buffer = msg_buffer + (buffer << (byte_count << 3));
      byte_count++;
      if (byte_count == 4)
      {
        // TODO: Send message to somewhere
        Message_TypeDef msg = { 
          .addr = addr,
          .count = count,
          .data = msg_buffer
        };
        if (xQueueSend(xRecvMessageQueue,
                        (void *) &msg,
                        (TickType_t) 20 != pdTRUE))
        {
          // TODO: Do error check
        }
        else
        {
          xEventGroupSetBits(xReadyEventGroup, BIT_RECV_READY);
        }
        msg_buffer = 0;
        byte_count = 0;
        count++;
      }
      if (count == len)
      {
        count = 0;
        state = Comm_HEAD;
      }
      break;
    }
    default:
      break;
    }

  }
}

void vAS69SendTask(void* arg)
{
  xEventGroupWaitBits(xReadyEventGroup, BIT_AS69_READY, pdFALSE, pdTRUE, portMAX_DELAY);
  for (;;)
  {
    Message_TypeDef msg;
    if (xQueueReceive(xSendMessageQueue, &msg, portMAX_DELAY) == pdTRUE)
    {
      // TODO
    }
  }
}

void vDisplayTask(void* arg)
{
  I2C_HandleTypeDef* hi2c = (I2C_HandleTypeDef*)arg;
  ssd1306_Init(hi2c);
  portENTER_CRITICAL();
  ssd1306_UpdateScreen(hi2c);
  portEXIT_CRITICAL();
  xEventGroupSetBits(xReadyEventGroup, BIT_SSD1306_READY);
  for (;;)
  {
    xEventGroupWaitBits(xUpdateScreenEventGroup, 0xff, pdTRUE, pdFALSE, portMAX_DELAY);
    // portENTER_CRITICAL();
    ssd1306_UpdateScreen(hi2c);
    // portEXIT_CRITICAL();
  }
}

matrix_key_Typedef matrix_key;
FSMKey_Typedef keys[4][4];
void vMatrixTask(void* arg)
{
  xEventGroupWaitBits(xReadyEventGroup, BIT_SSD1306_READY, pdFALSE, pdTRUE, portMAX_DELAY);
  void(*SingleHit_callback[4][4])(void);
  SingleHit_callback[0][0] = NULL;
  SingleHit_callback[0][1] = NULL;
  SingleHit_callback[0][2] = NULL;
  SingleHit_callback[0][3] = NULL;
  SingleHit_callback[1][0] = NULL;
  SingleHit_callback[1][1] = NULL;
  SingleHit_callback[1][2] = NULL;
  SingleHit_callback[1][3] = NULL;
  SingleHit_callback[2][0] = NULL;
  SingleHit_callback[2][1] = NULL;
  SingleHit_callback[2][2] = NULL;
  SingleHit_callback[2][3] = NULL;
  SingleHit_callback[3][0] = NULL;
  SingleHit_callback[3][1] = NULL;
  SingleHit_callback[3][2] = NULL;
  SingleHit_callback[3][3] = NULL;
  void(*DoubleHit_callback[4][4])(void);
  DoubleHit_callback[0][0] = NULL;
  DoubleHit_callback[0][1] = NULL;
  DoubleHit_callback[0][2] = NULL;
  DoubleHit_callback[0][3] = NULL;
  DoubleHit_callback[1][0] = NULL;
  DoubleHit_callback[1][1] = NULL;
  DoubleHit_callback[1][2] = NULL;
  DoubleHit_callback[1][3] = NULL;
  DoubleHit_callback[2][0] = NULL;
  DoubleHit_callback[2][1] = NULL;
  DoubleHit_callback[2][2] = NULL;
  DoubleHit_callback[2][3] = NULL;
  DoubleHit_callback[3][0] = NULL;
  DoubleHit_callback[3][1] = NULL;
  DoubleHit_callback[3][2] = NULL;
  DoubleHit_callback[3][3] = NULL;
  GPIO_struct_Typedef row[4];
  GPIO_struct_Typedef col[4];
  row[0].GPIOx = ROW0_GPIO_Port; row[0].GPIO_Pin = ROW0_Pin;
  row[1].GPIOx = ROW1_GPIO_Port; row[1].GPIO_Pin = ROW1_Pin;
  row[2].GPIOx = ROW2_GPIO_Port; row[2].GPIO_Pin = ROW2_Pin;
  row[3].GPIOx = ROW3_GPIO_Port; row[3].GPIO_Pin = ROW3_Pin;
  col[0].GPIOx = COL0_GPIO_Port; col[0].GPIO_Pin = COL0_Pin;
  col[1].GPIOx = COL1_GPIO_Port; col[1].GPIO_Pin = COL1_Pin;
  col[2].GPIOx = COL2_GPIO_Port; col[2].GPIO_Pin = COL2_Pin;
  col[3].GPIOx = COL3_GPIO_Port; col[3].GPIO_Pin = COL3_Pin;
  matrixInit(&matrix_key, keys, SingleHit_callback, DoubleHit_callback, row, col);
  
  TickType_t xLastWakeTime;
  const TickType_t xFrequency = 4;
  xLastWakeTime = xTaskGetTickCount(); 
  for (;;)
  {
    scanMatrix(&matrix_key);
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

// TODO: Feature in the future
//void vMessageRouterTask(void* arg)
//{
//  Message_TypeDef msg;
//  for (;;)
//  {
//    if (xQueueReceive(xRecvMessageQueue, &(msg), portMAX_DELAY) == pdPASS)
//    {
//      // TODO
//    }
//  }
//}

#define GUI_VALUE_COUNT 10
GUI_Value_TypeDef values[10];

void vGUITask(void* arg)
{
  xEventGroupWaitBits(xReadyEventGroup, BIT_SSD1306_READY, pdFALSE, pdTRUE, portMAX_DELAY);
  
  values[0].value = 0;
  values[1].value = (uint32_t)0.0f;
  values[2].value = (uint32_t)0.0f;
  values[3].value = (uint32_t)0.0f;
	values[4].value = 0;
	values[5].value = 0;
  
  
  GUI_TypeDef gmRoot, gmUltities, gmPidTuner,gvgTemp, gvsTestVal ,gvsPidP, gvsPidI, gvsPidD, gvsError, gmRegs, gmErrs, gmAbout, gmExample;
  GUI_Init(&gmRoot, "Root", NULL, GUI_MENU, NULL);
  {  
    GUI_Init(&gmUltities, "Ultity", NULL, GUI_MENU, NULL);
    {
      GUI_Init(&gvgTemp, "Temp", (void*)"Deg", GUI_VALUEGET, (void*)(values + 0));
      GUI_Init(&gvsTestVal, "TestVal", (void*)"Unit", GUI_VALUESET, (void*)(values + 5));
      GUI_SetProperties(&gvgTemp, guiINT | guiSCREEN_SYNC | guiENABLED);
      GUI_SetProperties(&gvsTestVal, guiINT | guiINC_CTRL | guiSCREEN_SYNC | guiENABLED);
      GUI_AddChild(&gmUltities, &gvgTemp);
      GUI_AddChild(&gmUltities, &gvsTestVal);
    }
    GUI_Init(&gmPidTuner, "PIDTune", NULL, GUI_MENU, NULL);
    {
      GUI_Init(&gvsPidP, "P", (void*)"x", GUI_VALUESET, (void*)(values + 1));
      GUI_Init(&gvsPidI, "I", (void*)"x", GUI_VALUESET, (void*)(values + 2));
      GUI_Init(&gvsPidD, "D", (void*)"x", GUI_VALUESET, (void*)(values + 3));
			GUI_Init(&gvsError, "Error", (void*)"x", GUI_VALUESET, (void*)(values + 4));
      GUI_SetProperties(&gvsPidP, guiFLOAT | guiENABLED);
      GUI_SetProperties(&gvsPidI, guiFLOAT | guiENABLED);
      GUI_SetProperties(&gvsPidD, guiFLOAT | guiENABLED);
			GUI_SetProperties(&gvsError, guiBOOL | guiINC_CTRL |guiENABLED);
      GUI_AddChild(&gmPidTuner, &gvsPidP);
      GUI_AddChild(&gmPidTuner, &gvsPidI);
      GUI_AddChild(&gmPidTuner, &gvsPidD);
			GUI_AddChild(&gmPidTuner, &gvsError);
    }
    GUI_Init(&gmRegs, "Regs", NULL, GUI_MENU, NULL);
    GUI_Init(&gmErrs, "Errors", NULL, GUI_MENU, NULL);
    GUI_Init(&gmAbout, "Aboutme", NULL, GUI_MENU, NULL);
    GUI_Init(&gmExample, "Example", NULL, GUI_MENU, NULL);
    GUI_SetProperties(&gmUltities, guiENABLED);
    GUI_SetProperties(&gmPidTuner, guiENABLED);
    GUI_SetProperties(&gmRegs, guiENABLED);
    GUI_SetProperties(&gmErrs, guiENABLED);
    GUI_SetProperties(&gmAbout, guiENABLED);
    GUI_SetProperties(&gmExample, guiENABLED);
    GUI_AddChild(&gmRoot, &gmUltities);
    GUI_AddChild(&gmRoot, &gmPidTuner);
    GUI_AddChild(&gmRoot, &gmRegs);
    GUI_AddChild(&gmRoot, &gmErrs);
    GUI_AddChild(&gmRoot, &gmAbout);
    GUI_AddChild(&gmRoot, &gmExample);
  }
  GUI_SetProperties(&gmRoot, guiENABLED);
  
  // TODO
  GUI_TypeDef* current_menu = &gmRoot;
  GUI_Render(current_menu);
  xEventGroupSetBits(xReadyEventGroup, BIT_SSD1306_READY);
  xEventGroupSetBits(xUpdateScreenEventGroup, BIT_OLED_UPDATE);
  
  KeyEvent_TypeDef event;
  Message_TypeDef msg;
  for (;;)
  {
    uint32_t bits = xEventGroupWaitBits(xReadyEventGroup, BIT_KEY_READY | BIT_RECV_READY, pdFALSE, pdFALSE, portMAX_DELAY);
    if ((bits & BIT_RECV_READY) && xQueueReceive(xRecvMessageQueue, &(msg), portMAX_DELAY) == pdTRUE)
    {
      if (!uxQueueMessagesWaiting(xRecvMessageQueue))
      {
        xEventGroupClearBits(xReadyEventGroup, BIT_RECV_READY);
      }
      if (msg.addr < GUI_VALUE_COUNT)
      {
        GUI_Value_TypeDef* pValue = values + msg.addr;
        pValue->value = msg.data;
        if (current_menu == pValue->associated_gui || current_menu == pValue->associated_gui->parent)
        {
          GUI_Render(current_menu);
          xEventGroupSetBits(xUpdateScreenEventGroup, BIT_OLED_UPDATE);
        }
      }
    }
    if ((bits & BIT_KEY_READY) && xQueueReceive(xKeyEventQueue, &event, portMAX_DELAY) == pdTRUE)
    {
      if (!uxQueueMessagesWaiting(xKeyEventQueue))
      {
        xEventGroupClearBits(xReadyEventGroup, BIT_KEY_READY);
      }
      current_menu = GUI_KeyProcess(current_menu, &event);
      GUI_Render(current_menu);
      xEventGroupSetBits(xUpdateScreenEventGroup, BIT_OLED_UPDATE);
    }
  }
}

//osThreadDef(vAS69Task, osPriorityNormal, 0, 2048);

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
  MX_I2C1_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */

//  as69TaskHandle = osThreadCreate(osThread(vAS69Task), (void const*)(&huart1));
  xUpdateScreenEventGroup = xEventGroupCreate();
  xReadyEventGroup = xEventGroupCreate();
  xRecvMessageQueue = xQueueCreate(10, sizeof(Message_TypeDef));
  xSendMessageQueue = xQueueCreate(10, sizeof(Message_TypeDef));
  xKeyEventQueue = xQueueCreate(30, sizeof(KeyEvent_TypeDef));
  xTaskCreate(vAS69Task, "Wireless serial task", configMINIMAL_STACK_SIZE, (void*)(&huart1), 3, &xAS69TaskHandle);
  xTaskCreate(vAS69SendTask, "Send task", configMINIMAL_STACK_SIZE, (void*)(&huart1), 2, &xAS69SendTaskHandle);
  xTaskCreate(vDisplayTask, "OLED task", configMINIMAL_STACK_SIZE * 2, (void*)(&hi2c1), 2, &xDisplayTaskHandle);
  xTaskCreate(vMatrixTask, "Matrix keys task", configMINIMAL_STACK_SIZE, NULL, 4, &xMatrixTaskHandle);
//  xTaskCreate(vMessageRouterTask, "Message queue process", configMINIMAL_STACK_SIZE, NULL, 1, &xMessageRouterTaskHandle);
  xTaskCreate(vGUITask, "GUI", configMINIMAL_STACK_SIZE * 3, NULL, 1, &xGUITaskHandle);
  
  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();  /* Call init function for freertos objects (in freertos.c) */
  MX_FREERTOS_Init();
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

 /**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM4 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
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
