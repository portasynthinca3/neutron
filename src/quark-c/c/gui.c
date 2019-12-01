//Neutron Project
//Graphical User interface
//Built on top of the GFX library (src/quark-c/c/gfx.c)

#include "../h/gui.h"
#include "../h/gfx.h"
#include "../h/stdlib.h"

//Mouse position on the screen
signed short mx, my;
//Keyboard buffer
unsigned char* kbd_buffer;
unsigned short kbd_buffer_head, kbd_buffer_tail;
//Mouse buffer
unsigned char* ms_buffer;
unsigned short ms_buffer_head, ms_buffer_tail;
//Current color scheme
color_scheme_t color_scheme;

/*
 * Performs some GUI initialization
 */
void gui_init(void){
    //Initialize the PS/2 controller
    gui_init_ps2();
    //Reset cursor coordinates
    mx = 0;
    my = 0;
    //Set the default color scheme
    color_scheme.desktop = 0x12; //Very dark grey
    color_scheme.top_bar = 0x2C; //Yellow
    color_scheme.cursor = 0x20; //Deep blue
    color_scheme.selection = 0x35; //Light blue
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
    //Draw the cursor
    gui_draw_cursor(mx, my);
    //Flip the buffers
    gfx_flip();
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