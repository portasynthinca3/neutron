//Neutron Project
//Graphical User interface
//Built on top of the GFX library (drivers/gfx.c)
//Also uses separate files for general (gui.c), window (windows.c)
//  and control (controls.c) rendering and processing

#include "./gui.h"
#include "./windows.h"
#include "./controls.h"
#include "../stdlib.h"
#include "../drivers/gfx.h"
#include "../drivers/diskio.h"
#include "../drivers/timr.h"
#include "../drivers/human_io/mouse.h"
#include "../drivers/human_io/kbd.h"

#include "../images/power.xbm"
#include "../images/system.xbm"
#include "../images/cursor_white.xbm"
#include "../images/cursor_black.xbm"

//Mouse position on the screen
int32_t mx, my;
//Mouse buttons state
uint8_t ml, mr;
//Current color scheme
color_scheme_t color_scheme;
//Window position in the the top bar
uint16_t topb_win_pos;
//The current time as a string
char time[64] = "??:??:??\0";
//The current date as a string
char date[64] = "?/?/?\0";
//The amount of time it took to render the last frame
uint64_t gui_render = 0;
//The amount of time it took to transfer the last frame to the screen
uint64_t gui_trans = 0;

uint8_t last_frame_ml = 0;
void krnl_gui_callback_power_pressed(void);
void krnl_gui_callback_system_pressed(void);

control_ext_progress_t* example_progress_bar;

void gui_example_button_callback(ui_event_args_t* args){
    if(example_progress_bar->val == 0)
        example_progress_bar->val = 80;
    else if(example_progress_bar->val == 80)
        example_progress_bar->val = 100;
    else if(example_progress_bar->val == 100)
        example_progress_bar->val = 0;
}

/*
 * Performs some GUI initialization
 */
void gui_init(void){
    //Reset mouse state
    gfx_verbose_println("Resetting mouse state");
    ml = mr = 0;
    //Move the cursor to the center of the screen
    mx = gfx_res_x() / 2;
    my = gfx_res_y() / 2;
    gfx_verbose_println("Initializing color scheme");
    //Set the default color scheme
    color_scheme.desktop =                  COLOR32(255, 20, 20, 20);
    color_scheme.top_bar =                  COLOR32(255, 71, 71, 71);
    color_scheme.cursor =                   COLOR32(255, 255, 255, 255);
    color_scheme.selection =                COLOR32(255, 0, 128, 255);
    color_scheme.time =                     COLOR32(255, 255, 255, 255);
    color_scheme.win_bg =                   COLOR32(255, 32, 32, 32);
    color_scheme.win_title =                COLOR32(255, 255, 255, 255);
    color_scheme.win_title_inactive =       COLOR32(255, 200, 200, 200);
    color_scheme.win_border =               COLOR32(255, 9, 9, 9);
    color_scheme.win_border_inactive =      COLOR32(255, 39, 39, 39);
    color_scheme.win_exit_btn =             COLOR32(255, 255, 0, 0);
    color_scheme.win_state_btn =            COLOR32(255, 255, 255, 255);
    color_scheme.win_minimize_btn =         COLOR32(255, 255, 255, 255);
    color_scheme.win_unavailable_btn =      COLOR32(255, 128, 128, 128);

    gui_trans = 0;
    gui_render = 0;

    gui_init_windows();
}

/*
 * Gets mouse data
 */
void gui_get_mouse(void){
    mouse_abs(&mx, &my);
    mouse_buttons(&ml, &mr);
}

/*
 * Returns mouse cursor position
 */
p2d_t gui_mouse_coords(void){
    return (p2d_t){.x = mx, .y = my};
}

/*
 * Returns mouse button states
 */
p2d_t gui_mouse_btns(void){
    return (p2d_t){.x = ml, .y = mr};
}

/*
 * Redraw the GUI
 */
void gui_update(void){
    uint64_t render_start = timr_ms();

    //Get the mouse data
    gui_get_mouse();
    //Draw the desktop
    gfx_fill(color_scheme.desktop);
    //Draw the top bar
    gfx_draw_filled_rect((p2d_t){.x = 0, .y = 0}, (p2d_t){.x = gfx_res_x(), .y = 20}, color_scheme.top_bar);
    
    //Get the time and date
    uint16_t h, m, s, d, mo, y = 0;
    if(read_rtc_time(&h, &m, &s, &d, &mo, &y)){
        //Clear the date string
        date[0] = 0;
        //Create a temporary local string
        char temp[64];
        temp[0] = 0;
        //Append days
        strcat(date, sprintu(temp, d, 2));
        strcat(date, "/");
        //Append months
        strcat(date, sprintu(temp, mo, 2));
        strcat(date, "/");
        //Append years
        strcat(date, sprintu(temp, y, 2));

        //Clear the time string
        time[0] = 0;
        //Clear the temporary local string
        temp[0] = 0;
        //Append hours
        strcat(time, sprintu(temp, h, 2));
        strcat(time, ":");
        //Append minutes
        strcat(time, sprintu(temp, m, 2));
        strcat(time, ":");
        //Append seconds
        strcat(time, sprintu(temp, s, 2));
    }

    //Print them
    gfx_puts((p2d_t){.x = gfx_res_x() - gfx_text_bounds(date).x, .y = 2}, color_scheme.time, COLOR32(0, 0, 0, 0), date);
    gfx_puts((p2d_t){.x = gfx_res_x() - gfx_text_bounds(time).x - 4, .y = 12}, color_scheme.time, COLOR32(0, 0, 0, 0), time);

    //Draw the power icon
    gfx_draw_xbm((p2d_t){.x = gfx_res_x() - 60 - 16, .y = 2}, power_bits, (p2d_t){.x = power_width, .y = power_height},
                 COLOR32(255, 255, 0, 0), COLOR32(0, 0, 0, 0));
    //Call the callback if it was pressed
    if(ml && !last_frame_ml && gfx_point_in_rect((p2d_t){.x = mx, .y = my}, (p2d_t){.x = gfx_res_x() - 60 - 16, .y = 2}, (p2d_t){.x = 16, .y = 16}))
        krnl_gui_callback_power_pressed();

    //Draw the system icon
    gfx_draw_xbm((p2d_t){.x = gfx_res_x() - 60 - 16 - 8 - 16, .y = 2}, system_bits, (p2d_t){.x = system_width, .y = system_height},
                 COLOR32(255, 255, 255, 255), COLOR32(0, 0, 0, 0));
    //Call the callback if it was pressed
    if(ml && !last_frame_ml && gfx_point_in_rect((p2d_t){.x = mx, .y = my}, (p2d_t){.x = gfx_res_x() - 60 - 16 - 8 - 16, .y = 2}, (p2d_t){.x = 16, .y = 16}))
        krnl_gui_callback_system_pressed();

    //If there's a focused window
    if(gui_get_focused_window() != NULL){
        //Read the keyboard events and send them to the focused window
        kbd_event_t event;
        while(kbd_pop_event(&event)){
            if(gui_get_focused_window()->event_handler != NULL){
                ui_event_args_t args = (ui_event_args_t){.win = gui_get_focused_window(), .control = NULL, .type = GUI_EVENT_KEYBOARD,
                    .mouse_pos = (p2d_t){.x = mx, .y = my}, .extra_data = &event};
                gui_get_focused_window()->event_handler(&args);
            }
        }
    }
    //Else, just flush the event buffer
    else {
        while(kbd_pop_event(NULL));
    }

    //Render and process the windows
    gui_render_windows();
    //Draw the cursor
    gui_draw_cursor(mx, my);

    #ifdef GUI_PRINT_RENDER_TIME
    char temp[25];
    gfx_puts((p2d_t){.x = 0, .y = 16}, COLOR32(255, 255, 255, 255), COLOR32(255, 0, 0, 0), sprintu(temp, gui_render, 5));
    gfx_puts((p2d_t){.x = 0, .y = 24}, COLOR32(255, 255, 255, 255), COLOR32(255, 0, 0, 0), sprintu(temp, gui_trans, 5));
    gfx_puts((p2d_t){.x = 0, .y = 32}, COLOR32(255, 255, 255, 255), COLOR32(255, 0, 0, 0), sprintu(temp, timr_ms(), 8));
    #endif

    //Flip the buffers
    uint64_t flip_start = timr_ms();
    gfx_flip();
    uint64_t all_end = timr_ms();

    gui_render = flip_start - render_start;
    gui_trans = all_end - flip_start;

    //Record the mouse state
    last_frame_ml = ml;
}

/*
 * Calls gui_process_window() and gui_render_window() according to the window order
 */
void gui_render_windows(void){
    //Some local variables
    window_t* current_window;
    uint32_t i = 0;
    uint32_t win_cnt = 0;
    //Reset the top bar position
    topb_win_pos = 2;

    uint8_t process_non_focus = 0;
    //If the window in focus is valid
    if(gui_get_focused_window() != NULL) //Process it first
        process_non_focus = gui_process_window(gui_get_focused_window());
    else
        process_non_focus = 1;
    //Scan through the window list to determine its end
    while((current_window = &gui_get_window_list()[i++])->title) win_cnt++;
    //Process windows from the end of the list
    if(process_non_focus)
        for(int32_t j = win_cnt - 1; j >= 0; j--)
            if(&gui_get_window_list()[j] != gui_get_focused_window())
                if(!gui_process_window(&gui_get_window_list()[j]))
                    break; //Don't process other gui_get_window_list() if this one is blocking others

    //Reset the counter
    i = 0;
    //Fetch the next window
    while((current_window = &gui_get_window_list()[i++])->title){
        //Draw the highlight in the top bar if the window is in focus
        if(gui_get_focused_window() == current_window)
            gfx_draw_filled_rect((p2d_t){.x = topb_win_pos, .y = 2},
                                 (p2d_t){.x = 16, .y = 16}, color_scheme.selection);
        //Draw the window icon in the top bar
        if(current_window->icon_8 == NULL){
            gfx_draw_filled_rect((p2d_t){.x = topb_win_pos + 4, .y = 6},
                                 (p2d_t){.x = 8, .y = 8}, color_scheme.win_bg);
            gfx_draw_filled_rect((p2d_t){.x = topb_win_pos + 5, .y = 7},
                                 (p2d_t){.x = 3, .y = 6}, COLOR32(255, 0, 64, 255));
            gfx_draw_hor_line((p2d_t){.x = topb_win_pos + 9, .y = 7}, 2, color_scheme.win_title);
        } else {
            gfx_draw_raw((p2d_t){.x = topb_win_pos + 4, .y = 6}, current_window->icon_8, (p2d_t){.x = 8, .y = 8});
        }
        //Advance the bar position
        topb_win_pos += 16;

        //Render the current window if it isn't in focus
        if(current_window != gui_get_focused_window())
            gui_render_window(current_window);
    }

    //If the window in focus is valid
    if(gui_get_focused_window() != NULL) //Render the window in focus last
        gui_render_window(gui_get_focused_window());
    
    //Set the window in focus according to the top bar clicks
    if(ml && (mx / 16 <= win_cnt)){
        gui_set_focused_window(&gui_get_window_list()[mx / 16]);
        //Clear window minimized flag
        gui_get_focused_window()->flags &= ~GUI_WIN_FLAG_MINIMIZED;
    }
}

/*
 * Draws the cursor on screen
 */
void gui_draw_cursor(uint32_t x, uint32_t y){
    //Draw two XBM images: one white, and one black
    gfx_draw_xbm((p2d_t){.x = mx, .y = my}, cursor_white_bits, (p2d_t){.x = cursor_white_width, .y = cursor_white_height},
                 COLOR32(255, 255, 255, 255), COLOR32(0, 0, 0, 0));
    gfx_draw_xbm((p2d_t){.x = mx, .y = my}, cursor_black_bits, (p2d_t){.x = cursor_black_width, .y = cursor_black_height},
                 COLOR32(255, 0, 0, 0), COLOR32(0, 0, 0, 0));
}

/*
 * Returns a pointer to the color scheme that's being used by the GUI system
 */
color_scheme_t* gui_get_color_scheme(void){
    return &color_scheme;
}