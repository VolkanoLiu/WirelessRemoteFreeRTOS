#ifndef __STATE_MACHINE_H
#define __STATE_MACHINE_H

#include <stdint.h>

typedef enum
{
  Comm_HEAD = 0,
  Comm_ADDR,
  Comm_LEN,
  Comm_MSG
} CommStateMachine_TypeDef;

#define BIT_AS69_READY 1U << (0U)
#define BIT_SSD1306_READY 1U << (1U)
#define BIT_KEY_READY 1U << (2U)
#define BIT_RECV_READY 1U << (3U)

#define BIT_OLED_UPDATE 1U << (0U)

#endif // __STATE_MACHINE_H