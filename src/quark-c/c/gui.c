//Neutron Project
//Graphical User interface
//Built on top of the GFX library (src/quark-c/c/gfx.c)

#include "../h/gui.h"
#include "../h/gfx.h"
#include "../h/stdlib.h"

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
    //The palette for reference is here: https://external-content.duckduckgo.com/iu/?u=http%3A%2F%2Fwww.fountainware.com%2FEXPL%2Fvgapalette.png&f=1&nofb=1
    color_scheme.desktop = 0x12; //Very dark grey
    color_scheme.top_bar = 0x14; //Dark grey
    color_scheme.cursor = 0x0F; //White
    color_scheme.selection = 0x35; //Light blue
    color_scheme.win_bg = 0x17; //Grey
    color_scheme.win_border = 0x32; //Green-blue-ish
    color_scheme.win_title = 0x0F; //White
    color_scheme.win_exit_btn = 0x28; //Red
    color_scheme.win_state_btn = 0x2C; //Yellow
    color_scheme.win_minimize_btn = 0x2F; //Lime
    color_scheme.win_unavailable_btn = 0x12; //Very dark grey

    //Allocate a chunk of memory for windows
    windows = (window_t*)malloc(64 * sizeof(window_t));
    //Set up an example window
    windows[0].title = "Window #0";
    windows[0].position = (p2d_t){.x = 100, .y = 100};
    windows[0].size = (p2d_t){.x = 150, .y = 100};
    windows[0].flags = GUI_WIN_FLAG_CLOSABLE | GUI_WIN_FLAG_DRAGGABLE | GUI_WIN_FLAG_MAXIMIZABLE |
                       GUI_WIN_FLAG_MINIMIZABLE | GUI_WIN_FLAG_TITLE_VISIBLE | GUI_WIN_FLAG_VISIBLE;
    windows[0].controls = NULL;
    //And another one...
    windows[1].title = "Window #1";
    windows[1].position = (p2d_t){.x = 120, .y = 120};
    windows[1].size = (p2d_t){.x = 150, .y = 100};
    windows[1].flags = GUI_WIN_FLAG_CLOSABLE | GUI_WIN_FLAG_DRAGGABLE | GUI_WIN_FLAG_MAXIMIZABLE |
                       GUI_WIN_FLAG_MINIMIZABLE | GUI_WIN_FLAG_TITLE_VISIBLE | GUI_WIN_FLAG_VISIBLE;
    windows[1].controls = NULL;
    //The third one...
    windows[2].title = "Window #2";
    windows[2].position = (p2d_t){.x = 140, .y = 140};
    windows[2].size = (p2d_t){.x = 150, .y = 100};
    windows[2].flags = GUI_WIN_FLAG_CLOSABLE | GUI_WIN_FLAG_DRAGGABLE | GUI_WIN_FLAG_MAXIMIZABLE |
                       GUI_WIN_FLAG_MINIMIZABLE | GUI_WIN_FLAG_TITLE_VISIBLE | GUI_WIN_FLAG_VISIBLE;
    windows[2].controls = NULL;
    //Mark the end of a window list
    windows[3].size.x = 0;
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
    gfx_draw_filled_rect(0, 0, gfx_res_x(), 16, color_scheme.top_bar);
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
    uint32_t i = 0;
    window_t* current_window;
    //Clear the focus processed flag
    focus_processed = 0;

    //If the window in focus is valid
    if(window_focused != NULL) //Process the window in focus first
        gui_process_window(window_focused);
    //Fetch the next window, size.x=0 means it's the end of the list
    while((current_window = &windows[i++])->size.x != 0){
        //Process the current window
        gui_process_window(current_window);
    }

    //Reset the counter
    i = 0;
    //Fetch the next window, size.x=0 means it's the end of the list
    while((current_window = &windows[i++])->size.x != 0){
        //Render the current window if it isn't in focus
        if(current_window != window_focused)
            gui_render_window(current_window);
    }
    //If the window in focus is valid
    if(window_focused != NULL) //Render the window in focus first
        gui_render_window(window_focused);
}

/*
 * Renders a window
 */
void gui_render_window(window_t* ptr){
    //Only draw the window if it has the visibility flag set
    if(ptr->flags & GUI_WIN_FLAG_VISIBLE){
        //Fill a rectangle with a window background color
        gfx_draw_filled_rect(ptr->position.x, ptr->position.y, ptr->size.x, ptr->size.y, color_scheme.win_bg);
        //Draw a border around it
        gfx_draw_rect(ptr->position.x, ptr->position.y, ptr->size.x, ptr->size.y, color_scheme.win_border);
        //Print its title if window has the title visibility flag set
        if(ptr->flags & GUI_WIN_FLAG_TITLE_VISIBLE)
            gfx_puts(ptr->position.x + 2, ptr->position.y + 2, color_scheme.win_title, ptr->title);
        //Draw a border arount the title
        gfx_draw_rect(ptr->position.x, ptr->position.y, ptr->size.x, 11, color_scheme.win_border);
        //Draw the close button 
        if(ptr->flags & GUI_WIN_FLAG_CLOSABLE)
            gfx_draw_filled_rect(ptr->position.x + ptr->size.x - 10, ptr->position.y + 2, 8, 8, color_scheme.win_exit_btn);
        else
            gfx_draw_filled_rect(ptr->position.x + ptr->size.x - 10, ptr->position.y + 2, 8, 8, color_scheme.win_unavailable_btn);
        //Draw the maximize (state change) button 
        if(ptr->flags & GUI_WIN_FLAG_MAXIMIZABLE)
            gfx_draw_filled_rect(ptr->position.x + ptr->size.x - 19, ptr->position.y + 2, 8, 8, color_scheme.win_state_btn);
        else
            gfx_draw_filled_rect(ptr->position.x + ptr->size.x - 10, ptr->position.y + 2, 8, 8, color_scheme.win_unavailable_btn);
        //Draw the minimize button 
        if(ptr->flags & GUI_WIN_FLAG_MAXIMIZABLE)
            gfx_draw_filled_rect(ptr->position.x + ptr->size.x - 28, ptr->position.y + 2, 8, 8, color_scheme.win_minimize_btn);
        else
            gfx_draw_filled_rect(ptr->position.x + ptr->size.x - 10, ptr->position.y + 2, 8, 8, color_scheme.win_unavailable_btn);
    }
}

/*
 * Process window's interaction with the cursor
 */
void gui_process_window(window_t* ptr){
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
            //If the left mouse button is still being pressed, drag the window
            if(ml)
                ptr->position = (p2d_t){.x = mx + window_dragging_cpos.x, .y = my + window_dragging_cpos.y};
            else //Else, reset the pointer
                window_dragging = NULL;
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
void gui_draw_cursor(unsigned short x, unsigned short y){
    //Retrieve the graphics buffer; draw directly on it
    unsigned char* buf = gfx_buffer();
    //Retrieve the X resolution
    unsigned short res_x = gfx_res_x();
    //Draw!
    buf[(y * res_x) + x] = color_scheme.cursor;
    buf[(y * res_x) + x + 1] = color_scheme.cursor;
    buf[(y * res_x) + x + 2] = color_scheme.cursor;
    buf[((y + 1) * res_x) + x] = color_scheme.cursor;
    buf[((y + 2) * res_x) + x] = color_scheme.cursor;
    buf[((y + 1) * res_x) + x + 1] = color_scheme.cursor;
    buf[((y + 2) * res_x) + x + 2] = color_scheme.cursor;
    buf[((y + 3) * res_x) + x + 3] = color_scheme.cursor;
    buf[((y + 4) * res_x) + x + 4] = color_scheme.cursor;
}