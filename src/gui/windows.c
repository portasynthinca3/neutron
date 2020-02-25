//Neutron Project
//Window processing and rendering

#include "./windows.h"
#include "../images/win_close.xbm"
#include "../images/win_state.xbm"
#include "../images/win_minimize.xbm"

//The list of windows
window_t* windows = NULL;
//The window that is being dragged currently
window_t* window_dragging = NULL;
//The point of the dragging window that is pinned to the cursor
p2d_t window_dragging_cpos;
///The window that is in focus
window_t* window_focused = NULL;
//If this flag is set, focus can't be changed
uint8_t focus_monopoly = 0;

/*
 * Initializes window-related variables
 */
void gui_init_windows(void){
    focus_monopoly = 0;
    window_dragging = NULL;
    
    //Allocate some space for the windows
    gfx_verbose_println("Allocating memory for the window list");
    windows = (window_t*)calloc(256, sizeof(window_t));
    gfx_verbose_println("GUI init done");
}

/*
 * Returns the window in focus
 */
window_t* gui_get_focused_window(void){
    return window_focused;
}

/*
 * Sets the window in focus
 */
void gui_set_focused_window(window_t* win){
    window_focused = win;
}

/*
 * Returns the window list
 */
window_t* gui_get_window_list(void){
    return windows;
}

/*
 * Creates a window and adds it to the window list
 */
window_t* gui_create_window(char* title, void* icon_8, uint32_t flags, p2d_t pos, p2d_t size, void(*event_handler)(ui_event_args_t*)){
    window_t win;
    //Allocate memory for its title
    win.title = (char*)malloc(sizeof(char) * (strlen(title) + 1));
    //Copy the title over
    memcpy(win.title, title, strlen(title) + 1);
    //Ignore the first frame of this window
    win.ignore = 1;
    //Assign the properties
    win.icon_8 = icon_8;
    win.flags = flags;
    win.position = pos;
    win.size_real = size;
    win.event_handler = event_handler;
    //Allocate some space for window controls
    win.controls = (control_t*)calloc(128, sizeof(control_t));
    win.controls[0].type = 0;
    //Scan through the window list to determine its end
    uint32_t i = 0;
    while((&windows[i++])->title);
    i--;
    //Assign the window
    windows[i] = win;
    //Mark the end of the list
    windows[i + 1].title = NULL;
    //Mark it as focused
    window_focused = &windows[i];
    //Return the window
    return &windows[i];
}

/*
 * Removes the window from the window list and frees all memory used by it
 */
void gui_destroy_window(window_t* win){
    //Raise the event
    ui_event_args_t args;
    args.type = GUI_EVENT_WIN_CLOSE;
    args.win = win;
    args.control = NULL;
    args.extra_data = NULL;
    if(win->event_handler != NULL)
        win->event_handler(&args);
    //If the window we're destroying was in focus, reset it
    if(win == window_focused)
        window_focused = NULL;
    //Free up the memory used by its controls
    control_t* control;
    uint32_t i = 0;
    while((control = &win->controls[i++])->type){
        free(control->extended);
        free(control);
    }
    //Free up the memory used by the window title
    free(win->title);
    //Free up the memory used by the window itself
    free(win);
    //Scan through the window list to determine the window count
    window_t* scan_win;
    i = 0;
    uint32_t win_cnt = 0;
    uint32_t idx = 0;
    while((scan_win = &windows[i++])->title){
        //Increment the window count
        win_cnt++;
        //While we're at it, find the index of the window we're about to delete
        if(scan_win == win)
            idx = i - 1;
    }
    //Move the windows that stand after one that we just deleted, one to the left
    if(idx != i)
        for(uint32_t j = idx + 1; j <= i; j++)
            windows[j - 1] = windows[j];
    //Mark the last window as the last one
    windows[i - 1].title = NULL;
}

/*
 * Renders a window
 */
void gui_render_window(window_t* ptr){
    if(ptr->ignore){
        ptr->ignore = 0;
        return;
    }
    if(ptr->flags & GUI_WIN_FLAG_MINIMIZED)
        return; //Do not render the window if it's minimized

    //Raise the "render start" event
    if(ptr->event_handler != NULL){
        ui_event_args_t args = (ui_event_args_t){.win = ptr, .control = NULL, .type = GUI_EVENT_RENDER_START, .mouse_pos = gui_mouse_coords()};
        ptr->event_handler(&args);
    }

    //Only render the window if it has the visibility flag set
    if(ptr->flags & GUI_WIN_FLAG_VISIBLE){
        //Fill a rectangle with a window background color
        gfx_draw_filled_rect((p2d_t){.x = ptr->position.x + 1, .y = ptr->position.y + 11},
                             (p2d_t){.x = ptr->size.x - 1, .y = ptr->size.y - 11}, gui_get_color_scheme()->win_bg);
        //Draw the top section background
        gfx_draw_filled_rect((p2d_t){.x = ptr->position.x, .y = ptr->position.y}, 
                             (p2d_t){.x = ptr->size.x, .y = 11},
                             (ptr == window_focused) ? gui_get_color_scheme()->win_border : gui_get_color_scheme()->win_border_inactive);
        //Draw the border
        gfx_draw_rect((p2d_t){.x = ptr->position.x, .y = ptr->position.y},
                      (p2d_t){.x = ptr->size.x, .y = ptr->size.y}, gui_get_color_scheme()->win_border);
        //Print its title
        gfx_puts((p2d_t){.x = ptr->position.x + 2 + 8 + 2, .y = ptr->position.y + 2},
            (ptr == window_focused) ? gui_get_color_scheme()->win_title : gui_get_color_scheme()->win_title_inactive, COLOR32(0, 0, 0, 0), ptr->title);
        //If there is an icon, draw it
        if(ptr->icon_8 != NULL)
            gfx_draw_raw((p2d_t){.x = ptr->position.x + 2, .y = ptr->position.y + 1}, ptr->icon_8, (p2d_t){.x = 8, .y = 8});

        //Draw the close button 
        if(ptr->flags & GUI_WIN_FLAG_CLOSABLE)
            gfx_draw_xbm((p2d_t){.x = ptr->position.x + ptr->size.x - 10, .y = ptr->position.y + 2}, win_close_bits, (p2d_t){.x = win_close_width, .y = win_close_height},
                gui_get_color_scheme()->win_exit_btn, COLOR32(0, 0, 0, 0));
        else
            gfx_draw_xbm((p2d_t){.x = ptr->position.x + ptr->size.x - 10, .y = ptr->position.y + 2}, win_close_bits, (p2d_t){.x = win_close_width, .y = win_close_height},
                gui_get_color_scheme()->win_unavailable_btn, COLOR32(0, 0, 0, 0));

        //Draw the maximize (state change) button 
        if(ptr->flags & GUI_WIN_FLAG_MAXIMIZABLE)
            gfx_draw_xbm((p2d_t){.x = ptr->position.x + ptr->size.x - 19, .y = ptr->position.y + 2}, win_state_bits, (p2d_t){.x = win_state_width, .y = win_state_height},
                gui_get_color_scheme()->win_state_btn, COLOR32(0, 0, 0, 0));
        else
            gfx_draw_xbm((p2d_t){.x = ptr->position.x + ptr->size.x - 19, .y = ptr->position.y + 2}, win_state_bits, (p2d_t){.x = win_state_width, .y = win_state_height},
                gui_get_color_scheme()->win_unavailable_btn, COLOR32(0, 0, 0, 0));

        //Draw the minimize button 
        if(ptr->flags & GUI_WIN_FLAG_MINIMIZABLE)
            gfx_draw_xbm((p2d_t){.x = ptr->position.x + ptr->size.x - 28, .y = ptr->position.y + 2}, win_minimize_bits, (p2d_t){.x = win_minimize_width, .y = win_minimize_height},
                gui_get_color_scheme()->win_minimize_btn, COLOR32(0, 0, 0, 0));
        else
            gfx_draw_xbm((p2d_t){.x = ptr->position.x + ptr->size.x - 28, .y = ptr->position.y + 2}, win_minimize_bits, (p2d_t){.x = win_minimize_width, .y = win_minimize_height},
                gui_get_color_scheme()->win_unavailable_btn, COLOR32(0, 0, 0, 0));

        //Now draw its controls
        uint32_t i = 0;
        control_t* control;
        while((control = &ptr->controls[i++])->type)
            gui_render_control(ptr, control);
    }

    //Raise the "render end" event
    if(ptr->event_handler != NULL){
        ui_event_args_t args = (ui_event_args_t){.win = ptr, .control = NULL, .type = GUI_EVENT_RENDER_END, .mouse_pos = gui_mouse_coords()};
        ptr->event_handler(&args);
    }
}

/*
 * Processes window's interaction with the mouse and returns
 *   1 or 0 depending on the mouse pointer state
 *   (0 = don't process other windows)
 */
uint8_t gui_process_window(window_t* ptr){
    if(ptr->ignore)
        return 1;
    //Set the size based on the real size and flags
    if(ptr->flags & GUI_WIN_FLAG_MAXIMIZED)
        ptr->size = (p2d_t){.x = gfx_res_x() - 1, .y = gfx_res_y() - 16 - 1};
    else
        ptr->size = ptr->size_real;
    if(ptr->flags & GUI_WIN_FLAG_MINIMIZED)
        return 1; //Do not process the window if it's minimized
    //Only process the window if it has the visibility flag set
    if(ptr->flags & GUI_WIN_FLAG_VISIBLE){

        //Process the top-right buttons
        if(gfx_point_in_rect(gui_mouse_coords(),
                             (p2d_t){.x = ptr->position.x + ptr->size.x - 10, .y = ptr->position.y + 2},
                             (p2d_t){.x = 8, .y = 8})
           && (ptr->flags & GUI_WIN_FLAG_CLOSABLE) && gui_mouse_btns().x){
            //Destroy this window
            gui_destroy_window(ptr);
            //Don't process the window as it doesn't exist anymore |X_X|
            //And don't process others too (because of the press)
            return 0;
        } else if(gfx_point_in_rect(gui_mouse_coords(),
                                    (p2d_t){.x = ptr->position.x + ptr->size.x - 19, .y = ptr->position.y + 2},
                                    (p2d_t){.x = 8, .y = 8})
                  && (ptr->flags & GUI_WIN_FLAG_MAXIMIZABLE) && gui_mouse_btns().x){
            if(ptr->flags & GUI_WIN_FLAG_MAXIMIZED){
                //If the window is already maximized, restore the normal size
                ptr->flags &= ~GUI_WIN_FLAG_MAXIMIZED;
            } else {
                //Else, set window position to the top left corner
                ptr->position = (p2d_t){.x = 0, .y = 16};
                //Set the maximized flag
                ptr->flags |= GUI_WIN_FLAG_MAXIMIZED;
            }
            return 0;
        } else if(gfx_point_in_rect(gui_mouse_coords(),
                                    (p2d_t){.x = ptr->position.x + ptr->size.x - 28, .y = ptr->position.y + 2},
                                    (p2d_t){.x = 8, .y = 8})
                  && (ptr->flags & GUI_WIN_FLAG_MINIMIZABLE) && gui_mouse_btns().x){
            //Set the minimized flag
            ptr->flags |= GUI_WIN_FLAG_MINIMIZED;
            //If the window is in focus, reset the focus
            if(ptr == window_focused)
                window_focused = NULL;
            return 0;
        }

        //Process window dragging
        //If there's no such window that's being dragged right now, the cursor is in bounds of the title
        //  and the left button is being pressed, assume the window we're dragging is this one
        if(window_dragging == NULL &&
            gui_mouse_btns().x && gfx_point_in_rect(gui_mouse_coords(),
                                    (p2d_t){.x = ptr->position.x, .y = ptr->position.y},
                                    (p2d_t){.x = ptr->size.x, .y = 9})){
                window_dragging = ptr;
                window_dragging_cpos = (p2d_t){.x = ptr->position.x - gui_mouse_coords().x, .y = ptr->position.y - gui_mouse_coords().y};
        }
        //If the window we're dragging is this one, drag it
        if(window_dragging == ptr){
            if(gui_mouse_btns().x){
                //If the left mouse button is still being pressed, drag the window
                ptr->position = (p2d_t){.x = gui_mouse_coords().x + window_dragging_cpos.x, .y = gui_mouse_coords().y + window_dragging_cpos.y};
                //Constrain the window coordinates
                if(ptr->position.x < 0)
                    ptr->position.x = 0;
                if(ptr->position.y < 20) //The top bar is 20 pixels T H I C C
                    ptr->position.y = 20;
                if(ptr->position.x > gfx_res_x() - ptr->size.x - 1)
                    ptr->position.x = gfx_res_x() - ptr->size.x - 1;
                if(ptr->position.y > gfx_res_y() - ptr->size.y - 1)
                    ptr->position.y = gfx_res_y() - ptr->size.y - 1;
            } else {
                //Else, reset the pointer
                window_dragging = NULL;
            }
        }

        //Process window focusing
        //If the cursor is inside the current window, focus monopoly isn't enabled
        //  and the left button is held down, set the window in focus
        if(!focus_monopoly &&
           gui_mouse_btns().x &&
           gfx_point_in_rect(gui_mouse_coords(),
                             (p2d_t){.x = ptr->position.x + 1, .y = ptr->position.y + 1},
                             (p2d_t){.x = ptr->size.x - 2, .y = ptr->size.y - 2})){
            window_focused = ptr;
        }

        //Now process the controls
        uint8_t process_ptr = gfx_point_in_rect(gui_mouse_coords(), ptr->position, ptr->size);
        uint32_t i = 0;
        control_t* control;
        while((control = &ptr->controls[i++])->type)
            gui_process_control(ptr, control, process_ptr);
        return !(process_ptr && gui_mouse_btns().x);
    }
}