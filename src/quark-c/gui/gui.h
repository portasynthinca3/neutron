#ifndef GUI_H
#define GUI_H

#include "../drivers/gfx.h"

//Structure defining the GUI color scheme
typedef struct {
    color32_t top_bar;
    color32_t time;
    color32_t desktop;
    color32_t cursor;
    color32_t selection;
    color32_t win_bg;
    color32_t win_shade;
    color32_t win_title;
    color32_t win_border;
    color32_t win_exit_btn;
    color32_t win_state_btn;
    color32_t win_minimize_btn;
    color32_t win_unavailable_btn;
} color_scheme_t;

//Structure defining a form control
typedef struct {
    p2d_t position;
    p2d_t size;
    void* extended;
    unsigned short type;
} control_t;

//Structure defining a GUI window
typedef struct {
    uint32_t flags;
    control_t* controls;
    char* title;
    p2d_t position;
    p2d_t size;
    p2d_t size_real;
} window_t;

//Structure defining parameters passed to the UI event handler
typedef struct {
    //Event type
    uint32_t type;
    //Pointer to the window that contains the control that has issued the event
    window_t* win;
    //Pointer to the control that has issued the event
    control_t* control;
    //Mouse position on screen when the event occured
    p2d_t mouse_pos;
} ui_event_args_t;

//Structures defining extended controls

//Structure defining a label
typedef struct {
    char* text;
    color32_t text_color;
    color32_t bg_color;
} control_ext_label_t;

//Structure defining a button
typedef struct {
    char* text;
    void (*event_handler)(ui_event_args_t*);
    color32_t text_color;
    color32_t bg_color;
    color32_t pressed_bg_color;
    color32_t border_color;
    uint8_t pressed_last_frame;
} control_ext_button_t;

//Structure defining a progress bar
typedef struct {
    color32_t bg_color;
    color32_t fill_color;
    color32_t border_color;
    uint32_t max_val;
    uint32_t val;
} control_ext_progress_t;

//Structure defining an image
typedef struct {
    uint32_t image_format;
    void* image;
    color32_t color_hi;
    color32_t color_lo;
} control_ext_image_t;

//Structure defining a track bar
typedef struct {
    color32_t bg_color;
    color32_t fill_color;
    color32_t border_color;
    uint32_t max_val;
    uint32_t val;
    void(*callback)(ui_event_args_t*);
} control_ext_track_bar_t;

//UI event types

#define GUI_EVENT_UNDEFINED                         0
#define GUI_EVENT_CLICK                             1
#define GUI_EVENT_TRACK_BAR_CHANGE                  2

//Image formats

#define GUI_IMAGE_FORMAT_XBM                        1
#define GUI_IMAGE_FORMAT_RAW                        2

//Keyboard buffer size in bytes
#define GUI_KEYBOARD_BUFFER_SIZE 128
//Mouse buffer size in bytes
#define GUI_MOUSE_BUFFER_SIZE 128

//Window flags

#define GUI_WIN_FLAG_CLOSABLE                       (1 << 1)
#define GUI_WIN_FLAG_MAXIMIZABLE                    (1 << 2)
#define GUI_WIN_FLAG_MINIMIZABLE                    (1 << 3)
#define GUI_WIN_FLAG_VISIBLE                        (1 << 4)
#define GUI_WIN_FLAG_DRAGGABLE                      (1 << 6)
#define GUI_WIN_FLAG_CLOSED                         (1 << 7)
#define GUI_WIN_FLAG_MAXIMIZED                      (1 << 8)
#define GUI_WIN_FLAG_MINIMIZED                      (1 << 9)
#define GUI_WIN_FLAGS_STANDARD (GUI_WIN_FLAG_CLOSABLE | GUI_WIN_FLAG_MAXIMIZABLE | GUI_WIN_FLAG_MINIMIZABLE | GUI_WIN_FLAG_VISIBLE | GUI_WIN_FLAG_DRAGGABLE)

//Control types

#define GUI_WIN_CTRL_LABEL                          1
#define GUI_WIN_CTRL_BUTTON                         2
#define GUI_WIN_CTRL_PROGRESS_BAR                   3
#define GUI_WIN_CTRL_IMAGE                          4
#define GUI_WIN_CTRL_TRACK_BAR                      5

void gui_init(void);
void gui_update(void);

window_t* gui_create_window(char* title, uint32_t flags, p2d_t pos, p2d_t size);
control_t* gui_create_control(window_t* win, uint32_t type, void* ext_ptr, p2d_t pos, p2d_t size);
control_t* gui_create_label(window_t* win, p2d_t pos, p2d_t size, char* text, color32_t text_color, color32_t bg_color);
control_t* gui_create_button(window_t* win, p2d_t pos, p2d_t size, char* text, void (*event_handler)(ui_event_args_t*),
                             color32_t text_color, color32_t bg_color, color32_t pressed_bg_color, color32_t border_color);
control_t* gui_create_progress_bar(window_t* win, p2d_t pos, p2d_t size, color32_t bg_color, color32_t fill_color,
                                   color32_t border_color, uint32_t max_val, uint32_t val);
control_t* gui_create_image(window_t* win, p2d_t pos, p2d_t size, uint32_t format, void* data, color32_t color_lo, color32_t color_hi);
control_t* gui_create_track_bar(window_t* win, p2d_t pos, p2d_t size, color32_t bg_color, color32_t fill_color,
                                color32_t border_color, uint32_t max_val, uint32_t val, void(*callback)(ui_event_args_t*));

void gui_render_windows(void);
void gui_process_window(window_t* ptr);
void gui_render_window(window_t* ptr);
void gui_render_control(window_t* win_ptr, control_t* ptr);
void gui_process_control(window_t* win_ptr, control_t* ptr, uint8_t handle_pointer);

void gui_init_ps2(void);
void gui_poll_ps2(void);
void gui_reset_ps2_kbd(void);

void gui_draw_cursor(uint32_t x, uint32_t y);

void gui_set_focus_monopoly(uint8_t val);
color_scheme_t* gui_get_color_scheme(void);

#endif