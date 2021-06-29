#ifndef __MATRIX_KEY_H
#define __MATRIX_KEY_H

#include "main.h"
#include "keypress_process.h"

// 定义矩阵键盘的行和列
#define MATRIX_ROW 4
#define MATRIX_COL 4

// GPIO引脚结构体定义
typedef struct {
  GPIO_TypeDef *GPIOx;
  uint16_t GPIO_Pin;
} GPIO_struct_Typedef;

// 矩阵键盘结构提定义
typedef struct {
  KeyGroup_Typedef keyGroup_row[MATRIX_ROW];
} matrix_key_Typedef;



// 初始化矩阵键盘
void matrixInit(matrix_key_Typedef *matrix,
    FSMKey_Typedef key[MATRIX_ROW][MATRIX_COL],
    GPIO_struct_Typedef row[MATRIX_ROW],
    GPIO_struct_Typedef col[MATRIX_COL]);

// 扫描矩阵键盘
void scanMatrix(matrix_key_Typedef* m);

#endif