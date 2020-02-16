#ifndef WINDOWS_H
#define WINDOWS_H

#include "../stdlib.h"
#include "./gui.h"

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
    //The process UID that controls this window
    //(if exists)
    uint64_t task_uid;
} window_t;

#include "./controls.h"

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

void gui_init_windows(void);

window_t* gui_get_focused_window(void);
void gui_set_focused_window(window_t* win);
window_t* gui_get_window_list(void);

window_t* gui_create_window(char* title, void* icon_8, uint32_t flags, p2d_t pos, p2d_t size,
                            void(*event_handler)(ui_event_args_t*));
void gui_destroy_window(window_t* win);

uint8_t gui_process_window(window_t* ptr);
void gui_render_window(window_t* ptr);

#endif