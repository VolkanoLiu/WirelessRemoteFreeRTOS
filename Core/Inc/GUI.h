#ifndef __GUI_H
#define __GUI_H

#include "main.h"
#include "keypress_process.h"

#define guiINT          1U << (0U)
#define guiUINT         1U << (1U)
#define guiFLOAT        1U << (2U)
#define guiBOOL         1U << (3U)
#define guiHEX          1U << (4U)
#define guiSCREEN_SYNC  1U << (5U)
#define guiINC_CTRL     1U << (6U)
#define guiENABLED      1U << (31U)

#define guiTYPE_MASK    0x1F

typedef uint32_t GUI_Properties_TypeDef;

typedef enum GUI_ComponentType
{
  GUI_MENU = 0,
  GUI_VALUEGET,
  GUI_VALUESET
} GUI_ComponentType_TypeDef;

typedef struct GUI
{
  const char* name;
  void* selected_or_unit;
  GUI_ComponentType_TypeDef type;
  struct GUI* child;
  struct GUI* next;
  struct GUI* prev;
  void* childCount_or_value;
  GUI_Properties_TypeDef properties;
  struct GUI* parent;
  uint32_t index;
} GUI_TypeDef;

typedef struct GUI_Value
{
  uint32_t value;
  GUI_TypeDef* associated_gui;
} GUI_Value_TypeDef;

void GUI_SetProperties(GUI_TypeDef* pGUI, GUI_Properties_TypeDef properties);
void GUI_ClearProperties(GUI_TypeDef* pGUI, GUI_Properties_TypeDef properties);
void GUI_Init(GUI_TypeDef* pGUI, const char* name, void* selected_or_unit, GUI_ComponentType_TypeDef type, void* childCount_or_value);
void GUI_AddChild(GUI_TypeDef* pParent, GUI_TypeDef* pChild);
void GUI_Append(GUI_TypeDef* pHead, GUI_TypeDef* pNew);
void GUI_Render(GUI_TypeDef* pCurr);
GUI_TypeDef* GUI_KeyProcess(GUI_TypeDef* current_menu, KeyEvent_TypeDef* event);


#endif // !__GUI_H
