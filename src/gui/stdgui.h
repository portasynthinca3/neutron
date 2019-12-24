#ifndef STDGUI_H
#define STDHUI_H

#include "../stdlib.h"
#include "../apps/app.h"

void stdgui_create_shutdown_prompt(void);
void stdgui_create_system_win(void);
uint8_t stdgui_create_color_picker(void (*callback)(ui_event_args_t*), color32_t start);
color32_t stdgui_cpick_get_color(void);

void stdgui_register_app(app_t app);
void stdgui_create_program_launcher(void);

#endif