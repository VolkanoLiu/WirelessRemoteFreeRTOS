#include "matrix_key.h"
#include "main.h"
#include "usart.h"
#include "ssd1306.h"

/**
  * @brief 初始化矩阵键盘
  * @param matrix 按键矩阵结构体
  * @param key 按键二维数组，用于存放按键的相关参数
  * @param SingleHit_callback 单击事件回调函数数组
  * @param DoubleHit_callback 双击事件回调函数数组
  * @param row GPIO的行（引脚为输出）
  * @param col GPIO的列（引脚为输入）
  */
void matrixInit(matrix_key_Typedef *matrix,
    FSMKey_Typedef key[MATRIX_ROW][MATRIX_COL],
    void (*SingleHit_callback[MATRIX_ROW][MATRIX_COL])(void),
    void (*DoubleHit_callback[MATRIX_ROW][MATRIX_COL])(void),
    GPIO_struct_Typedef *row,
    GPIO_struct_Typedef *col)
{
  for(uint8_t row_num = 0; row_num < MATRIX_ROW; row_num++) {
    keyGroupInit(matrix->keyGroup_row + row_num);
    for(uint8_t col_num = 0; col_num < MATRIX_COL; col_num++) {
      keyInit(matrix->keyGroup_row + row_num,
          &key[row_num][col_num],
          (col + col_num)->GPIOx,
          (col + col_num)->GPIO_Pin,
          SingleHit_callback[row_num][col_num],
          DoubleHit_callback[row_num][col_num],
          0,
          row_num,
          col_num);
    }
  }
}

void scanMatrix(matrix_key_Typedef* m)
{
  for(uint32_t row = 0; row < MATRIX_ROW; row++)
  {
    switch (row)
    {
    case 0:
      HAL_GPIO_WritePin(ROW0_GPIO_Port, ROW0_Pin, GPIO_PIN_SET);
      break;
    
    case 1:
      HAL_GPIO_WritePin(ROW1_GPIO_Port, ROW1_Pin, GPIO_PIN_SET);
      break;

    case 2:
      HAL_GPIO_WritePin(ROW2_GPIO_Port, ROW2_Pin, GPIO_PIN_SET);
      break;

    case 3:
      HAL_GPIO_WritePin(ROW3_GPIO_Port, ROW3_Pin, GPIO_PIN_SET);
      break;
    
    default:
      break;
    }
    keyScanAll((m->keyGroup_row) + row);
    HAL_GPIO_WritePin(ROW0_GPIO_Port, ROW0_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(ROW1_GPIO_Port, ROW1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(ROW2_GPIO_Port, ROW2_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(ROW3_GPIO_Port, ROW3_Pin, GPIO_PIN_RESET);
  }
}


//void firstTest(void (*SingleHit_callback[4][4])(void),
//    void (*DoubleHit_callback[4][4])(void),
//    GPIO_struct_Typedef row[4],
//    GPIO_struct_Typedef col[4])
//{
//  SingleHit_callback[0][0]=print1;
//  SingleHit_callback[0][1]=print2;
//  SingleHit_callback[0][2]=print3;
//  SingleHit_callback[1][0]=print4;
//  SingleHit_callback[1][1]=print5;
//  SingleHit_callback[1][2]=print6;
//  SingleHit_callback[2][0]=print7;
//  SingleHit_callback[2][1]=print8;
//  SingleHit_callback[2][2]=print9;
//  SingleHit_callback[3][0]=yes;
//  SingleHit_callback[3][1]=print0;
//  SingleHit_callback[3][2]=no;
//  row[0].GPIOx=ROW0_GPIO_Port;row[0].GPIO_Pin=ROW0_Pin;
//  row[1].GPIOx=ROW1_GPIO_Port;row[1].GPIO_Pin=ROW1_Pin;
//  row[2].GPIOx=ROW2_GPIO_Port;row[2].GPIO_Pin=ROW2_Pin;
//  row[3].GPIOx=ROW3_GPIO_Port;row[3].GPIO_Pin=ROW3_Pin;
//  col[0].GPIOx=COL0_GPIO_Port;col[0].GPIO_Pin=COL0_Pin;
//  col[1].GPIOx=COL1_GPIO_Port;col[1].GPIO_Pin=COL1_Pin;
//  col[2].GPIOx=COL2_GPIO_Port;col[2].GPIO_Pin=COL2_Pin;
//  col[3].GPIOx=COL3_GPIO_Port;col[3].GPIO_Pin=COL3_Pin;
//}

