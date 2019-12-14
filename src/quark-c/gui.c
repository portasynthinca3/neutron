//Neutron Project
//Graphical User interface
//Built on top of the GFX library (src/quark-c/c/gfx.c)

#include "./gui.h"
#include "./stdlib.h"
#include "./drivers/gfx.h"
#include "./drivers/diskio.h"
#include "./drivers/pit.h"

//Mouse position on the screen
signed short mx, my;
//Mouse buttons state
uint8_t ml, mr;
//Keyboard buffer
unsigned char* kbd_buffer;
unsigned short kbd_buffer_head, kbd_buffer_tail;
//Mouse buffer
unsigned char* ms_buffer;
unsigned short ms_buffer_head, ms_buffer_tail;
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

void gui_example_button_callback(ui_event_args_t* args){
    control_ext_button_t* btn = (control_ext_button_t*)args->control->extended;
    if(btn->border_color.r == 64)
        btn->border_color = COLOR32(255, 255, 0, 0);
    else
        btn->border_color = COLOR32(255, 64, 64, 64);
}

/*
 * Performs some GUI initialization
 */
void gui_init(void){
    //Initialize the PS/2 controller
    gui_init_ps2();
    //Reset mouse state
    mx = my = ml = mr = 0;
    //Reset some variables
    window_dragging = NULL;
    //Set the default color scheme
    color_scheme.desktop =                  COLOR32(255, 40, 40, 40);           //Very dark grey
    color_scheme.top_bar =                  COLOR32(255, 70, 70, 70);           //Dark grey
    color_scheme.cursor =                   COLOR32(255, 255, 255, 255);        //White
    color_scheme.selection =                COLOR32(255, 0, 128, 255);          //Light blue
    color_scheme.time =                     COLOR32(255, 0, 128, 255);          //Light blue
    color_scheme.win_bg =                   COLOR32(255, 127, 127, 127);        //Grey
    color_scheme.win_border =               COLOR32(255, 0, 255, 128);          //Green-blue-ish
    color_scheme.win_title =                COLOR32(255, 255, 255, 255);        //White
    color_scheme.win_exit_btn =             COLOR32(255, 255, 0, 0);            //Red
    color_scheme.win_state_btn =            COLOR32(255, 255, 255, 0);          //Yellow
    color_scheme.win_minimize_btn =         COLOR32(255, 0, 255, 0);            //Lime
    color_scheme.win_unavailable_btn =      COLOR32(255, 40, 40, 40);           //Very dark grey
    
    //Allocate some space for the windows
    windows = (window_t*)malloc(32 * sizeof(window_t));
    windows[0].title = NULL;

    //Set up an example window
    window_t* ex_win = gui_create_window("Hello, World", GUI_WIN_FLAGS_STANDARD,
                                         (p2d_t){.x = 100, .y = 100}, (p2d_t){.x = 150, .y = 150});
    window_focused = ex_win;
    //Create an example control
    gui_create_label(ex_win, (p2d_t){.x = 1, .y = 1},
                             (p2d_t){.x = 100, .y = 100}, "Hello-o", COLOR32(255, 255, 255, 255),
                                                                     COLOR32(0, 0, 0, 0));
    gui_create_button(ex_win, (p2d_t){.x = 1, .y = 20},
                              (p2d_t){.x = 100, .y = 50}, "Click me", gui_example_button_callback,
                      COLOR32(255, 0, 0, 0),
                      COLOR32(255, 128, 128, 128),
                      COLOR32(255, 64, 64, 64),
                      COLOR32(255, 64, 64, 64));
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
    win.controls = (control_t*)malloc(32 * sizeof(control_t));
    win.controls[0].type = 0;
    //Scan through the window list to determine its end
    window_t* last;
    uint32_t i = 0;
    while((last = &windows[i++])->title);
    i--;
    //Assign the window
    windows[i] = win;
    //Mark the end of the list
    windows[i + 1].title = NULL;
    //Return the window
    return &windows[i];
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
 * Initializes the PS/2 controller
 */
void gui_init_ps2(){
    //Allocate the keyboard buffer
    kbd_buffer = (unsigned char*)malloc(GUI_KEYBOARD_BUFFER_SIZE);
    //Allocate the mouse buffer
    ms_buffer = (unsigned char*)malloc(GUI_MOUSE_BUFFER_SIZE);
    //Reset FIFO head and tail pointers
    kbd_buffer_head = kbd_buffer_tail = 0;
    ms_buffer_head = ms_buffer_tail = 0;
    while(inb(0x64) & 2); //Wait for input acceptability
    outb(0x64, 0xAE); //Issue command 0xAE (enable first PS/2 port)
    while(inb(0x64) & 2); //Wait for input acceptability
    outb(0x64, 0xA8); //Issue command 0xA8 (enable second PS/2 port)
    while(inb(0x64) & 2); //Wait for input acceptability
    outb(0x64, 0xD4); //Issue command 0xD4 (write to second PS/2 port)
    while(inb(0x64) & 2); //Wait for input acceptability
    outb(0x60, 0xF4); //Issue mouse command 0xF4 (enable packet streaming)
    while(!(inb(0x64) & 1)); //Wait for the mouse to send an ACK byte
    inb(0x60); //Read and discard the ACK byte
}

/*
 * Redraw the GUI
 */
void gui_update(void){

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

    //Render the windows
    gui_render_windows();
    //Draw the cursor
    gui_draw_cursor(mx, my);
    //Flip the buffers
    gfx_flip();
}

/*
 * Calls gui_render_window() according to the window order
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
    if(window_focused != NULL) //Render the window in focus first
        gui_render_window(window_focused);
    
    //Set the window in focus according to the top bar clicks
    if(ml && mx / 16 <= win_cnt){
        window_focused = &windows[mx / 16];
    }
}

/*
 * Renders a window
 */
void gui_render_window(window_t* ptr){
    //Only render the window if it has the visibility flag set
    if(ptr->flags & GUI_WIN_FLAG_VISIBLE){
        //Fill a rectangle with a window background color
        gfx_draw_filled_rect((p2d_t){.x = ptr->position.x, .y = ptr->position.y},
                             (p2d_t){.x = ptr->size.x, .y = ptr->size.y}, color_scheme.win_bg);
        //Draw a border around it
        gfx_draw_rect((p2d_t){.x = ptr->position.x, .y = ptr->position.y},
                      (p2d_t){.x = ptr->size.x, .y = ptr->size.y}, color_scheme.win_border);
        //Print its title
        gfx_puts((p2d_t){.x = ptr->position.x + 2, .y = ptr->position.y + 2}, color_scheme.win_title, COLOR32(0, 0, 0, 0), ptr->title);
        //Draw a border arount the title
        gfx_draw_rect((p2d_t){.x = ptr->position.x, .y = ptr->position.y}, 
                      (p2d_t){.x = ptr->size.x, .y = 11}, color_scheme.win_border);

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
        uint8_t process_ptr = gfx_point_in_rect((p2d_t){.x = mx, .y = my}, ptr->position, ptr->size);
        uint32_t i = 0;
        control_t* control;
        while((control = &ptr->controls[i++])->type)
            gui_render_control(ptr, control, process_ptr);
    }
}

/*
 * Renders a control
 */
void gui_render_control(window_t* win_ptr, control_t* ptr, uint8_t handle_pointer){
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
            //Draw the rectangles
            gfx_draw_filled_rect((p2d_t){.x = ptr->position.x + win_ptr->position.x + 1,
                                         .y = ptr->position.y + win_ptr->position.y + 12},
                                 ptr->size, pressed ? button->pressed_bg_color : button->bg_color);
            gfx_draw_rect((p2d_t){.x = ptr->position.x + win_ptr->position.x + 1,
                                  .y = ptr->position.y + win_ptr->position.y + 12},
                          ptr->size, button->border_color);
            //Calculate text bounds
            p2d_t t_bounds = gfx_text_bounds(button->text);
            //Draw the text
            gfx_puts((p2d_t){.x = ptr->position.x + win_ptr->position.x + 1  + ((ptr->size.y - t_bounds.y) / 2),
                             .y = ptr->position.y + win_ptr->position.y + 12 + ((ptr->size.y - t_bounds.y) / 2)},
                     button->text_color, COLOR32(0, 0, 0, 0), button->text);
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
        //  and the left button is held down, set the window in focus and set the "focus processed" flag
        if(ml &&
           !focus_processed &&
           gfx_point_in_rect((p2d_t){.x = mx, .y = my},
                             (p2d_t){.x = ptr->position.x + 1, .y = ptr->position.y + 1},
                             (p2d_t){.x = ptr->size.x - 2, .y = ptr->size.y - 2})){
            focus_processed = 1;
            window_focused = ptr;
        }
    }
}

/*
 * Polls the PS/2 controller
 */
void gui_poll_ps2(){
    //Variable holding the I/O port 64h data
    unsigned char p64d;
    //While data is available for reading
    while((p64d = inb(0x64)) & 1){
        //If bit 5 is set, it's a mouse data byte
        if(p64d & 0x20)
            fifo_pushb(ms_buffer, &ms_buffer_head, inb(0x60));
        else //Else, a keyboard one
            fifo_pushb(kbd_buffer, &kbd_buffer_head, inb(0x60));
    }
    //If at least three bytes are available for reading in the mouse buffer
    if(fifo_av(&ms_buffer_head, &ms_buffer_tail) >= 3){
        //Read the packet
        unsigned char ms_flags = fifo_popb(ms_buffer, &ms_buffer_head, &ms_buffer_tail);
        unsigned char ms_x = fifo_popb(ms_buffer, &ms_buffer_head, &ms_buffer_tail);
        unsigned char ms_y = fifo_popb(ms_buffer, &ms_buffer_head, &ms_buffer_tail);
        //Bit 3 of flags should always be set
        if(ms_flags & 8){
            //Process it
            if(ms_flags & 0x20) //Bit 5 in flags means delta Y is negative
                my -= (signed short)((signed short)ms_y) | 0xFF00; //We subtract because PS/2 assumes that the Y axis is looking up, but it's the opposite in graphics
            else
                my -= (signed short)ms_y;
            if(ms_flags & 0x10) //Bit 4 in flags means delta X is negative
                mx += (signed short)((signed short)ms_x) | 0xFF00;
            else
                mx += (signed short)ms_x;
        }
        //Constrain the coordinates
        if(mx < 0)
            mx = 0;
        else if(mx >= gfx_res_x())
            mx = gfx_res_x() - 1;
        if(my < 0)
            my = 0;
        else if(my >= gfx_res_y())
            my = gfx_res_y() - 1;
        //Set mouse button state variables
        ml = ms_flags & 1;
        mr = ms_flags & 2;
    }
}

/*
 * Draws the cursor on screen
 */
void gui_draw_cursor(uint16_t x, uint16_t y){
    //Retrieve the graphics buffer; draw directly on it
    color24_t* buf = gfx_buffer();
    //Retrieve the X resolution
    uint16_t res_x = gfx_res_x();
    //Draw!
    buf[(y * res_x) + x] = COLOR24(color_scheme.cursor);
    buf[(y * res_x) + x + 1] = COLOR24(color_scheme.cursor);
    buf[(y * res_x) + x + 2] = COLOR24(color_scheme.cursor);
    buf[((y + 1) * res_x) + x] = COLOR24(color_scheme.cursor);
    buf[((y + 2) * res_x) + x] = COLOR24(color_scheme.cursor);
    buf[((y + 1) * res_x) + x + 1] = COLOR24(color_scheme.cursor);
    buf[((y + 2) * res_x) + x + 2] = COLOR24(color_scheme.cursor);
    buf[((y + 3) * res_x) + x + 3] = COLOR24(color_scheme.cursor);
    buf[((y + 4) * res_x) + x + 4] = COLOR24(color_scheme.cursor);
}