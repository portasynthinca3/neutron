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
    color32_t win_title;
    color32_t win_title_inactive;
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
    //Pointer to extra data
    void* extra_data;
} ui_event_args_t;

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
#define GUI_EVENT_KEYBOARD                          5
#define GUI_EVENT_WIN_CLOSE                         6

//Image formats

#define GUI_IMAGE_FORMAT_XBM                        1
#define GUI_IMAGE_FORMAT_RAW                        2

//Control types

#define GUI_WIN_CTRL_LABEL                          1
#define GUI_WIN_CTRL_BUTTON                         2
#define GUI_WIN_CTRL_PROGRESS_BAR                   3
#define GUI_WIN_CTRL_IMAGE                          4
#define GUI_WIN_CTRL_TRACK_BAR                      5

void gui_init(void);
void gui_update(void);

void gui_render_windows(void);

void gui_get_mouse(void);
p2d_t gui_mouse_coords(void);
p2d_t gui_mouse_btns(void);

void gui_draw_cursor(uint32_t x, uint32_t y);

void gui_set_focus_monopoly(uint8_t val);
color_scheme_t* gui_get_color_scheme(void);

#endif