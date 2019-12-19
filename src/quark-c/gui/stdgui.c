//Neutron project
//Standard GUI windows

#include "../stdlib.h"
#include "../drivers/gfx.h"
#include "./gui.h"

#include "../images/neutron_logo.xbm"

//These are defined in Quark
void quark_shutdown(void);
void quark_reboot(void);
void quark_open_sys_color_picker(ui_event_args_t* args);

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
 * Creates a system control window
 */
void stdgui_create_system_win(void){
    p2d_t system_win_size = (p2d_t){.x = 300, .y = 300};
    //Create the window itself
    window_t* window = gui_create_window("System", GUI_WIN_FLAGS_STANDARD,
                                         (p2d_t){.x = (gfx_res_x() - system_win_size.x) / 2,
                                                 .y = (gfx_res_y() - system_win_size.y) / 2}, system_win_size);
    //Add the Neutron logo to it
    gui_create_image(window,
                     (p2d_t){.x = (system_win_size.x - neutron_logo_width) / 2, .y = 13}, (p2d_t){.x = neutron_logo_width, .y = neutron_logo_height},
                     GUI_IMAGE_FORMAT_XBM, neutron_logo_bits, COLOR32(0, 0, 0, 0), COLOR32(255, 0, 0, 0));
    //Add the name label to it
    char* name_label_text = "Neutron Project. 2019, Andrey Antonenko";
    uint32_t name_label_width = gfx_text_bounds(name_label_text).x;
    gui_create_label(window, (p2d_t){.x = (system_win_size.x - name_label_width) / 2, .y = 13 + neutron_logo_height + 2}, 
                             (p2d_t){.x = name_label_width, .y = 8}, name_label_text, COLOR32(255, 0, 0, 0), COLOR32(0, 0, 0, 0));
    //Add the version label to it
    char* ver_label_text = QUARK_VERSION_STR;
    uint32_t ver_label_width = gfx_text_bounds(ver_label_text).x;
    gui_create_label(window, (p2d_t){.x = (system_win_size.x - ver_label_width) / 2, .y = 13 + neutron_logo_height + 2 + 8 + 2}, 
                             (p2d_t){.x = ver_label_width, .y = 8}, ver_label_text, COLOR32(255, 0, 0, 0), COLOR32(0, 0, 0, 0));
    //Add the RAM label to it
    char ram_label_text[50] = "RAM: ";
    char temp[10];
    strcat(ram_label_text, sprintu(temp, stdlib_usable_ram() / 1024 / 1024, 1));
    strcat(ram_label_text, "MB usable by Neutron");
    uint32_t ram_label_width = gfx_text_bounds(ram_label_text).x;
    gui_create_label(window, (p2d_t){.x = (system_win_size.x - ram_label_width) / 2, .y = 13 + neutron_logo_height + 22}, 
                             (p2d_t){.x = ram_label_width, .y = 8}, ram_label_text, COLOR32(255, 0, 0, 0), COLOR32(0, 0, 0, 0));
    //Add the system color change button to it
    gui_create_button(window, (p2d_t){.x = 2, .y = 13 + neutron_logo_height + 32}, (p2d_t){.x = system_win_size.x - 2 - 4, .y = 15}, "Change system color",
                      quark_open_sys_color_picker, COLOR32(255, 255, 255, 255), COLOR32(0, 0, 0, 0), COLOR32(0, 0, 0, 0), COLOR32(0, 0, 0, 0));
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