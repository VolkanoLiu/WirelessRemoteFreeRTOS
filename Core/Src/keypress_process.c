#include "keypress_process.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "event_groups.h"
#include "state_machine.h"

// static const uint8_t key_event_press = 0b0011;
// static const uint8_t key_event_release = 0b1100;
static const uint8_t key_event_press = 0x03;
static const uint8_t key_event_release = 0x0c;

/**
  * @brief 初始化按键
  * @param keyGroup             按键组
  * @param key                  按键
  * @param GPIOx                GPIOx
  * @param GPIO_Pin             GPIO_Pin
  * @param singleHit_callback   单击的回调函数
  * @param doubleHit_callback   双击的回调函数
  * @param polarity             按键极性(0: 上拉, 1:下拉)
  */
void keyInit(KeyGroup_Typedef *keyGroup,
    FSMKey_Typedef *key,
    GPIO_TypeDef *GPIOx,
    uint16_t GPIO_Pin,
    uint8_t polarity,
    uint16_t row,
    uint16_t col)
{
  key->GPIOx = GPIOx;
  key->GPIO_Pin = GPIO_Pin;
  key->polarity = polarity;
  // key->buffer = 0b00000000;
	key->buffer = 0x0;
  key->state = NORMAL;
  key->counter = 0;
  key->row = row;
  key->col = col;

  if(keyGroup->HEAD == NULL) {
    keyGroup->HEAD = key;
    keyGroup->TAIL = key;
  } else {
    keyGroup->TAIL->next = key;
    keyGroup->TAIL = key;
  }
  key->next = NULL;
}

/**
  * @brief 初始化按键组
  * @param keyGroup 按键组
  */
void keyGroupInit(KeyGroup_Typedef *keyGroup)
{
  keyGroup->HEAD = NULL;
  keyGroup->TAIL = NULL;
}

/**
  * @brief 更新按键缓存
  * @param key 按键
  * @param voltage 按键电压采样
  */
void keyFlashBuffer(FSMKey_Typedef *key, uint8_t voltage)
{
  key->buffer = (key->buffer << 1) | (voltage ^ key->polarity);
}

extern QueueHandle_t xKeyEventQueue;
extern EventGroupHandle_t xReadyEventGroup;

/**
  * @brief 扫描按键，并通过状态机执行函数
  * @param key 按键
  */
void keyScan(FSMKey_Typedef *key)
{
  keyFlashBuffer(key, HAL_GPIO_ReadPin(key->GPIOx, key->GPIO_Pin));
  uint8_t press_flag = 0 , release_flag = 0;
  press_flag = (key->buffer & 0x0F) == key_event_press ? 1 : 0;
  release_flag = (key->buffer & 0x0F) == key_event_release ? 1 : 0;

  switch (key->state)
  {
  case NORMAL:
    if(press_flag) {
      key->state = PRE_ACTIVE;
      key->counter = 0;
    }
    break;

  case PRE_ACTIVE:
    if (key->counter < 4) {
      if (release_flag) { //4个采样周期内释放按键
        key->state = PRE_HIT;
        key->counter = 0;
        break;
      }
      key->counter++;
    } else { //未释放
      key->state = PRE_LONGPRESS;
      key->counter = 0;
    }
    break;

  case PRE_LONGPRESS:
    if (key->counter < 100) {
      if (release_flag) {     //100周期内释放了
        key->state = PRE_HIT; //算单击(或双击)
        key->counter = 0;
        break;
      }
      key->counter++;
    } else { //超过了1s
      key->state = LONGPRESS;
      key->counter = 0;
    }
    break;

  case PRE_HIT:
    if (key->counter < 30) { //300ms内
      if (press_flag) {      //又按下一次
        key->state = DOUBLE_HIT;
        key->counter = 0;
        break;
      }
      key->counter++;
    } else { //超过了300ms
      key->state = SINGLE_HIT;
      key->counter = 0;
    }
    break;

  case SINGLE_HIT: {
      //执行单击回调函数
      //    if (key->singleHit_callback != NULL) {
      //      key->singleHit_callback();
      //    }
    KeyEvent_TypeDef event = { 
      .row = key->row,
      .col = key->col,
      .state = SINGLE_HIT
    };
    if (xQueueSend(xKeyEventQueue, &event, 1) != pdTRUE)
    {
      // TODO: Error check
    }
    else
    {
      xEventGroupSetBits(xReadyEventGroup, BIT_KEY_READY);
    }
    key->state = NORMAL;
    break;
  }

  case DOUBLE_HIT: {
      //执行双击回调函数
      //    if (key->doubleHit_callback != NULL) {
      //      key->doubleHit_callback();
      //    } else if (key->singleHit_callback != NULL) { //若双击回调函数未注册，则执行两次单击回调函数
      //      key->singleHit_callback();
      //      key->singleHit_callback();
      //    }
    
    KeyEvent_TypeDef event = {
      .row = key->row,
      .col = key->col,
      .state = DOUBLE_HIT
    };
    if (xQueueSend(xKeyEventQueue, &event, 1) != pdTRUE)
    {
      // TODO: Error check
    }
    else
    {
      xEventGroupSetBits(xReadyEventGroup, BIT_KEY_READY);
    }
    key->state = NORMAL;
    break;
  }

  case LONGPRESS: {
      //长按状态下，每0.5s执行一次单击回调函数，循环执行
    if(release_flag || key->buffer == 0) {
      key->state = NORMAL;
      key->counter = 0;
      break;
    }
    if (!(key->counter % 20)) {
        //到达0.5s
      //执行单击回调函数
      KeyEvent_TypeDef event = { 
        .row = key->row,
        .col = key->col,
        .state = SINGLE_HIT
      };
      if (xQueueSend(xKeyEventQueue, &event, 1) != pdTRUE)
      {
        // TODO: Error check
      }
      else
      {
        xEventGroupSetBits(xReadyEventGroup, BIT_KEY_READY);
      }
    }
    key->counter++;
    key->counter %= 1000;
    break;
  }

  default:
    break;
  }
}

void keyScanAll(KeyGroup_Typedef *keyGroup)
{
  FSMKey_Typedef* current_key = keyGroup->HEAD;
  while(current_key != NULL) {
    keyScan(current_key);
    current_key = current_key->next;
  }
}
