//Neutron project
//Standard GUI windows

#include "../stdlib.h"
#include "../drivers/gfx.h"
#include "./gui.h"

//Are defined in Quark
void quark_shutdown(void);
void quark_reboot(void);

void _stdgui_cb_shutdown_pressed(ui_event_args_t* args){
    quark_shutdown();
}
void _stdgui_cb_reboot_pressed(ui_event_args_t* args){
    quark_reboot();
}

//Image that shows the currently selected color in the color picker
uint8_t* cpick_img_buf;
//Color that is currently selected using the color picker
color32_t cpick_color;

void _stdgui_fill_img(void){
    uint32_t pos = 0;
    for(uint32_t y = 0; y < 40; y++){
        for(uint32_t x = 0; x < 40; x++){
            cpick_img_buf[pos++] = cpick_color.r;
            cpick_img_buf[pos++] = cpick_color.g;
            cpick_img_buf[pos++] = cpick_color.b;
        }
    }
}
void _stdgui_cb_color_r(ui_event_args_t* args){
    cpick_color.r = ((control_ext_track_bar_t*)args->control->extended)->val;
    _stdgui_fill_img();
}
void _stdgui_cb_color_g(ui_event_args_t* args){
    cpick_color.g = ((control_ext_track_bar_t*)args->control->extended)->val;
    _stdgui_fill_img();
}
void _stdgui_cb_color_b(ui_event_args_t* args){
    cpick_color.b = ((control_ext_track_bar_t*)args->control->extended)->val;
    _stdgui_fill_img();
}

/*
 * Creates a shutdown prompt window
 */
void stdgui_create_shutdown_prompt(void){
    p2d_t shutdown_win_size = (p2d_t){.x = 300, .y = 150};
    //Create a window
    window_t* shutdown_window = gui_create_window("Shutdown", GUI_WIN_FLAGS_STANDARD,
                                                  (p2d_t){.x = (gfx_res_x() - shutdown_win_size.x) / 2,
                                                          .y = (gfx_res_y() - shutdown_win_size.y) / 2}, shutdown_win_size);
    //Create a label and two buttons
    char* msg = "What do you want to do?";
    p2d_t msg_bounds = gfx_text_bounds(msg);
    gui_create_label(shutdown_window, (p2d_t){.x = (shutdown_win_size.x - msg_bounds.x) / 2,
                                              .y = (shutdown_win_size.y - msg_bounds.y - 25) / 2}, msg_bounds, msg, COLOR32(255, 0, 0, 0), COLOR32(0, 0, 0, 0));
    gui_create_button(shutdown_window, (p2d_t){.x = 0, .y = shutdown_win_size.y - 25 - 13}, (p2d_t){.x = shutdown_win_size.x / 2, .y = 25}, "Shutdown", _stdgui_cb_shutdown_pressed,
                      COLOR32(255, 255, 255, 255), COLOR32(0, 0, 0, 0), COLOR32(0, 0, 0, 0), COLOR32(0, 0, 0, 0));
    gui_create_button(shutdown_window, (p2d_t){.x = shutdown_win_size.x / 2 - 2, .y = shutdown_win_size.y - 25 - 13}, (p2d_t){.x = shutdown_win_size.x / 2, .y = 25}, "Reboot", _stdgui_cb_reboot_pressed,
                      COLOR32(255, 255, 255, 255), COLOR32(0, 0, 0, 0), COLOR32(0, 0, 0, 0), COLOR32(0, 0, 0, 0));
}

/*
 * Creates a color picker
 */
uint8_t stdgui_create_color_picker(void (*callback)(ui_event_args_t*), color32_t start){
    p2d_t win_size = (p2d_t){.x = 157, 80};
    cpick_color = start;
    //Create the window
    window_t* window = gui_create_window("Choose color", GUI_WIN_FLAGS_STANDARD, (p2d_t){.x = (gfx_res_x() - win_size.x) / 2,
                                                                                         .y = (gfx_res_y() - win_size.y) / 2}, win_size);
    //Create a buffer for the image
    cpick_img_buf = (uint8_t*)malloc(40 * 40 * 3);
    _stdgui_fill_img();
    //Create the image
    gui_create_image(window, (p2d_t){.x = 5, .y = 5}, (p2d_t){.x = 40, .y = 40}, GUI_IMAGE_FORMAT_RAW, cpick_img_buf, COLOR32(0, 0, 0, 0), COLOR32(0, 0, 0, 0));
    //Create a red track bar
    control_ext_track_bar_t* track_bar_r = (control_ext_track_bar_t*)
    gui_create_track_bar(window, (p2d_t){.x = 50, .y = 5}, (p2d_t){.x = 100, .y = 6}, COLOR32(255, 255, 255, 255), COLOR32(255, 255, 0, 0), COLOR32(255, 0, 0, 0),
                         255, 128, _stdgui_cb_color_r)->extended;
    track_bar_r->val = start.r;
    //Create a green track bar
    control_ext_track_bar_t* track_bar_g = (control_ext_track_bar_t*)
    gui_create_track_bar(window, (p2d_t){.x = 50, .y = 20}, (p2d_t){.x = 100, .y = 6}, COLOR32(255, 255, 255, 255), COLOR32(255, 0, 255, 0), COLOR32(255, 0, 0, 0),
                         255, 128, _stdgui_cb_color_g)->extended;
    track_bar_g->val = start.g;
    //Create a blue track bar
    control_ext_track_bar_t* track_bar_b = (control_ext_track_bar_t*)
    gui_create_track_bar(window, (p2d_t){.x = 50, .y = 35}, (p2d_t){.x = 100, .y = 6}, COLOR32(255, 255, 255, 255), COLOR32(255, 0, 0, 255), COLOR32(255, 0, 0, 0),
                         255, 128, _stdgui_cb_color_b)->extended;
    track_bar_b->val = start.b;
    //Create an OK button
    control_ext_button_t* ok_btn = (control_ext_button_t*)
    gui_create_button(window, (p2d_t){.x = 5, .y = 50}, (p2d_t){.x = 145, .y = 12}, "OK", NULL,
                      COLOR32(255, 255, 255, 255), COLOR32(0, 0, 0, 0), COLOR32(0, 0, 0, 0), COLOR32(0, 0, 0, 0))->extended;
    //Assign an event handler to it
    ok_btn->event_handler = callback;
}

/*
 * Returns the color that was selected by a color picker
 */
color32_t stdgui_cpick_get_color(void){
    return cpick_color;
}