//Neutron Project
//Graphical User interface
//Built on top of the GFX library (src/quark-c/c/gfx.c)

#include "./gui.h"
#include "./drivers/gfx.h"
#include "./stdlib.h"
#include "./drivers/diskio.h"

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
window_t* windows;
//The window that is being dragged currently
window_t* window_dragging;
//The point of the dragging window that is pinned to the cursor
p2d_t window_dragging_cpos;
///The window that is in focus
window_t* window_focused;
//Flag indicating that focusing was already processed this frame
uint8_t focus_processed;
//Window position in the the top bar
uint16_t topb_win_pos;
//The current time as a string
char time[64] = "??:??:??\0";

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

    //Allocate a chunk of memory for windows
    windows = (window_t*)malloc(64 * sizeof(window_t));
    //Set up an example window
    windows[0].title = (char*)malloc(sizeof(char) * 100);
    memcpy(windows[0].title, "This text was used to build this:", 34);
    windows[0].position = (p2d_t){.x = 100, .y = 100};
    windows[0].size_real = (p2d_t){.x = 300, .y = 300};
    windows[0].flags = GUI_WIN_FLAG_CLOSABLE | GUI_WIN_FLAG_DRAGGABLE | GUI_WIN_FLAG_MAXIMIZABLE |
                       GUI_WIN_FLAG_MINIMIZABLE | GUI_WIN_FLAG_TITLE_VISIBLE | GUI_WIN_FLAG_VISIBLE;
    //Create a simple list of controls
    control_t* controls = (control_t*)malloc(sizeof(control_t) * 2);
    controls[0].position = (p2d_t){.x = 2, .y = 2};
    controls[0].size = (p2d_t){.x = 1, .y = 1};
    controls[0].type = GUI_WIN_CTRL_LABEL;
    controls[0].extended_size = sizeof(control_ext_label_t);
    control_ext_label_t* label = (control_ext_label_t*)malloc(sizeof(control_ext_label_t));
    label->alignment = ALIGN_V_TOP | ALIGN_H_LEFT;
    label->bg_color = COLOR32(0, 0, 0, 0);
    label->text_color = COLOR32(255, 255, 255, 255); //White
    label->text = (char*)malloc(sizeof(char) * 512);
    label->text[0] = 0;
    //Read the build config file just for fun
    uint8_t fs_status = 0;
    if((fs_status = diskio_fs_read_file(0, "nbuild\x00", label->text)) != DISKIO_STATUS_OK){
        char temp[20];
        strcat(label->text, sprintu(temp, fs_status, 1));
    }
    label->text[344] = 0;
    controls[0].extended = (void*)label;
    //Mark the end of a control list
    controls[1].type = 0;
    //Set the controls
    windows[0].controls = controls;
    //Mark the end of a window list
    windows[1].size_real.x = 0;
    //Set the window in focus
    window_focused = &windows[0];
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
    int32_t i = 0;
    window_t* current_window;
    //Clear the focus processed flag
    focus_processed = 0;
    //Reset the top bar position
    topb_win_pos = 0;

    //If the window in focus is valid
    if(window_focused != NULL) //Process the window in focus first
        gui_process_window(window_focused);
    //Walk through the window list to determine the end of it, size_real.x=0 means it's the end of the list
    while(windows[i++].size_real.x != 0);
    i--;
    //Fetch the next window
    while(i >= 0){
        current_window = &windows[i--];
        //Process it
        gui_process_window(current_window);
    }

    //Reset the counter
    i = 0;
    uint16_t win_cnt = 0;
    //Fetch the next window, size_real.x=0 means it's the end of the list
    while((current_window = &windows[i++])->size_real.x != 0){
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
        //Print its title if window has the title visibility flag set
        if(ptr->flags & GUI_WIN_FLAG_TITLE_VISIBLE)
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
        control_t* control;
        uint32_t i = 0;
        //Control type = 0 marks the end of the list
        while((control = &ptr->controls[i++])->type)
            gui_render_control(ptr, control);
    }
}

/*
 * Renders a control
 */
void gui_render_control(window_t* win_ptr, control_t* ptr){
    //Check controls type
    switch(ptr->type){
        case GUI_WIN_CTRL_LABEL: {
            //Fetch the extended data
            control_ext_label_t* label = (control_ext_label_t*)ptr->extended;
            //Draw the label
            gfx_puts((p2d_t){.x = ptr->position.x + win_ptr->position.x + 1, .y = ptr->position.y + win_ptr->position.y + 12},
                     label->text_color, label->bg_color, label->text);
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