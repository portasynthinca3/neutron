#ifndef GUI_H
#define GUI_H

#include "../h/gfx.h"

//Structure defining the GUI color scheme
typedef struct {
    color8_t top_bar;
    color8_t desktop;
    color8_t cursor;
    color8_t selection;
} color_scheme_t;

//Keyboard buffer size in bytes
#define GUI_KEYBOARD_BUFFER_SIZE 128
//Mouse buffer size in bytes
#define GUI_MOUSE_BUFFER_SIZE 128

void gui_init(void);
void gui_update(void);

void gui_init_ps2(void);
void gui_poll_ps2(void);

void gui_draw_cursor(unsigned short x, unsigned short y);

#endif