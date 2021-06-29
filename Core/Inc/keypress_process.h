#ifndef __KEYPRESS_PROCESS_H
#define __KEYPRESS_PROCESS_H

#include "main.h"

/*
 * 基于有限状态机(FSM)的独立按键库
 * 
 * 重写自: Jayant Tang
 * 
 * 重写者: Volkano Liu
 * 
 * TODO
 */

// 按键状态枚举
typedef enum {
  NORMAL = 0,
  PRE_ACTIVE,
  PRE_HIT,
  PRE_LONGPRESS,
  SINGLE_HIT,
  DOUBLE_HIT,
  LONGPRESS
} KeyState_Typedef;

// 按键结构体
struct FSMKey
{
  // 按键GPIO配置(矩阵键盘需进行部分魔改)
  GPIO_TypeDef *GPIOx;
  uint16_t GPIO_Pin;
  void (*singleHit_callback)(void);
  void (*doubleHit_callback)(void);
  uint8_t polarity;
  uint16_t row;
  uint16_t col;

  uint8_t buffer;         // 仅低四位有效
  KeyState_Typedef state; // 按键状态
  uint16_t counter;       // 用于处理双击和长按的计数器
  struct FSMKey *next;    // 链表结构，指向下一个按键的索引
};

typedef struct FSMKey FSMKey_Typedef;

// 按键组结构体
typedef struct {
  FSMKey_Typedef *HEAD;
  FSMKey_Typedef *TAIL;
}KeyGroup_Typedef;

typedef struct KeyEvent
{
  uint16_t row;
  uint16_t col;
  KeyState_Typedef state;
} KeyEvent_TypeDef;

// 初始化一个按键并将其加入按键组
void keyInit(KeyGroup_Typedef *keyGroup,
    FSMKey_Typedef *key,
    GPIO_TypeDef *GPIOx,
    uint16_t GPIO_Pin,
    void (*SingleHit_callback)(void),
    void (*DoubleHit_callback)(void),
    uint8_t polarity,
    uint16_t row,
    uint16_t col);

// 初始化按键组
void keyGroupInit(KeyGroup_Typedef *keyGroup);

// 扫描按键状态
void keyScan(FSMKey_Typedef *key);
void keyScanAll(KeyGroup_Typedef *keyGroup);

#endif