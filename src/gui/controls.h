#ifndef CONTROLS_H
#define CONTROLS_H

#include "../stdlib.h"
#include "./gui.h"
#include "./windows.h"

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
                                
void gui_render_control(window_t* win_ptr, control_t* ptr);
void gui_process_control(window_t* win_ptr, control_t* ptr, uint8_t handle_pointer);

#endif