#ifndef GUI_H
#define GUI_H

#include "../drivers/gfx.h"

//Print the time it took to render things or not?
//#define GUI_PRINT_RENDER_TIME

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
    color32_t win_border_inactive;
    color32_t win_exit_btn;
    color32_t win_state_btn;
    color32_t win_minimize_btn;
    color32_t win_unavailable_btn;
} color_scheme_t;

//Structure defining parameters passed to the UI event handler
typedef struct {
    //Event type
    uint32_t type;
    //Pointer to the window that contains the control that has issued the event
    void* win;
    //Pointer to the control that has issued the event
    void* control;
    //Mouse position on screen when the event occured
    p2d_t mouse_pos;
} ui_event_args_t;

//Structure defining a form control
typedef struct {
    //Position of the control
    p2d_t position;
    //Size of the control
    p2d_t size;
    //Pointer to the extended control
    void* extended;
    //Type of the control
    uint16_t type;
    //The event handler for the control
    void(*event_handler)(ui_event_args_t*);
} control_t;

//Structure defining a GUI window
typedef struct {
    //Flags of the window
    uint32_t flags;
    //List of window's controls
    control_t* controls;
    //Title of the window
    char* title;
    //Pointer to the 8x8 raw icon of the window
    void* icon_8;
    //Position of the window
    p2d_t position;
    //Size of the window that it's rendered with
    p2d_t size;
    //The real (defined) size of the window
    p2d_t size_real;
    //The event handler for the window
    void(*event_handler)(ui_event_args_t*);
    //Ignore window rendering and processing?
    //(is being set on creation)
    uint8_t ignore;
} window_t;

//Structures defining extended controls

//Structure defining a label
typedef struct {
    //Text of the label
    char* text;
    //Color of the text of the label
    color32_t text_color;
    //Color of the background of the label
    color32_t bg_color;
} control_ext_label_t;

//Structure defining a button
typedef struct {
    //Text of the button
    char* text;
    //Color of the text of the button
    color32_t text_color;
    //Color of the background of the button
    color32_t bg_color;
    //Color of the background of the button while it's being pressed
    color32_t pressed_bg_color;
    //Color of the border of the button
    color32_t border_color;
    //Whether the button was being pressed last frame
    uint8_t pressed_last_frame;
} control_ext_button_t;

//Structure defining a progress bar
typedef struct {
    //Color of the background of the progress bar
    color32_t bg_color;
    //Color of the fill of the progress bar
    color32_t fill_color;
    //Color of the border of the progress bar
    color32_t border_color;
    //Maximum value of the progress bar
    uint32_t max_val;
    //Value of the progress bar
    uint32_t val;
} control_ext_progress_t;

//Structure defining an image
typedef struct {
    //Format of the image
    uint32_t image_format;
    //Pointer to the image data
    void* image;
    //High color for the XBM format
    color32_t color_hi;
    //Low color for the XBM format
    color32_t color_lo;
} control_ext_image_t;

//Structure defining a track bar
typedef struct {
    //Background color of the trackbar
    color32_t bg_color;
    //Fill color of the trackbar
    color32_t fill_color;
    //Border color of the trackbar
    color32_t border_color;
    //Maximum value of the trackbar
    uint32_t max_val;
    //Value of the trackbar
    uint32_t val;
} control_ext_track_bar_t;

//UI event types

#define GUI_EVENT_UNDEFINED                         0
#define GUI_EVENT_RENDER_START                      1
#define GUI_EVENT_RENDER_END                        2
#define GUI_EVENT_CLICK                             3
#define GUI_EVENT_TRACK_BAR_CHANGE                  4

//Image formats

#define GUI_IMAGE_FORMAT_XBM                        1
#define GUI_IMAGE_FORMAT_RAW                        2

//Window flags

#define GUI_WIN_FLAG_CLOSABLE                       (1 << 1)
#define GUI_WIN_FLAG_MAXIMIZABLE                    (1 << 2)
#define GUI_WIN_FLAG_MINIMIZABLE                    (1 << 3)
#define GUI_WIN_FLAG_VISIBLE                        (1 << 4)
#define GUI_WIN_FLAG_DRAGGABLE                      (1 << 6)
#define GUI_WIN_FLAG_CLOSED                         (1 << 7)
#define GUI_WIN_FLAG_MAXIMIZED                      (1 << 8)
#define GUI_WIN_FLAG_MINIMIZED                      (1 << 9)
#define GUI_WIN_FLAGS_STANDARD (GUI_WIN_FLAG_CLOSABLE | GUI_WIN_FLAG_MINIMIZABLE | GUI_WIN_FLAG_VISIBLE | GUI_WIN_FLAG_DRAGGABLE)

//Control types

#define GUI_WIN_CTRL_LABEL                          1
#define GUI_WIN_CTRL_BUTTON                         2
#define GUI_WIN_CTRL_PROGRESS_BAR                   3
#define GUI_WIN_CTRL_IMAGE                          4
#define GUI_WIN_CTRL_TRACK_BAR                      5

void gui_init(void);
void gui_update(void);

window_t* gui_create_window(char* title, void* icon_8, uint32_t flags, p2d_t pos, p2d_t size,
                            void(*event_handler)(ui_event_args_t*));
void gui_destroy_window(window_t* win);
control_t* gui_create_control(window_t* win, uint32_t type, void* ext_ptr, p2d_t pos, p2d_t size,
                              void(*event_handler)(ui_event_args_t*));
control_t* gui_create_label(window_t* win, p2d_t pos, p2d_t size, char* text, color32_t text_color, color32_t bg_color,
                            void(*event_handler)(ui_event_args_t*));
control_t* gui_create_button(window_t* win, p2d_t pos, p2d_t size, char* text, color32_t text_color, color32_t bg_color,
                             color32_t pressed_bg_color, color32_t border_color, void(*event_handler)(ui_event_args_t*));
control_t* gui_create_progress_bar(window_t* win, p2d_t pos, p2d_t size, color32_t bg_color, color32_t fill_color,
                                   color32_t border_color, uint32_t max_val, uint32_t val, void(*event_handler)(ui_event_args_t*));
control_t* gui_create_image(window_t* win, p2d_t pos, p2d_t size, uint32_t format, void* data, color32_t color_lo, color32_t color_hi,
                            void(*event_handler)(ui_event_args_t*));
control_t* gui_create_track_bar(window_t* win, p2d_t pos, p2d_t size, color32_t bg_color, color32_t fill_color,
                                color32_t border_color, uint32_t max_val, uint32_t val, void(*event_handler)(ui_event_args_t*));

void gui_render_windows(void);
void gui_process_window(window_t* ptr);
void gui_render_window(window_t* ptr);
void gui_render_control(window_t* win_ptr, control_t* ptr);
void gui_process_control(window_t* win_ptr, control_t* ptr, uint8_t handle_pointer);

void gui_init_ps2(void);
void gui_poll_ps2(void);
void gui_reset_ps2(void);

void gui_draw_cursor(uint32_t x, uint32_t y);

void gui_set_focus_monopoly(uint8_t val);
color_scheme_t* gui_get_color_scheme(void);

#endif