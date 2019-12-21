//Neutron Project
//Graphical User interface
//Built on top of the GFX library (src/quark-c/c/gfx.c)

#include "./gui.h"
#include "../stdlib.h"
#include "../drivers/gfx.h"
#include "../drivers/diskio.h"
#include "../drivers/pit.h"
#include "../drivers/ps2.h"

#include "../images/power.xbm"
#include "../images/system.xbm"

//Mouse position on the screen
int32_t mx, my;
//Mouse buttons state
uint8_t ml, mr;
//Current color scheme
color_scheme_t color_scheme;
//The list of windows
window_t* windows = NULL;
//The window that is being dragged currently
window_t* window_dragging = NULL;
//The point of the dragging window that is pinned to the cursor
p2d_t window_dragging_cpos;
///The window that is in focus
window_t* window_focused = NULL;
//Flag indicating that focusing was already processed this frame
uint8_t focus_processed;
//Window position in the the top bar
uint16_t topb_win_pos;
//The current time as a string
char time[64] = "??:??:??\0";
//If this flag is set, focus can't be changed and the background below
//  the window in focus is darkended
uint8_t focus_monopoly = 0;

uint8_t last_frame_ml = 0;
void quark_gui_callback_power_pressed(void);
void quark_gui_callback_system_pressed(void);

control_ext_progress_t* example_progress_bar;

void gui_example_button_callback(ui_event_args_t* args){
    if(example_progress_bar->val == 0)
        example_progress_bar->val = 50;
    else if(example_progress_bar->val == 50)
        example_progress_bar->val = 100;
    else if(example_progress_bar->val == 100)
        example_progress_bar->val = 0;
}

/*
 * Performs some GUI initialization
 */
void gui_init(void){
    //Disable focus monopoly
    focus_monopoly = 0;
    //Reset mouse state
    gfx_verbose_println("Resetting mouse state");
    ml = mr = 0;
    //Move the cursor to the center of the screen
    mx = gfx_res_x() / 2;
    my = gfx_res_y() / 2;
    //Reset some variables
    window_dragging = NULL;
    gfx_verbose_println("Initializing color scheme");
    //Set the default color scheme
    color_scheme.desktop =                  COLOR32(255, 40, 40, 40);           //Very dark grey
    color_scheme.top_bar =                  COLOR32(255, 70, 70, 70);           //Dark grey
    color_scheme.cursor =                   COLOR32(255, 255, 255, 255);        //White
    color_scheme.selection =                COLOR32(255, 0, 128, 255);          //Light blue
    color_scheme.time =                     COLOR32(255, 0, 128, 255);          //Light blue
    color_scheme.win_bg =                   COLOR32(255, 200, 200, 200);        //Grey
    color_scheme.win_shade =                COLOR32(255, 20, 20, 20);           //Very-very dark grey
    color_scheme.win_title =                COLOR32(255, 0, 0, 0);              //Black
    color_scheme.win_border =               COLOR32(255, 0, 116, 53);           //Dark green
    color_scheme.win_exit_btn =             COLOR32(255, 255, 0, 0);            //Red
    color_scheme.win_state_btn =            COLOR32(255, 255, 255, 0);          //Yellow
    color_scheme.win_minimize_btn =         COLOR32(255, 0, 255, 0);            //Lime
    color_scheme.win_unavailable_btn =      COLOR32(255, 40, 40, 40);           //Very dark grey
    
    //Allocate some space for the windows
    gfx_verbose_println("Allocating memory for windows");
    windows = (window_t*)calloc(32, sizeof(window_t));
    gfx_verbose_println("GUI init done");
}

/*
 * Creates a window and adds it to the window list
 */
window_t* gui_create_window(char* title, uint32_t flags, p2d_t pos, p2d_t size){
    window_t win;
    //Allocate memory for its title
    win.title = (char*)malloc(sizeof(char) * (strlen(title) + 1));
    //Copy the title over
    memcpy(win.title, title, strlen(title) + 1);
    //Assign flags, position and size
    win.flags = flags;
    win.position = pos;
    win.size_real = size;
    //Allocate some space for window controls
    win.controls = (control_t*)calloc(32, sizeof(control_t));
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
    //If the window we're destroying was in focus, reset it
    if(win == window_focused)
        window_focused = NULL;
    //Free up the memory used by its controls
    control_t* control;
    uint32_t i = 0;
    while((control = &win->controls[i++])->type){
        //free(control->extended);
        //free(control);
    }
    //Free up the memory used by the window title
    //free(win->title);
    //Free up the memory used by the window itself
    //free(win);
    //Scan through the window list to determine its end
    window_t* last;
    i = 0;
    while((last = &windows[i++])->title);
    i--;
    //Calculate index of the deleted window in the window list
    uint32_t idx = ((uint64_t)win - (uint64_t)windows) / sizeof(window_t);
    //Move the windows that stand after one that we just deleted, one to the left
    for(uint32_t j = idx + 1; j < i; j++){
        windows[j - 1] = windows[j];
    }
    //Mark the last window as the last one
    windows[i - 1].title = NULL;
}

/*
 * Creates a control and adds it to the window
 */
control_t* gui_create_control(window_t* win, uint32_t type, void* ext_ptr, p2d_t pos, p2d_t size){
    //Allocate memory for the control
    control_t cont;
    //Set its parameters
    cont.type = type;
    cont.extended = ext_ptr;
    cont.position = pos;
    cont.size = size;
    //Scan through the control list to determine its end
    control_t* last;
    uint32_t i = 0;
    while((last = &win->controls[i++])->type);
    i--;
    //Assign the control
    win->controls[i] = cont;
    //Mark the end of the list
    win->controls[i + 1].type = 0;
    //Return the control
    return &win->controls[i];
}

/*
 * Creates a label and adds it to the window
 */
control_t* gui_create_label(window_t* win, p2d_t pos, p2d_t size, char* text, color32_t text_color, color32_t bg_color){
    //Create the "extended control" of label type
    control_ext_label_t* label = (control_ext_label_t*)malloc(sizeof(control_ext_label_t));
    //Allocate memory for the label text
    label->text = malloc(sizeof(char) * (strlen(text) + 1));
    //Copy the text over
    memcpy(label->text, text, strlen(text) + 1);
    //Assign other parameters
    label->bg_color = bg_color;
    label->text_color = text_color;
    //Create a normal control with this extension
    return gui_create_control(win, GUI_WIN_CTRL_LABEL, (void*)label, pos, size);
}

/*
 * Creates a button and adds it to the window
 */
control_t* gui_create_button(window_t* win, p2d_t pos, p2d_t size, char* text, void (*event_handler)(ui_event_args_t*),
                             color32_t text_color, color32_t bg_color, color32_t pressed_bg_color, color32_t border_color){
    //Create the "extended control" of button type
    control_ext_button_t* button = (control_ext_button_t*)malloc(sizeof(control_ext_button_t));
    //Allocate memory for the label text
    button->text = malloc(sizeof(char) * (strlen(text) + 1));
    //Copy the text over
    memcpy(button->text, text, strlen(text) + 1);
    //Assign other parameters
    button->bg_color = bg_color;
    button->text_color = text_color;
    button->border_color = border_color;
    button->event_handler = event_handler;
    button->pressed_bg_color = pressed_bg_color;
    button->pressed_last_frame = 0;
    //Create a normal control with this extension
    return gui_create_control(win, GUI_WIN_CTRL_BUTTON, (void*)button, pos, size);
}

/*
 * Creates a progress bar and adds it to the window
 */
control_t* gui_create_progress_bar(window_t* win, p2d_t pos, p2d_t size, color32_t bg_color, color32_t fill_color,
                                   color32_t border_color, uint32_t max_val, uint32_t val){
    //Create the "extended control" of progress bar type
    control_ext_progress_t* progress = (control_ext_progress_t*)malloc(sizeof(control_ext_progress_t));
    //Assign the parameters
    progress->bg_color = bg_color;
    progress->border_color = border_color;
    progress->fill_color = fill_color;
    progress->max_val = max_val;
    progress->val = val;
    //Create a normal control with this extension
    return gui_create_control(win, GUI_WIN_CTRL_PROGRESS_BAR, (void*)progress, pos, size);
}

/*
 * Creates a progress bar and adds it to the window
 */
control_t* gui_create_image(window_t* win, p2d_t pos, p2d_t size, uint32_t format, void* data, color32_t color_lo, color32_t color_hi){
    //Create the "extended control" of image type
    control_ext_image_t* image = (control_ext_image_t*)malloc(sizeof(control_ext_image_t));
    //Assign properties
    image->image_format = format;
    image->image = data;
    image->color_lo = color_lo;
    image->color_hi = color_hi;
    //Create a normal control with this extension
    return gui_create_control(win, GUI_WIN_CTRL_IMAGE, (void*)image, pos, size);
}

/*
 * Creates a track bar and adds it to the window
 */
control_t* gui_create_track_bar(window_t* win, p2d_t pos, p2d_t size, color32_t bg_color, color32_t fill_color,
                                color32_t border_color, uint32_t max_val, uint32_t val, void(*callback)(ui_event_args_t*)){
    //Create the "extended control" of progress bar type
    control_ext_track_bar_t* track = (control_ext_track_bar_t*)malloc(sizeof(control_ext_track_bar_t));
    //Assign the parameters
    track->bg_color = bg_color;
    track->border_color = border_color;
    track->fill_color = fill_color;
    track->max_val = max_val;
    track->val = val;
    track->callback = callback;
    //Create a normal control with this extension
    return gui_create_control(win, GUI_WIN_CTRL_TRACK_BAR, (void*)track, pos, size);
}

/*
 * Polls PS/2 mouse
 */
void gui_poll_ps2(void){
    ps2_poll(&mx, &my, &ml, &mr);
}

/*
 * Redraw the GUI
 */
void gui_update(void){
    uint32_t render_start = pit_ticks();

    //Poll the PS/2 controller
    gui_poll_ps2();
    //Draw the desktop
    gfx_fill(color_scheme.desktop);
    //Draw the top bar
    gfx_draw_filled_rect((p2d_t){.x = 0, .y = 0}, (p2d_t){.x = gfx_res_x(), .y = 16}, color_scheme.top_bar);
    
    //Get the time
    uint8_t h, m, s = 0;
    if(read_rtc_time(&h, &m, &s)){
        //Clear the time string
        time[0] = 0;
        //Create a temporary local string
        char temp[64];
        temp[0] = 0;
        strcat(time, temp);
        //Append hours
        strcat(time, sprintu(temp, h, 2));
        strcat(time, ":");
        //Append minutes
        strcat(time, sprintu(temp, m, 2));
        strcat(time, ":");
        //Append seconds
        strcat(time, sprintu(temp, s, 2));
    }
    //Print it
    gfx_puts((p2d_t){.x = gfx_res_x() - gfx_text_bounds(time).x - 4, .y = 5}, color_scheme.time, COLOR32(0, 0, 0, 0), time);

    //Draw the power icon
    gfx_draw_xbm((p2d_t){.x = gfx_res_x() - gfx_text_bounds(time).x - 4 - 4 - 16, .y = 0}, power_bits, (p2d_t){.x = power_width, .y = power_height},
                 COLOR32(255, 255, 0, 0), COLOR32(0, 0, 0, 0));
    //Call the callback if it was pressed
    if(ml && !last_frame_ml && gfx_point_in_rect((p2d_t){.x = mx, .y = my}, (p2d_t){.x = gfx_res_x() - gfx_text_bounds(time).x - 4 - 4 - 16, .y = 0}, (p2d_t){.x = 16, .y = 16}))
        quark_gui_callback_power_pressed();
    //Draw the system icon
    gfx_draw_xbm((p2d_t){.x = gfx_res_x() - gfx_text_bounds(time).x - 4 - 4 - 16 - 8 - 16, .y = 0}, system_bits, (p2d_t){.x = system_width, .y = system_height},
                 COLOR32(255, 255, 255, 255), COLOR32(0, 0, 0, 0));

    //Call the callback if it was pressed
    if(ml && !last_frame_ml && gfx_point_in_rect((p2d_t){.x = mx, .y = my}, (p2d_t){.x = gfx_res_x() - gfx_text_bounds(time).x - 4 - 4 - 16 - 8 - 16, .y = 0}, (p2d_t){.x = 16, .y = 16}))
        quark_gui_callback_system_pressed();

    //Render the windows
    gui_render_windows();
    //Draw the cursor
    gui_draw_cursor(mx, my);
    //Flip the buffers
    uint32_t flip_start = pit_ticks();
    gfx_flip();
    uint32_t all_end = pit_ticks();

    /*
    char temp[50];
    temp[0] = 0;
    char temp2[25];
    strcat(temp, sprintu(temp2, flip_start - render_start, 1));
    strcat(temp, "\n");
    strcat(temp, sprintu(temp2, all_end - flip_start, 1));
    gfx_set_buf(GFX_BUF_VBE);
    gfx_puts((p2d_t){.x = 0, .y = 0}, COLOR32(255, 255, 255, 255), COLOR32(255, 0, 0, 0), temp);
    gfx_set_buf(GFX_BUF_SEC);
    */

    //Record the mouse state
    last_frame_ml = ml;
}

/*
 * Calls gui_process_window() and gui_render_window() according to the window order
 */
void gui_render_windows(void){
    //Some local variables
    window_t* current_window;
    //Clear the focus processed flag
    focus_processed = 0;
    //Reset the top bar position
    topb_win_pos = 0;

    //If the window in focus is valid
    if(window_focused != NULL) //Process the window in focus first
        gui_process_window(window_focused);
    //Scan through the window list to determine its end
    window_t* last;
    uint32_t i = 0;
    while((last = &windows[i++])->title);
    i--;
    //Process windows from the end of the list
    while(i--){
        if(&windows[i] != window_focused)
            gui_process_window(&windows[i]);
    }

    //Reset the counter
    uint16_t win_cnt = 0;
    i = 0;
    //Fetch the next window
    while((current_window = &windows[i++])->title){
        //Draw the highlight in the top bar if the window is in focus
        if(window_focused == current_window)
            gfx_draw_filled_rect((p2d_t){.x = topb_win_pos, .y = 0},
                                 (p2d_t){.x = 16, .y = 16}, color_scheme.selection);
        //Draw the window icon in the top bar
        gfx_draw_filled_rect((p2d_t){.x = topb_win_pos + 4, .y = 4},
                             (p2d_t){.x = 8, .y = 8}, color_scheme.win_bg);
        gfx_draw_filled_rect((p2d_t){.x = topb_win_pos + 5, .y = 5},
                             (p2d_t){.x = 3, .y = 6}, COLOR32(255, 0, 64, 255));
        gfx_draw_hor_line((p2d_t){.x = topb_win_pos + 9, .y = 5}, 2, color_scheme.win_title);
        //Advance the bar position
        topb_win_pos += 16;

        //Render the current window if it isn't in focus
        if(current_window != window_focused)
            gui_render_window(current_window);

        //Increment the window counter
        win_cnt++;
    }

    //If the window in focus is valid
    if(window_focused != NULL) //Render the window in focus last
        gui_render_window(window_focused);
    
    //Set the window in focus according to the top bar clicks
    if(ml && mx / 16 <= win_cnt && !focus_monopoly){
        window_focused = &windows[mx / 16];
    }
}

/*
 * Renders a window
 */
void gui_render_window(window_t* ptr){
    //Only render the window if it has the visibility flag set
    if(ptr->flags & GUI_WIN_FLAG_VISIBLE){
        //Draw the shade
        gfx_draw_filled_rect((p2d_t){.x = ptr->position.x + ptr->size.x + 1,
                                     .y = ptr->position.y + 4}, (p2d_t){.x = 4, .y = ptr->size.y + 1}, color_scheme.win_shade);
        gfx_draw_filled_rect((p2d_t){.x = ptr->position.x + 4,
                                     .y = ptr->position.y + ptr->size.y + 1}, (p2d_t){.x = ptr->size.x + 1, .y = 4}, color_scheme.win_shade);
        //Fill a rectangle with a window background color
        gfx_draw_filled_rect((p2d_t){.x = ptr->position.x, .y = ptr->position.y},
                             (p2d_t){.x = ptr->size.x, .y = ptr->size.y}, color_scheme.win_bg);
        //Draw a border around it
        gfx_draw_rect((p2d_t){.x = ptr->position.x, .y = ptr->position.y},
                      (p2d_t){.x = ptr->size.x, .y = ptr->size.y}, color_scheme.win_border);
        //Draw a background for the title
        gfx_draw_filled_rect((p2d_t){.x = ptr->position.x, .y = ptr->position.y}, 
                             (p2d_t){.x = ptr->size.x, .y = 11}, color_scheme.win_border);
        //Print its title
        gfx_puts((p2d_t){.x = ptr->position.x + 2, .y = ptr->position.y + 2}, color_scheme.win_title, COLOR32(0, 0, 0, 0), ptr->title);

        //Draw the close button 
        if(ptr->flags & GUI_WIN_FLAG_CLOSABLE)
            gfx_draw_filled_rect((p2d_t){.x = ptr->position.x + ptr->size.x - 10, .y = ptr->position.y + 2}, (p2d_t){.x = 8, .y = 8}, color_scheme.win_exit_btn);
        else
            gfx_draw_filled_rect((p2d_t){.x = ptr->position.x + ptr->size.x - 10, .y = ptr->position.y + 2}, (p2d_t){.x = 8, .y = 8}, color_scheme.win_unavailable_btn);

        //Draw the maximize (state change) button 
        if(ptr->flags & GUI_WIN_FLAG_MAXIMIZABLE)
            gfx_draw_filled_rect((p2d_t){.x = ptr->position.x + ptr->size.x - 19, .y = ptr->position.y + 2}, (p2d_t){.x = 8, .y = 8}, color_scheme.win_state_btn);
        else
            gfx_draw_filled_rect((p2d_t){.x = ptr->position.x + ptr->size.x - 19, .y = ptr->position.y + 2}, (p2d_t){.x = 8, .y = 8}, color_scheme.win_unavailable_btn);

        //Draw the minimize button 
        if(ptr->flags & GUI_WIN_FLAG_MINIMIZABLE)
            gfx_draw_filled_rect((p2d_t){.x = ptr->position.x + ptr->size.x - 28, .y = ptr->position.y + 2}, (p2d_t){.x = 8, .y = 8}, color_scheme.win_minimize_btn);
        else
            gfx_draw_filled_rect((p2d_t){.x = ptr->position.x + ptr->size.x - 28, .y = ptr->position.y + 2}, (p2d_t){.x = 8, .y = 8}, color_scheme.win_unavailable_btn);

        //Now draw its controls
        uint32_t i = 0;
        control_t* control;
        while((control = &ptr->controls[i++])->type)
            gui_render_control(ptr, control);
    }
}

/*
 * Processes window's interaction with the mouse
 */
void gui_process_window(window_t* ptr){
    //Set the size to the real one as the proper processing hadn't been implemented yet
    ptr->size = ptr->size_real;
    //Only process the window if it has the visibility flag set
    if(ptr->flags & GUI_WIN_FLAG_VISIBLE){
        //Process window dragging
        //If there's no such window that's being dragged right now, the cursor is in bounds of the title
        //  and the left button is being pressed, assume the window we're dragging is this one
        if(window_dragging == NULL &&
            ml && gfx_point_in_rect((p2d_t){.x = mx, .y = my},
                                    (p2d_t){.x = ptr->position.x + 1, .y = ptr->position.y + 1},
                                    (p2d_t){.x = ptr->size.x - 2, .y = 9})){
                window_dragging = ptr;
                window_dragging_cpos = (p2d_t){.x = ptr->position.x - mx, .y = ptr->position.y - my};
        }
        //If the window we're dragging is this one, drag it
        if(window_dragging == ptr){
            if(ml){
                //If the left mouse button is still being pressed, drag the window
                ptr->position = (p2d_t){.x = mx + window_dragging_cpos.x, .y = my + window_dragging_cpos.y};
                //Constrain the window coordinates
                if(ptr->position.x < 0)
                    ptr->position.x = 0;
                if(ptr->position.y < 16) //The top bar is 16 pixels T H I C C
                    ptr->position.y = 16;
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
        //If the cursor is inside the current window, the focusing hadn't been done in the current frame,
        //  focus monopoly isn't enabled and the left button is held down, set the window in focus and set the "focus processed" flag
        if(!focus_monopoly &&
           ml &&
           !focus_processed &&
           gfx_point_in_rect((p2d_t){.x = mx, .y = my},
                             (p2d_t){.x = ptr->position.x + 1, .y = ptr->position.y + 1},
                             (p2d_t){.x = ptr->size.x - 2, .y = ptr->size.y - 2})){
            focus_processed = 1;
            window_focused = ptr;
        }

        //Process the top-right buttons
        if(gfx_point_in_rect((p2d_t){.x = mx, .y = my}, (p2d_t){.x = ptr->position.x + ptr->size.x - 10, .y = ptr->position.y + 2}, (p2d_t){.x = 8, .y = 8})
           && (ptr->flags & GUI_WIN_FLAG_CLOSABLE) && ml){
            //Destroy this window
            gui_destroy_window(ptr);
        }

        //Now process its controls
        uint8_t process_ptr = gfx_point_in_rect((p2d_t){.x = mx, .y = my}, ptr->position, ptr->size);
        uint32_t i = 0;
        control_t* control;
        while((control = &ptr->controls[i++])->type)
            gui_process_control(ptr, control, process_ptr);
    }
}

/*
 * Renders a control
 */
void gui_render_control(window_t* win_ptr, control_t* ptr){
    //Check control's type
    switch(ptr->type){
        case GUI_WIN_CTRL_LABEL: {
            //Fetch the extended data
            control_ext_label_t* label = (control_ext_label_t*)ptr->extended;
            //Draw the label
            gfx_puts((p2d_t){.x = ptr->position.x + win_ptr->position.x + 1, .y = ptr->position.y + win_ptr->position.y + 12},
                     label->text_color, label->bg_color, label->text);
        }
        break;
        case GUI_WIN_CTRL_BUTTON: {
            //Fetch the extended data
            control_ext_button_t* button = (control_ext_button_t*)ptr->extended;
            //Full transparency = we choose the color on our own
            if(button->bg_color.a == 0)
                button->bg_color = COLOR32(0, color_scheme.win_border.r, color_scheme.win_border.g, color_scheme.win_border.b);
            if(button->border_color.a == 0)
                button->border_color = COLOR32(0, color_scheme.win_border.r, color_scheme.win_border.g, color_scheme.win_border.b);
            if(button->pressed_bg_color.a == 0)
                button->pressed_bg_color = COLOR32(0, color_scheme.win_border.r >> 1, color_scheme.win_border.g >> 1, color_scheme.win_border.b >> 1);
            //Draw the rectangles
            gfx_draw_filled_rect((p2d_t){.x = ptr->position.x + win_ptr->position.x + 1,
                                         .y = ptr->position.y + win_ptr->position.y + 12},
                                 ptr->size, button->pressed_last_frame ? button->pressed_bg_color : button->bg_color);
            gfx_draw_rect((p2d_t){.x = ptr->position.x + win_ptr->position.x + 1,
                                  .y = ptr->position.y + win_ptr->position.y + 12},
                          ptr->size, button->border_color);
            //Calculate text bounds
            p2d_t t_bounds = gfx_text_bounds(button->text);
            //Draw the text
            gfx_puts((p2d_t){.x = ptr->position.x + win_ptr->position.x + 1  + ((ptr->size.x - t_bounds.x) / 2),
                             .y = ptr->position.y + win_ptr->position.y + 12 + ((ptr->size.y - t_bounds.y) / 2)},
                     button->text_color, COLOR32(0, 0, 0, 0), button->text);
        }
        break;
        case GUI_WIN_CTRL_PROGRESS_BAR: {
            //Fetch the extended data
            control_ext_progress_t* progress = (control_ext_progress_t*)ptr->extended;
            //Draw the rectangles
            gfx_draw_filled_rect((p2d_t){.x = ptr->position.x + win_ptr->position.x + 1,
                                         .y = ptr->position.y + win_ptr->position.y + 12},
                                 ptr->size, progress->bg_color);
            gfx_draw_filled_rect((p2d_t){.x = ptr->position.x + win_ptr->position.x + 1,
                                         .y = ptr->position.y + win_ptr->position.y + 12},
                                 (p2d_t){.x = ptr->size.x * progress->val / progress->max_val,
                                         .y = ptr->size.y},
                                 progress->fill_color);
            gfx_draw_rect((p2d_t){.x = ptr->position.x + win_ptr->position.x + 1,
                                  .y = ptr->position.y + win_ptr->position.y + 12},
                          ptr->size, progress->border_color);
        }
        break;
        case GUI_WIN_CTRL_IMAGE: {
            //Fetch the extended data
            control_ext_image_t* image = (control_ext_image_t*)ptr->extended;
            //Draw the image
            if(image->image_format == GUI_IMAGE_FORMAT_XBM)
                gfx_draw_xbm((p2d_t){.x = ptr->position.x + win_ptr->position.x + 1,
                                     .y = ptr->position.y + win_ptr->position.y + 12},
                             (uint8_t*)image->image, ptr->size, image->color_hi, image->color_lo);
            if(image->image_format == GUI_IMAGE_FORMAT_RAW)
                gfx_draw_raw((p2d_t){.x = ptr->position.x + win_ptr->position.x + 1,
                                     .y = ptr->position.y + win_ptr->position.y + 12},
                             (uint8_t*)image->image, ptr->size);
        }
        break;
        case GUI_WIN_CTRL_TRACK_BAR: {
            //Fetch the extended data
            control_ext_track_bar_t* track = (control_ext_track_bar_t*)ptr->extended;
            //Draw the rectangles
            uint32_t fill_width = ptr->size.x * track->val / track->max_val;
            gfx_draw_filled_rect((p2d_t){.x = ptr->position.x + win_ptr->position.x + 1,
                                         .y = ptr->position.y + win_ptr->position.y + 12}, ptr->size, track->bg_color);
            gfx_draw_filled_rect((p2d_t){.x = ptr->position.x + win_ptr->position.x + 1,
                                         .y = ptr->position.y + win_ptr->position.y + 12},
                                 (p2d_t){.x = fill_width, .y = ptr->size.y}, track->fill_color);
            gfx_draw_rect((p2d_t){.x = ptr->position.x + win_ptr->position.x + 1,
                                  .y = ptr->position.y + win_ptr->position.y + 12}, ptr->size, track->border_color);
            gfx_draw_filled_rect((p2d_t){.x = ptr->position.x + win_ptr->position.x + fill_width + 1 - 3,
                                         .y = ptr->position.y + win_ptr->position.y + ptr->size.y + 3},
                                 (p2d_t){.x = 6, .y = ptr->size.y + 6}, track->bg_color);
            gfx_draw_rect((p2d_t){.x = ptr->position.x + win_ptr->position.x + fill_width + 1 - 3,
                                  .y = ptr->position.y + win_ptr->position.y + ptr->size.y + 3},
                          (p2d_t){.x = 6, .y = ptr->size.y + 6}, track->border_color);
        }
        break;
    }
}

/*
 * Processes control's interaction with the mouse
 */
void gui_process_control(window_t* win_ptr, control_t* ptr, uint8_t handle_pointer){
    //Check control's type
    switch(ptr->type){
        case GUI_WIN_CTRL_LABEL: {}
        break;
        case GUI_WIN_CTRL_BUTTON: {
            //Fetch the extended data
            control_ext_button_t* button = (control_ext_button_t*)ptr->extended;
            //Check if the button is pressed
            uint8_t pressed = 0;
            if(handle_pointer)
                pressed = gfx_point_in_rect((p2d_t){.x = mx, .y = my}, 
                                            (p2d_t){.x = ptr->position.x + win_ptr->position.x + 1,
                                                    .y = ptr->position.y + win_ptr->position.y + 12},
                                            ptr->size) && ml;
            //Check if the button is clicked (pressed for the first frame)
            uint8_t clicked = 0;
            if(pressed && !button->pressed_last_frame)
                clicked = 1;
            button->pressed_last_frame = pressed;
            //Call the event handler in case of a click
            if(clicked && button->event_handler != NULL){
                ui_event_args_t event;
                event.control = ptr;
                event.win = win_ptr;
                event.type = GUI_EVENT_CLICK;
                event.mouse_pos = (p2d_t){.x = mx, .y = my};
                button->event_handler(&event);
            }
        }
        break;
        case GUI_WIN_CTRL_PROGRESS_BAR: {}
        break;
        case GUI_WIN_CTRL_IMAGE: {}
        break;
        case GUI_WIN_CTRL_TRACK_BAR: {
            //Fetch the extended data
            control_ext_track_bar_t* track = (control_ext_track_bar_t*)ptr->extended;
            //If the pointer is in bounds of the window
            if(handle_pointer){
                //The left mouse button is pressed and the cursor is in bounds
                if(ml && gfx_point_in_rect((p2d_t){.x = mx, .y = my},
                                           (p2d_t){.x = ptr->position.x + win_ptr->position.x + 1,
                                                   .y = ptr->position.y + win_ptr->position.y + 12},
                                           ptr->size)){
                    //Calculate the new value
                    uint32_t new_val = (mx - (ptr->position.x + win_ptr->position.x + 1)) * track->max_val / ptr->size.x;
                    if(new_val != track->val){
                        //If the value has changed, assign it and call the callback function
                        track->val = new_val;
                        ui_event_args_t args;
                        args.control = ptr;
                        args.mouse_pos = (p2d_t){.x = mx, .y = my};
                        args.type = GUI_EVENT_TRACK_BAR_CHANGE;
                        args.win = win_ptr;
                        track->callback(&args);
                    }
                }
            }
        }
        break;
    }
}

/*
 * Draws the cursor on screen
 */
void gui_draw_cursor(uint32_t x, uint32_t y){
    //Retrieve the graphics buffer; draw directly on it
    color32_t* buf = gfx_buffer();
    //Retrieve the resolution
    uint32_t res_x = gfx_res_x();
    uint32_t res_y = gfx_res_y();
    //Draw!
    if(x + 3 >= res_x)
        return;
    if(y >= res_y)
        return;
    buf[(y * res_x) + x] = color_scheme.cursor;
    buf[(y * res_x) + x + 1] = color_scheme.cursor;
    buf[(y * res_x) + x + 2] = color_scheme.cursor;
    if(y + 1 >= res_y)
        return;
    buf[((y + 1) * res_x) + x + 1] = color_scheme.cursor;
    buf[((y + 1) * res_x) + x] = color_scheme.cursor;
    if(y + 2 >= res_y)
        return;
    buf[((y + 2) * res_x) + x] = color_scheme.cursor;
    buf[((y + 2) * res_x) + x + 2] = color_scheme.cursor;
    if(y + 3 >= res_y)
        return;
    buf[((y + 3) * res_x) + x + 3] = color_scheme.cursor;
    if(y + 4 >= res_y)
        return;
    buf[((y + 4) * res_x) + x + 4] = color_scheme.cursor;
}

/*
 * Sets or clears the focus monopoly 
 */
void gui_set_focus_monopoly(uint8_t val){
    focus_monopoly = val;
}

/*
 * Returns a pointer to the color scheme that's being used by the GUI system
 */
color_scheme_t* gui_get_color_scheme(void){
    return &color_scheme;
}