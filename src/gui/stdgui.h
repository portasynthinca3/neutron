#ifndef STDGUI_H
#define STDHUI_H

#include "../stdlib.h"
#include "./windows.h"
#include "./controls.h"

void stdgui_create_shutdown_prompt(void);
void stdgui_create_system_win(void);
void stdgui_create_task_manager(void);
uint8_t stdgui_create_color_picker(void (*callback)(ui_event_args_t*), color32_t start);
color32_t stdgui_cpick_get_color(void);

#endif