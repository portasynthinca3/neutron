//Neutron Project
//Control processing and rendering

#include "./controls.h"
#include "./windows.h"

/*
 * Creates a control and adds it to the window
 */
control_t* gui_create_control(window_t* win, uint32_t type, void* ext_ptr, p2d_t pos, p2d_t size, void(*event_handler)(ui_event_args_t*)){
    //Allocate memory for the control
    control_t cont;
    //Set its parameters
    cont.type = type;
    cont.extended = ext_ptr;
    cont.position = pos;
    cont.size = size;
    cont.event_handler = event_handler;
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
control_t* gui_create_label(window_t* win, p2d_t pos, p2d_t size, char* text, color32_t text_color, color32_t bg_color, void(*event_handler)(ui_event_args_t*)){
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
    return gui_create_control(win, GUI_WIN_CTRL_LABEL, (void*)label, pos, size, event_handler);
}

/*
 * Creates a button and adds it to the window
 */
control_t* gui_create_button(window_t* win, p2d_t pos, p2d_t size, char* text, color32_t text_color, color32_t bg_color,
                             color32_t pressed_bg_color, color32_t border_color, void(*event_handler)(ui_event_args_t*)){
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
    button->pressed_bg_color = pressed_bg_color;
    button->pressed_last_frame = 0;
    //Create a normal control with this extension
    return gui_create_control(win, GUI_WIN_CTRL_BUTTON, (void*)button, pos, size, event_handler);
}

/*
 * Creates a progress bar and adds it to the window
 */
control_t* gui_create_progress_bar(window_t* win, p2d_t pos, p2d_t size, color32_t bg_color, color32_t fill_color,
                                   color32_t border_color, uint32_t max_val, uint32_t val, void(*event_handler)(ui_event_args_t*)){
    //Create the "extended control" of progress bar type
    control_ext_progress_t* progress = (control_ext_progress_t*)malloc(sizeof(control_ext_progress_t));
    //Assign the parameters
    progress->bg_color = bg_color;
    progress->border_color = border_color;
    progress->fill_color = fill_color;
    progress->max_val = max_val;
    progress->val = val;
    //Create a normal control with this extension
    return gui_create_control(win, GUI_WIN_CTRL_PROGRESS_BAR, (void*)progress, pos, size, event_handler);
}

/*
 * Creates a progress bar and adds it to the window
 */
control_t* gui_create_image(window_t* win, p2d_t pos, p2d_t size, uint32_t format, void* data, color32_t color_lo, color32_t color_hi, void(*event_handler)(ui_event_args_t*)){
    //Create the "extended control" of image type
    control_ext_image_t* image = (control_ext_image_t*)malloc(sizeof(control_ext_image_t));
    //Assign properties
    image->image_format = format;
    image->image = data;
    image->color_lo = color_lo;
    image->color_hi = color_hi;
    //Create a normal control with this extension
    return gui_create_control(win, GUI_WIN_CTRL_IMAGE, (void*)image, pos, size, event_handler);
}

/*
 * Creates a track bar and adds it to the window
 */
control_t* gui_create_track_bar(window_t* win, p2d_t pos, p2d_t size, color32_t bg_color, color32_t fill_color,
                                color32_t border_color, uint32_t max_val, uint32_t val, void(*event_handler)(ui_event_args_t*)){
    //Create the "extended control" of progress bar type
    control_ext_track_bar_t* track = (control_ext_track_bar_t*)malloc(sizeof(control_ext_track_bar_t));
    //Assign the parameters
    track->bg_color = bg_color;
    track->border_color = border_color;
    track->fill_color = fill_color;
    track->max_val = max_val;
    track->val = val;
    //Create a normal control with this extension
    return gui_create_control(win, GUI_WIN_CTRL_TRACK_BAR, (void*)track, pos, size, event_handler);
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
            //if(button->bg_color.a == 0)
                button->bg_color = COLOR32(255, gui_get_color_scheme()->win_border.r, gui_get_color_scheme()->win_border.g,
                    gui_get_color_scheme()->win_border.b);
            //if(button->border_color.a == 0)
                button->border_color = COLOR32(255, button->bg_color.r, button->bg_color.g, button->bg_color.b);
            //if(button->pressed_bg_color.a == 0)
                button->pressed_bg_color = COLOR32(255, button->bg_color.r >> 1, button->bg_color.g >> 1, button->bg_color.b >> 1);
            //Draw the rectangles
            color32_t bg_color = button->pressed_last_frame ? button->pressed_bg_color : button->bg_color;
            if(gui_mouse_btns().x && gfx_point_in_rect(gui_mouse_coords(), (p2d_t){.x = ptr->position.x + win_ptr->position.x + 1,
                .y = ptr->position.y + win_ptr->position.y + 12}, ptr->size))
                    bg_color = gfx_blend_colors(button->pressed_bg_color, button->bg_color, 128);
            gfx_draw_filled_rect((p2d_t){.x = ptr->position.x + win_ptr->position.x + 1,
                                         .y = ptr->position.y + win_ptr->position.y + 12},
                                 ptr->size, bg_color);
            gfx_draw_rect((p2d_t){.x = ptr->position.x + win_ptr->position.x + 1,
                                  .y = ptr->position.y + win_ptr->position.y + 12},
                          ptr->size, button->border_color);
            //Calculate text bounds
            p2d_t t_bounds = gfx_text_bounds(button->text);
            //Draw the text
            gfx_puts((p2d_t){.x = ptr->position.x + win_ptr->position.x + 3  + ((ptr->size.x - t_bounds.x) / 2),
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
                pressed = gfx_point_in_rect(gui_mouse_coords(), 
                                            (p2d_t){.x = ptr->position.x + win_ptr->position.x + 1,
                                                    .y = ptr->position.y + win_ptr->position.y + 12},
                                            ptr->size) && gui_mouse_btns().x;
            //Check if the button is clicked (pressed for the first frame)
            uint8_t clicked = 0;
            if(pressed && !button->pressed_last_frame)
                clicked = 1;
            button->pressed_last_frame = pressed;
            //Call the event handler in case of a click
            if(clicked && ptr->event_handler != NULL){
                ui_event_args_t event;
                event.control = ptr;
                event.win = win_ptr;
                event.type = GUI_EVENT_CLICK;
                event.mouse_pos = gui_mouse_coords();
                ptr->event_handler(&event);
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
                if(gui_mouse_btns().x && gfx_point_in_rect(gui_mouse_coords(),
                                           (p2d_t){.x = ptr->position.x + win_ptr->position.x + 1,
                                                   .y = ptr->position.y + win_ptr->position.y + 12},
                                           ptr->size)){
                    //Calculate the new value
                    uint32_t new_val = (gui_mouse_coords().x - (ptr->position.x + win_ptr->position.x + 1)) * track->max_val / ptr->size.x;
                    if(new_val != track->val && ptr->event_handler != NULL){
                        //If the value has changed, assign it and call the callback function
                        track->val = new_val;
                        ui_event_args_t args;
                        args.control = ptr;
                        args.mouse_pos = gui_mouse_coords();
                        args.type = GUI_EVENT_TRACK_BAR_CHANGE;
                        args.win = win_ptr;
                        ptr->event_handler(&args);
                    }
                }
            }
        }
        break;
    }
}