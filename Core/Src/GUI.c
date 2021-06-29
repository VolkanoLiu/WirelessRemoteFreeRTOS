#include "GUI.h"
#include "ssd1306.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "keypress_process.h"

void GUI_SetProperties(GUI_TypeDef* pGUI, GUI_Properties_TypeDef properties)
{
  if (pGUI != NULL)
  {
    pGUI->properties |= properties;
  }
}

void GUI_ClearProperties(GUI_TypeDef* pGUI, GUI_Properties_TypeDef properties)
{
  if (pGUI != NULL)
  {
    pGUI->properties &= ~properties;
  }
}

void GUI_Init(GUI_TypeDef* pGUI, const char* name, void* selected_or_unit, GUI_ComponentType_TypeDef type, void* childCount_or_value)
{
  if (pGUI != NULL)
  {  
    pGUI->name = name;
    pGUI->selected_or_unit = selected_or_unit;
    pGUI->type = type;
    pGUI->childCount_or_value = childCount_or_value;
    pGUI->child = NULL;
    pGUI->next = NULL;
    pGUI->prev = NULL;
    pGUI->properties = 0;
    pGUI->parent = NULL;
    pGUI->index = 0;
    if (type == GUI_VALUEGET || type == GUI_VALUESET)
    {
      ((GUI_Value_TypeDef*)childCount_or_value)->associated_gui = pGUI;
    }
  }
}

void GUI_AddChild(GUI_TypeDef* pParent, GUI_TypeDef* pChild)
{
  if (pParent != NULL && pChild != NULL)
  {
    if (pParent->child == NULL)
    {
      pParent->child = pChild;
      pParent->selected_or_unit = pChild;
      pChild->index = 0;
    }
    else
    {
      GUI_TypeDef* pTemp = pParent->child;
      while (pTemp->next != NULL) pTemp = pTemp->next;
      pTemp->next = pChild;
      pChild->prev = pTemp;
      pChild->index = pTemp->index + 1;
    }
    pChild->parent = pParent;
    pParent->childCount_or_value = (void*)((uint32_t)(pParent->childCount_or_value) + 1);
  }
}
void GUI_Append(GUI_TypeDef* pHead, GUI_TypeDef* pNew)
{
  if (pHead != NULL&&pNew != NULL)
  {
    while (pHead->next != NULL) pHead = pHead->next;
    pHead->next = pNew;
    pNew->prev = pHead;
    pNew->index = pHead->index + 1;
    pNew->parent = pHead->parent;
    pNew->parent->childCount_or_value = (void*)((uint32_t)(pNew->parent->childCount_or_value) + 1);
  }
}

uint8_t input_buffer[19];
int input_offset = 0;
uint8_t input_dot = 0;

void GUI_WriteValue(GUI_TypeDef* pFirst, char* str_value, int* pstr_len)
{
  GUI_Value_TypeDef* value = (GUI_Value_TypeDef*)pFirst->childCount_or_value;
	switch (pFirst->properties & guiTYPE_MASK)
	{
	case guiINT:
		*pstr_len = sprintf(str_value, "%d", *(int32_t*)&(value->value));
		break;
	case guiUINT:
		*pstr_len = sprintf(str_value, "%u", *(uint32_t*)&(value->value));
		break;
	case guiFLOAT:
		*pstr_len = sprintf(str_value, "%.3f", *(float*)&(value->value));
  	if (*pstr_len > 10)
  	{
    	*pstr_len = sprintf(str_value, "%.4e", *(float*)&(value->value));
  	}
		break;
	case guiBOOL:
		*pstr_len = 5;
		if (value->value == 0)
		{
			strcpy(str_value, "False");
		}
		else
		{
			strcpy(str_value, " True");
		}
		break;
	case guiHEX:
		*pstr_len = sprintf(str_value, "%x", value->value);
		break;
	default:
		break;
	}
}

void GUI_ReadValue(GUI_TypeDef* pFirst, char* str_value)
{
  GUI_Value_TypeDef* value = (GUI_Value_TypeDef*)pFirst->childCount_or_value;
  switch (pFirst->properties & guiTYPE_MASK)
  {
  case guiINT:
    sscanf((const char*)str_value, "%d", &(value->value));
    break;
  case guiUINT:
    sscanf((const char*)str_value, "%lu", &(value->value));
    break;
  case guiFLOAT:
    sscanf((const char*)str_value, "%f", &(value->value));
    break;
  default:
    break;
  }
}


void GUI_Input(uint8_t ch)
{
  if (input_offset < 18)
  {
    input_buffer[input_offset] = ch;
    input_offset++;
    input_buffer[input_offset] = '\0';
  }
}
void GUI_InputNum(uint8_t num)
{
  if (num >= 0 && num < 10)
  {
    GUI_Input('0' + num);
  }
}
void GUI_InputDot()
{
  if (input_dot == 0 && input_offset < 18)
  {
    input_dot = 1;
    GUI_Input('.');
  }
}
void GUI_InputMinus()
{
  if (input_offset == 0)
  {
    GUI_Input('-');
  }
}
void GUI_InputDel()
{
  if (input_offset > 0)
  {
    input_offset--;
    if (input_buffer[input_offset] == '.')
    {
      input_dot = 0;
    }
    input_buffer[input_offset] = '\0';
  }
}

void GUI_Render(GUI_TypeDef* pCurr)
{
  // TODO
  if (pCurr != NULL)
  {
    ssd1306_Fill(Black);
    if (pCurr->type == GUI_MENU)
    {
      uint8_t row = 0;
      GUI_TypeDef* pChild = pCurr->selected_or_unit;
      GUI_TypeDef* volatile pFirst = pChild;
      uint32_t childCount = (uint32_t)pCurr->childCount_or_value;
      if (pChild != NULL) /* Choose the first item on the screen */
      {
        if (pChild->index < 3)
        {
          for (int i = 0; i < pChild->index; i++)
          {
            pFirst = pFirst->prev;
          }
        }
        else if (childCount - pChild->index <= 2)
        {
          for (int i = 0; i < (childCount < 5 ? childCount : 5) - (childCount - pChild->index); i++)
          {
            pFirst = pFirst->prev;
          }
        }
        else
        {
          pFirst = pFirst->prev->prev;
        }
      }
      for (int i = 0; i < 5; i++) /* Render */
      {
        SSD1306_COLOR color = White;
        ssd1306_SetCursor(0, 11 * i);
        uint8_t str_value[32];
        int str_len;
        // GUI_Value_TypeDef* value = (GUI_Value_TypeDef*)pFirst->childCount_or_value;
        
        if (pFirst == pChild) /* Focused row */
        {
          ssd1306_fillRect(0, 11 * i, 128, 10, White);
          color = Black;
        }
        ssd1306_WriteString(pFirst->name, &Font_7x10, color);
        
        /* Render row by row */
        ssd1306_SetCursor(48, 11 * i);
        if (pFirst->type == GUI_MENU)
        {
          ssd1306_WriteChar('>', &Font_7x10, color);
        }
        else if (pFirst->type == GUI_VALUEGET)
        {
          ssd1306_WriteChar(':', &Font_7x10, color);
          if (pFirst->childCount_or_value != NULL)
          {
						GUI_WriteValue(pFirst, str_value, &str_len);
            ssd1306_SetCursor(7 * (18 - str_len), 11 * i);
            ssd1306_WriteString(str_value, &Font_7x10, color);
          }
        }
        else if (pFirst->type == GUI_VALUESET)
        {
          ssd1306_WriteChar(']', &Font_7x10, color);
          if (pFirst->childCount_or_value != NULL)
          {
						GUI_WriteValue(pFirst, str_value, &str_len);
            ssd1306_SetCursor(7 * (18 - str_len), 11 * i);
            ssd1306_WriteString(str_value, &Font_7x10, color);
          }
        }
        pFirst = pFirst->next;
        if (pFirst == NULL) break;
      }
    }
    else if (pCurr->type == GUI_VALUEGET)
    {
      int str_len;
      uint8_t str_value[32];
      // GUI_Value_TypeDef* value = (GUI_Value_TypeDef*)pCurr->childCount_or_value;
      ssd1306_SetCursor(0, 0);
      ssd1306_fillRect(0, 0, 128, 10, White);
      ssd1306_WriteString("Name :", &Font_7x10, Black);
      ssd1306_SetCursor(42, 0);
      ssd1306_WriteString(pCurr->name, &Font_7x10, Black);
      ssd1306_SetCursor(0, 11);
      ssd1306_WriteString("Unit :", &Font_7x10, White);
      ssd1306_SetCursor(42, 11);
      ssd1306_WriteString(pCurr->selected_or_unit, &Font_7x10, White);
      ssd1306_SetCursor(0, 22);
      ssd1306_WriteString("Value:", &Font_7x10, White);
      ssd1306_SetCursor(42, 22);
      GUI_WriteValue(pCurr, str_value, &str_len);
      ssd1306_WriteString(str_value, &Font_7x10, White);
    }
    else if (pCurr->type == GUI_VALUESET)
    {
      int str_len;
      uint8_t str_value[32];
      // GUI_Value_TypeDef* value = (GUI_Value_TypeDef*)pCurr->childCount_or_value;
      ssd1306_SetCursor(0, 0);
      ssd1306_fillRect(0, 0, 128, 10, White);
      ssd1306_WriteString("Name :", &Font_7x10, Black);
      ssd1306_SetCursor(42, 0);
      ssd1306_WriteString(pCurr->name, &Font_7x10, Black);
      ssd1306_SetCursor(0, 11);
      ssd1306_WriteString("Unit :", &Font_7x10, White);
      ssd1306_SetCursor(42, 11);
      ssd1306_WriteString(pCurr->selected_or_unit, &Font_7x10, White);
      ssd1306_SetCursor(0, 22);
      ssd1306_WriteString("Value:", &Font_7x10, White);
      ssd1306_SetCursor(42, 22);
      GUI_WriteValue(pCurr, str_value, &str_len);
      ssd1306_WriteString(str_value, &Font_7x10, White);
      ssd1306_SetCursor(0, 33);
      ssd1306_WriteString("New  :", &Font_7x10, White);
      ssd1306_SetCursor(0, 44);
      ssd1306_WriteString(input_buffer, &Font_7x10, White);
      ssd1306_WriteString("_", &Font_7x10, White);
    }
  }
}
/*
 * MENU:
 *  NOP | NOP | NOP |  UP
 *  DEC | TOG | INC |  DW
 *  NOP | NOP | NOP |  OK
 *  NOP | NOP | NOP | ESC
 *  
 * VALUEGET:
 *  NOP | NOP | NOP | NOP
 *  NOP | NOP | NOP | NOP
 *  NOP | NOP | NOP | NOP
 *  NOP | NOP | NOP | ESC
 *  
 * VALUESET:
 *   1  |  2  |  3  |  -
 *   4  |  5  |  6  | SET
 *   7  |  8  |  9  |  OK
 *  DEL |  0  | DOT | ESC
 **/
GUI_TypeDef* GUI_KeyProcess(GUI_TypeDef* current_menu, KeyEvent_TypeDef* event)
{
  GUI_TypeDef* selected = (GUI_TypeDef*)current_menu->selected_or_unit;
  if (selected == NULL) return current_menu;
  
  switch(current_menu->type)
  {
  case GUI_MENU:
    if (event->col == 3)
    {
      switch (event->row)
      {
      case 0:
        if (selected->index != 0)
        {
          current_menu->selected_or_unit = selected->prev;
          return current_menu;
        }
        break;
        
      case 1:
        if ((selected->index < ((uint32_t)current_menu->childCount_or_value) - 1))
        {
          current_menu->selected_or_unit = selected->next;
          return current_menu;
        }
        break;
        
      case 2:
        if (selected->childCount_or_value != NULL)
        {
          if (selected->type == GUI_VALUESET)
          {
            input_offset = 0;
            input_buffer[0] = 0;
            input_dot = 0;
          }
          return selected;
        }
        break;
        
      case 3:
        if (current_menu->parent != NULL)
        {
          return current_menu->parent;
        }
        break;
        
      default:
        break;
      }
    }
    else if (event->row == 1)
    {
      if (selected->type == GUI_VALUESET && (selected->properties & guiTYPE_MASK) & (guiINT | guiUINT) && selected->properties & guiINC_CTRL)
      {
        // TODO: limit settings
        if (event->col == 0)((GUI_Value_TypeDef *)selected->childCount_or_value)->value -= 1;
        else if (event->col == 2)((GUI_Value_TypeDef *)selected->childCount_or_value)->value += 1;
      }
      else if (selected->type == GUI_VALUESET && (selected->properties & guiTYPE_MASK) & (guiBOOL) && selected->properties & guiINC_CTRL)
      {
        if (event->col == 1) ((GUI_Value_TypeDef *)selected->childCount_or_value)->value = ((GUI_Value_TypeDef *)selected->childCount_or_value)->value == 0 ? 1 : 0 ;
      }
    }
    break;
    
  case GUI_VALUEGET:
    if (event->col == 3)
    {
      switch (event->row)
      {       
      case 3:
        if (current_menu->parent != NULL)
        {
          return current_menu->parent;
        }
        break;
        
      default:
        break;
      }
    }
    break;
    
  case GUI_VALUESET:
    if (event->col == 3)
    {
      switch (event->row)
      {
      case 0:
        GUI_InputMinus();
        break;
      case 1:
        GUI_ReadValue(current_menu, input_buffer);
        return current_menu;
        break;
        
      case 2:
        GUI_ReadValue(current_menu, input_buffer);
        if (current_menu->parent != NULL)
        {
          return current_menu->parent;
        }
        break;
        
      case 3:
        if (current_menu->parent != NULL)
        {
          return current_menu->parent;
        }
        break;
        
      default:
        break;
      }
    }
    else
    {
      uint8_t input_key_num = event->col + 3 * event->row;
      if (input_key_num < 9)
      {
        GUI_InputNum(input_key_num + 1);
      }
      else if (input_key_num == 10)
      {
        GUI_InputNum(0);
      }
      else if (input_key_num == 9)
      {
        GUI_InputDel();
      }
      else if (input_key_num == 11)
      {
        GUI_InputDot();
      }
    }
    break;
    
  default:
    break;
  }
  
  return current_menu;
}