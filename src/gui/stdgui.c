//Neutron project
//Standard GUI windows

#include "./stdgui.h"
#include "../stdlib.h"
#include "../drivers/gfx.h"
#include "./gui.h"
#include "./windows.h"
#include "./controls.h"
#include "../mtask/mtask.h"

#include "../images/neutron_logo.h"
#include "../images/task_mgr.h"

//These are defined in the Kernel
void krnl_shutdown(void);
void krnl_reboot(void);
void krnl_open_sys_color_picker(ui_event_args_t* args);

//The list of buttons for launching respective apps
control_t* app_buttons[32];

//The position the next app will be loaded into

void _stdgui_cb_shutdown_pressed(ui_event_args_t* args){
    if(args->type == GUI_EVENT_CLICK)
        krnl_shutdown();
}
void _stdgui_cb_reboot_pressed(ui_event_args_t* args){
    if(args->type == GUI_EVENT_CLICK)
        krnl_reboot();
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
            cpick_img_buf[pos++] = 255;
        }
    }
}
void _stdgui_cb_color_r(ui_event_args_t* args){
    if(args->type == GUI_EVENT_TRACK_BAR_CHANGE){
        cpick_color.r = ((control_ext_track_bar_t*)((control_t*)(args->control))->extended)->val;
        _stdgui_fill_img();
    }
}
void _stdgui_cb_color_g(ui_event_args_t* args){
    if(args->type == GUI_EVENT_TRACK_BAR_CHANGE){
        cpick_color.g = ((control_ext_track_bar_t*)((control_t*)(args->control))->extended)->val;
        _stdgui_fill_img();
    }
}
void _stdgui_cb_color_b(ui_event_args_t* args){
    if(args->type == GUI_EVENT_TRACK_BAR_CHANGE){
        cpick_color.b = ((control_ext_track_bar_t*)((control_t*)(args->control))->extended)->val;
        _stdgui_fill_img();
    }
}

/*
 * Creates a shutdown prompt window
 */
void stdgui_create_shutdown_prompt(void){
    p2d_t shutdown_win_size = (p2d_t){.x = 300, .y = 150};
    //Create a window
    window_t* shutdown_window = gui_create_window("Shutdown", NULL, GUI_WIN_FLAGS_STANDARD,
                                                  (p2d_t){.x = (gfx_res_x() - shutdown_win_size.x) / 2,
                                                          .y = (gfx_res_y() - shutdown_win_size.y) / 2}, shutdown_win_size, NULL);
    //Create a label and two buttons
    char* msg = "What do you want to do?";
    p2d_t msg_bounds = gfx_text_bounds(msg);
    gui_create_label(shutdown_window, (p2d_t){.x = (shutdown_win_size.x - msg_bounds.x) / 2,
                                              .y = (shutdown_win_size.y - msg_bounds.y - 25) / 2}, msg_bounds, msg,
        COLOR32(255, 255, 255, 255), COLOR32(0, 0, 0, 0), NULL);
    gui_create_button(shutdown_window, (p2d_t){.x = 0, .y = shutdown_win_size.y - 25 - 13}, (p2d_t){.x = shutdown_win_size.x / 2, .y = 25}, "Shutdown",
                      COLOR32(255, 255, 255, 255), COLOR32(0, 0, 0, 0), COLOR32(0, 0, 0, 0), COLOR32(0, 0, 0, 0), _stdgui_cb_shutdown_pressed);
    gui_create_button(shutdown_window, (p2d_t){.x = shutdown_win_size.x / 2 - 2, .y = shutdown_win_size.y - 25 - 13}, (p2d_t){.x = shutdown_win_size.x / 2, .y = 25}, "Reboot",
                      COLOR32(255, 255, 255, 255), COLOR32(0, 0, 0, 0), COLOR32(0, 0, 0, 0), COLOR32(0, 0, 0, 0), _stdgui_cb_reboot_pressed);
}

void _stdgui_task_mgr_btn_click(ui_event_args_t* args){
    if(args->type == GUI_EVENT_CLICK){
        stdgui_create_task_manager();
    }
}

/*
 * Creates a system control window
 */
void stdgui_create_system_win(void){
    p2d_t system_win_size = (p2d_t){.x = 300, .y = 300};
    //Create the window itself
    window_t* window = gui_create_window("System", NULL, GUI_WIN_FLAGS_STANDARD,
                                         (p2d_t){.x = (gfx_res_x() - system_win_size.x) / 2,
                                                 .y = (gfx_res_y() - system_win_size.y) / 2}, system_win_size, NULL);
    //Add the Neutron logo to it
    gui_create_image(window,
                     (p2d_t){.x = (system_win_size.x - neutron_logo_width) / 2, .y = 8}, (p2d_t){.x = neutron_logo_width, .y = neutron_logo_height},
                     GUI_IMAGE_FORMAT_RAW, neutron_logo, COLOR32(0, 0, 0, 0), COLOR32(255, 255, 255, 255), NULL);
    //Add the name label to it
    char* name_label_text = "Neutron Project. 2019-2020, Andrey Antonenko";
    uint32_t name_label_width = gfx_text_bounds(name_label_text).x;
    gui_create_label(window, (p2d_t){.x = (system_win_size.x - name_label_width) / 2, .y = 13 + neutron_logo_height + 7}, 
                             (p2d_t){.x = name_label_width, .y = 8}, name_label_text, COLOR32(255, 255, 255, 255), COLOR32(0, 0, 0, 0), NULL);
    //Add the version label to it
    char* ver_label_text = KRNL_VERSION_STR;
    uint32_t ver_label_width = gfx_text_bounds(ver_label_text).x;
    gui_create_label(window, (p2d_t){.x = (system_win_size.x - ver_label_width) / 2, .y = 13 + neutron_logo_height + 7 + 8 + 2}, 
                             (p2d_t){.x = ver_label_width, .y = 8}, ver_label_text, COLOR32(255, 255, 255, 255), COLOR32(0, 0, 0, 0), NULL);
    //Add the RAM usage indicator to it
    gui_create_progress_bar(window, (p2d_t){.x = 2, .y = 13 + neutron_logo_height + 27}, (p2d_t){.x = system_win_size.x - 2 - 4, .y = 15},
                            gui_get_color_scheme()->win_bg, COLOR32(255, 255, 0, 0), COLOR32(255, 255, 255, 255), stdlib_usable_ram(), stdlib_used_ram(), NULL);
    //Add the RAM label to it
    char ram_label_text[50] = "RAM: ";
    char temp[10];
    strcat(ram_label_text, sprintu(temp, stdlib_used_ram() / 1024 / 1024, 1));
    strcat(ram_label_text, "/");
    strcat(ram_label_text, sprintu(temp, stdlib_usable_ram() / 1024 / 1024, 1));
    strcat(ram_label_text, " MB used");
    uint32_t ram_label_width = gfx_text_bounds(ram_label_text).x;
    gui_create_label(window, (p2d_t){.x = (system_win_size.x - ram_label_width) / 2, .y = 13 + neutron_logo_height + 31}, 
                             (p2d_t){.x = ram_label_width, .y = 8}, ram_label_text, COLOR32(255, 255, 255, 255), COLOR32(0, 0, 0, 0), NULL);
    //Add the system color change button to it
    gui_create_button(window, (p2d_t){.x = 2, .y = 13 + neutron_logo_height + 45}, (p2d_t){.x = system_win_size.x - 2 - 4, .y = 15}, "Change system color",
                      COLOR32(255, 255, 255, 255), COLOR32(0, 0, 0, 0), COLOR32(0, 0, 0, 0), COLOR32(0, 0, 0, 0), krnl_open_sys_color_picker);
    //Add the system task manager button to it
    gui_create_button(window, (p2d_t){.x = 2, .y = 13 + neutron_logo_height + 62}, (p2d_t){.x = system_win_size.x - 2 - 4, .y = 15}, "Task manager",
                      COLOR32(255, 255, 255, 255), COLOR32(0, 0, 0, 0), COLOR32(0, 0, 0, 0), COLOR32(0, 0, 0, 0), _stdgui_task_mgr_btn_click);
}

/*
 * Task manager window event handler
 */
void _stdgui_task_mgr_evt(ui_event_args_t* args){
    //If the window is being closed, stop the updater process
    if(args->type == GUI_EVENT_WIN_CLOSE)
        mtask_stop_task(((window_t*)args->win)->task_uid);
}

/*
 * The task that updates the task manager
 */
void _stdgui_task_mgr_updater(void* task_mgr_label){
    //Get the UID
    uint64_t uid = mtask_get_uid();
    //Temporary string
    char temp[1024];
    char temp2[128];
    while(1){
        //Construct the temporary string
        temp[0] = 0;
        //Scan through the task list
        task_t* tasks = mtask_get_task_list();
        for(uint32_t i = 0; i < MTASK_TASK_COUNT; i++){
            //If the task is valid
            if(tasks[i].valid){
                //Append its name to the string
                strcat(temp, tasks[i].name);
                //Append its kilocycle count to the string
                strcat(temp, " (prio: ");
                strcat(temp, sprintu(temp2, tasks[i].priority, 1));
                strcat(temp, ")");
                if(tasks[i].state_code == TASK_STATE_RUNNING)
                    strcat(temp, " [running]");
                else
                    strcat(temp, " [blocked]");
                strcat(temp, "\n");
            }
        }
        //Copy the temporary string
        memcpy(((control_ext_label_t*)task_mgr_label)->text, temp, 1024);
    }
}

/*
 * Creates a task manager window
 */
void stdgui_create_task_manager(void){
    p2d_t window_size = (p2d_t){.x = 300, .y = 300};
    //Create the window
    window_t* window = gui_create_window("Task manager", task_mgr_icon, GUI_WIN_FLAGS_STANDARD, (p2d_t){.x = (gfx_res_x() - window_size.x) / 2,
                                                                                                        .y = (gfx_res_y() - window_size.y) / 2},
        window_size, _stdgui_task_mgr_evt);
    //Create the label
    control_ext_label_t* task_mgr_label = gui_create_label(window, (p2d_t){.x = 0, .y = 0}, window_size, "", COLOR32(255, 255, 255, 255), COLOR32(0, 0, 0, 0), NULL)->extended;
        task_mgr_label->text = (char*)malloc(1024);
    //Create the update process
    window->task_uid = mtask_create_task(16384, "Task manager", 2, _stdgui_task_mgr_updater, (void*)task_mgr_label);
}

/*
 * Creates a color picker
 */
uint8_t stdgui_create_color_picker(void (*callback)(ui_event_args_t*), color32_t start){
    p2d_t win_size = (p2d_t){.x = 157, 80};
    cpick_color = start;
    //Create the window
    window_t* window = gui_create_window("Choose color", NULL, GUI_WIN_FLAGS_STANDARD,
                                         (p2d_t){.x = (gfx_res_x() - win_size.x) / 2,
                                                 .y = (gfx_res_y() - win_size.y) / 2}, win_size, NULL);
    //Create a buffer for the image
    cpick_img_buf = (uint8_t*)malloc(40 * 40 * 4);
    _stdgui_fill_img();
    //Create the image
    gui_create_image(window, (p2d_t){.x = 5, .y = 5}, (p2d_t){.x = 40, .y = 40}, GUI_IMAGE_FORMAT_RAW, cpick_img_buf, COLOR32(0, 0, 0, 0), COLOR32(0, 0, 0, 0), NULL);
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
    gui_create_button(window, (p2d_t){.x = 5, .y = 50}, (p2d_t){.x = 145, .y = 12}, "OK",
                      COLOR32(255, 255, 255, 255), COLOR32(0, 0, 0, 0), COLOR32(0, 0, 0, 0), COLOR32(0, 0, 0, 0), callback)->extended;
}

/*
 * Returns the color that was selected by a color picker
 */
color32_t stdgui_cpick_get_color(void){
    return cpick_color;
}