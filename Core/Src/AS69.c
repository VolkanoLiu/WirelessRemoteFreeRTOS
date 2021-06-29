#include "AS69.h"
#include "string.h"

uint8_t AS69_RxBuffer[128];

void AS69_RxCpltCallback()
{
  HAL_UART_Transmit(&huart1, AS69_RxBuffer, 1, 0xFFFF);
  HAL_UART_Receive_IT(&huart1, AS69_RxBuffer, 1);
}

HAL_StatusTypeDef AS69_Init(UART_HandleTypeDef* huart)
{
  uint8_t hw_version[15] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
  HAL_Delay(100);
  
  HAL_GPIO_WritePin(MD0_GPIO_Port, MD0_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(MD1_GPIO_Port, MD1_Pin, GPIO_PIN_SET);
  HAL_UART_Receive(huart, hw_version, 5, 0xFFFF);
  if(strncmp((const char*)hw_version, "START", 5) != 0)
  {
    Error_Handler();
  }
  HAL_Delay(100);
  
  uint8_t msg_hwid[3] = { 0xC3, 0xC3, 0xC3 };
  HAL_UART_Transmit(huart, msg_hwid, 3, 0xFFFF);
  HAL_UART_Receive(huart, hw_version, 15, 0xFFFF);
  if (strncmp((const char*)hw_version, "AS69", 4) != 0)
  {
    Error_Handler();
  }
  
  uint8_t msg_tmpcfg[] = { 0xC0, 0x12, 0x34, 0x38, 0x00, 0x40 };
  HAL_UART_Transmit(huart, msg_tmpcfg, 6, 0xFFFF);
  HAL_UART_Receive(huart, hw_version, 4, 0xFFFF);
  if (strncmp((const char*)hw_version, "OK", 2) != 0)
  {
    Error_Handler();
  }
  
  HAL_GPIO_WritePin(MD0_GPIO_Port, MD0_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(MD1_GPIO_Port, MD1_Pin, GPIO_PIN_RESET);
  while (HAL_GPIO_ReadPin(AUX_GPIO_Port, AUX_Pin)) ;
  while (!HAL_GPIO_ReadPin(AUX_GPIO_Port, AUX_Pin)) ;

  huart->Init.BaudRate = 115200;
  huart->RxCpltCallback = AS69_RxCpltCallback;
  if (HAL_UART_Init(huart) != HAL_OK)
  {
    Error_Handler();
  }
  
  HAL_Delay(100);
  return HAL_OK;
}

