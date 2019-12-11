#ifndef GUI_H
#define GUI_H

#include "./gfx.h"

//Structure defining the GUI color scheme
typedef struct {
    color8_t top_bar;
    color8_t time;
    color8_t desktop;
    color8_t cursor;
    color8_t selection;
    color8_t win_bg;
    color8_t win_border;
    color8_t win_title;
    color8_t win_exit_btn;
    color8_t win_state_btn;
    color8_t win_minimize_btn;
    color8_t win_unavailable_btn;
} color_scheme_t;

//Alignement
enum gui_alignment_t {ALIGN_V_TOP = 1, ALIGN_V_CENTER = 2, ALIGN_V_BOTTOM = 4,
                      ALIGN_H_LEFT = 8, ALIGN_H_CENTER = 16, ALIGN_H_RIGHT = 32};

//Structure defining a form control
typedef struct {
    p2d_t position;
    p2d_t size;
    uint32_t extended_size;
    void* extended;
    unsigned short type;
} control_t;

//Structure defining a GUI window
typedef struct {
    unsigned int flags;
    control_t* controls;
    char* title;
    p2d_t position;
    p2d_t size;
    p2d_t size_real;
} window_t;

//Structures defining extended controls

//Structure defining a label
typedef struct {
    char* text;
    uint32_t alignment;
    color8_t text_color;
    color8_t bg_color;
} control_ext_label_t;

//Keyboard buffer size in bytes
#define GUI_KEYBOARD_BUFFER_SIZE 128
//Mouse buffer size in bytes
#define GUI_MOUSE_BUFFER_SIZE 128

//Window flags

#define GUI_WIN_FLAG_CLOSABLE                       (1 << 1)
#define GUI_WIN_FLAG_MAXIMIZABLE                    (1 << 2)
#define GUI_WIN_FLAG_MINIMIZABLE                    (1 << 3)
#define GUI_WIN_FLAG_VISIBLE                        (1 << 4)
#define GUI_WIN_FLAG_TITLE_VISIBLE                  (1 << 5)
#define GUI_WIN_FLAG_DRAGGABLE                      (1 << 6)
#define GUI_WIN_FLAG_CLOSED                         (1 << 7)
#define GUI_WIN_FLAG_MAXIMIZED                      (1 << 8)
#define GUI_WIN_FLAG_MINIMIZED                      (1 << 9)

//Control types

#define GUI_WIN_CTRL_LABEL                          1
#define GUI_WIN_CTRL_BUTTON                         2

void gui_init(void);
void gui_update(void);

void gui_render_windows(void);
void gui_process_window(window_t* ptr);
void gui_render_window(window_t* ptr);
void gui_render_control(window_t* win_ptr, control_t* ptr);

void gui_init_ps2(void);
void gui_poll_ps2(void);

void gui_draw_cursor(unsigned short x, unsigned short y);

#endif