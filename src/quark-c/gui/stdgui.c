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
                      COLOR32(255, 255, 255, 255), COLOR32(255, 0, 116, 255), COLOR32(255, 0, 116, 255), COLOR32(255, 0, 53, 128));
    gui_create_button(shutdown_window, (p2d_t){.x = shutdown_win_size.x / 2 - 2, .y = shutdown_win_size.y - 25 - 13}, (p2d_t){.x = shutdown_win_size.x / 2, .y = 25}, "Reboot", _stdgui_cb_reboot_pressed,
                      COLOR32(255, 255, 255, 255), COLOR32(255, 0, 116, 255), COLOR32(255, 0, 116, 255), COLOR32(255, 0, 53, 128));
}