//Neutron Project
//Kernel graphics driver

#include "./gfx.h"
#include "../stdlib.h"

//The video buffer pointers
unsigned char* vbe_buffer;
unsigned char* sec_buffer;
//The resolution
unsigned short res_x;
unsigned short res_y;
//The font
const unsigned char* font;
//The buffer selected for operations
unsigned char buf_sel;
//Virtual text mode position
unsigned short vterm_y = 0;

/*
 * Retrieve the horizontal resolution
 */
unsigned short gfx_res_x(void){
    return res_x;
}

/*
 * Retrieve the vertical resolution
 */
unsigned short gfx_res_y(void){
    return res_y;
}

/*
 * Retrieve the currently used buffer pointer
 */
unsigned char* gfx_buffer(void){
    return (buf_sel == GFX_BUF_VBE) ? vbe_buffer : sec_buffer;
}

/*
 * Initialize the graphics driver
 */
void gfx_init(void){
    //Read the VBE buffer pointer (it was written in memory by the second stage loader)
    vbe_buffer = (unsigned char*)(*(int*)(0x8FC00));
    //Read the display resolution
    unsigned int res = *(unsigned int*)(0x8FC04);
    res_y = res & 0xFFFF;
    res_x = (res >> 16) & 0xFFFF;
    //Allocate the second buffer based on the screen size
    sec_buffer = (unsigned char*)malloc(res_x * res_y);
    //Clear the virtual text mode position
    vterm_y = 0;
}

/*
 * Transfer the data from the current buffer to the opposing buffer
 */
void gfx_flip(void){
    //Choose the source and destination buffers
    unsigned char* buf_src = (buf_sel == GFX_BUF_VBE) ? vbe_buffer : sec_buffer;
    unsigned char* buf_dst = (buf_sel == GFX_BUF_VBE) ? sec_buffer : vbe_buffer;
    //Calculate the number of doublewords to be transferred
    unsigned int size_dw = res_x * res_y / 4;
    //Use the very clever assembly REP MOVSD opcode to transfer a large amount of data quickly
    __asm__ volatile("mov %%eax, %%esi;"
                     "mov %%ebx, %%edi;"
                     "rep movsd;" : :
                     "c" (size_dw), "a" (buf_src), "b" (buf_dst));
}

/*
 * Set the font that will be used in future
 */
void gfx_set_font(const unsigned char* fnt){
    font = fnt;
}

/*
 * Set the video buffer that will be used in future
 */
void gfx_set_buf(unsigned char buf){
    //Abort if buffer is invalid
    if(buf != GFX_BUF_VBE && buf != GFX_BUF_SEC)
        abort();
    //Assign the buffer otherwise
    buf_sel = buf;
}

/*
 * Fill the screen with one color
 */
void gfx_fill(unsigned char color){
    //We can use memset for that
    memset((buf_sel == GFX_BUF_VBE) ? vbe_buffer : sec_buffer, (int)color, res_x * res_y);
}

/*
 * Draw a filled rectangle
 */
void gfx_draw_filled_rect(unsigned short sx, unsigned short sy, unsigned short w, unsigned short h, color8_t c){
    //For each horizontal line in the rectangle
    for(unsigned short y = sy; y < sy + h; y++){
        //Use memset to draw a horizontal line
        unsigned int offset = (y * res_x) + sx;
        memset(((buf_sel == GFX_BUF_VBE) ? vbe_buffer : sec_buffer) + offset, c, w);
    }
}

/*
 * Draw a rectangle
 */
void gfx_draw_rect(unsigned short sx, unsigned short sy, unsigned short w, unsigned short h, color8_t c){
    //Draw two horizontal lines
    unsigned int offset = (sy * res_x) + sx;
    memset(((buf_sel == GFX_BUF_VBE) ? vbe_buffer : sec_buffer) + offset, c, w);
    offset = ((sy + h) * res_x) + sx;
    memset(((buf_sel == GFX_BUF_VBE) ? vbe_buffer : sec_buffer) + offset, c, w);
    //Draw two vertical lines
    for(uint16_t y = sy; y <= sy + h; y++){
        offset = (y * res_x) + sx;
        *((buf_sel == GFX_BUF_VBE) ? vbe_buffer : sec_buffer + offset) = c;
    }
    for(uint16_t y = sy; y <= sy + h; y++){
        offset = (y * res_x) + sx + w;
        *((buf_sel == GFX_BUF_VBE) ? vbe_buffer : sec_buffer + offset) = c;
    }
}

/*
 * Draw a horizontal line
 */
void gfx_draw_hor_line(uint16_t sx, uint16_t sy, uint16_t w, color8_t c){
    //We can use memset for that
    unsigned int offset = (sy * res_x) + sx;
    memset(((buf_sel == GFX_BUF_VBE) ? vbe_buffer : sec_buffer) + offset, c, w);
}

/*
 * Draw checkerboard pattern
 */
void gfx_draw_checker(unsigned char c1, unsigned char c2){
    for(unsigned short y = 0; y < res_y; y++){
        //Choose the starting color
        unsigned char c = ((y % 2 == 1) ? c2 : c1);
        for (unsigned short x = 0; x < res_x; x++){
            //Draw a pixel
            ((buf_sel == GFX_BUF_VBE) ? vbe_buffer : sec_buffer)[(y * res_x) + x] = c;
            //Switch the color
            if(c == c1)
                c = c2;
            else
                c = c1;
        }
    }
}

/*
 * Draw an XBM image
 */
void gfx_draw_xbm(p2d_t position, uint8_t* xbm_ptr, p2d_t xbm_size, color8_t color_h, color8_t color_l){
    //Create some local variables
    uint8_t* ptr = xbm_ptr;
    p2d_t pos = position;
    color8_t* buffer = gfx_buffer();
    while(1){
        //Fetch a byte
        uint8_t data = *(ptr++);
        //Cycle through its bits
        for(uint16_t x = 0; x < 8; x++){
            //Check the position
            if(pos.x - position.x < xbm_size.x){
                //If it is in bounds, draw the pixel
                if((data >> x) & 1)
                    buffer[(pos.y * res_x) + pos.x] = color_h;
                else
                    buffer[(pos.y * res_x) + pos.x] = color_l;
            }
            //Increment the position
            pos.x++;
        }
        //If the X coordinate has reached the limit, reset it and increment the Y coordinate
        if(pos.x - position.x >= xbm_size.x){
            pos.x = position.x;
            pos.y++;
        }
        //If the Y coordinate has reached the limit, return
        if(pos.y - position.y >= xbm_size.y)
            return;
    }
}

/*
 * Put a char with backgrund color in video buffer
 */
void gfx_putch(p2d_t pos, color8_t color, color8_t bcolor, char c){
    //Get the video buffer
    color8_t* buf = gfx_buffer();
    //Calculate the video buffer offset
    unsigned int buf_offset = (pos.y * res_x) + pos.x - 6;
    //Calculate the font offset
    unsigned int font_offset = (c - 1) * 6;
    //For each column in the font
    for(unsigned char i = 0; i < 6; i++){
        //Load it
        unsigned char font_col = font[font_offset + i];
        //And for each pixel in that column
        for(unsigned char j = 0; j < 8; j++){
            if((font_col >> j) & 1)
                buf[buf_offset + i + (j * res_x)] = color; //Draw it
            else if(bcolor != COLOR_TRANSPARENT)
                buf[buf_offset + i + (j * res_x)] = bcolor; //Or clear it
        }
    }
}

/*
 * Put a string in video buffer
 */
void gfx_puts(p2d_t pos, color8_t color, color8_t bcolor, char* s){
    //Data byte, position, state and counter
    char c = 0;
    p2d_t pos_actual = pos;
    uint32_t state = 0;
    uint32_t i = 0;
    //Fetch the next character
    while((c = s[i++]) != 0){
        if(state == 0){ //Normal state
            //Process control characters
            switch(c){
                case '\n': //Carriage return
                    pos_actual.x = pos.x;
                    pos_actual.y += 8;
                    break;
                case 1: //Foreground color change
                    state = 1;
                    break;
                case 2: //Background color change
                    state = 2;
                    break;
                default: //Print the char and advance its position
                    pos_actual.x += 6;
                    gfx_putch(pos_actual, color, bcolor, c);
                    break;
            }
        } else if(state == 1){ //State: foreground color change
            color = c;
            state = 0;
        } else if(state == 2){ //State: background color change
            color = c;
            state = 0;
        }
    }
}

/*
 * Calculate the bounds of a string if it was rendered on screen
 */
p2d_t gfx_text_bounds(char* s){
    char c = 0;
    p2d_t sz = (p2d_t){.x = 0, .y = 0};
    p2d_t pos = sz;
    uint32_t state = 0;
    uint32_t i = 0;
    //Fetch the next character
    while((c = s[i++]) != 0){
        //Process control characters
        switch(c){
            case '\n': //Carriage return
                pos.x = 0;
                pos.y += 8;
                break;
            case 1: //Foreground color change
                i++; //Skip one char
                break;
            case 2: //Background color change
                i++; //Skip one char
                break;
            default: //Print the char and advance its position
                pos.x += 6;
                break;
        }
        //If the X position is greater than the X size, update the last one
        if(pos.x > sz.x)
            sz.x = pos.x;
        //The same thing with the Y position
        if(pos.y > sz.y)
            sz.y = pos.y;
    }
    //Return the size
    return sz;
}

/*
 * Put a string in virtual text mode
 */
void gfx_vterm_println(char* s, unsigned char color){
    //Normally put a string
    gfx_puts((p2d_t){.x = 0, .y = vterm_y}, color, 0, s);
    //Increment the position
    vterm_y += 8;
}

//4-bit value to its hexadecimal symbol conversion table
const char hex_conv_const[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

/*
 * Put a string representing a 32-bit hexadecimal value in virtual text mode
 */
void gfx_vterm_println_hex(int value, unsigned char color){
    //Convert the value into string
    char* hex = "  PLACEHLD";
    hex[2] = hex_conv_const[(value >> 28) & 0xF];
    hex[3] = hex_conv_const[(value >> 24) & 0xF];
    hex[4] = hex_conv_const[(value >> 20) & 0xF];
    hex[5] = hex_conv_const[(value >> 16) & 0xF];
    hex[6] = hex_conv_const[(value >> 12) & 0xF];
    hex[7] = hex_conv_const[(value >> 8) & 0xF];
    hex[8] = hex_conv_const[(value >> 4) & 0xF];
    hex[9] = hex_conv_const[value & 0xF];
    //Normally print the string
    gfx_vterm_println(hex, color);
}

/*
 * Draw a panic screen
 */
void gfx_panic(int ip, int code){
    //Do not print the panic message outside of the screen bounds
    if(vterm_y > res_y - (8 * 4))
        vterm_y = res_y - (8 * 4);
    //Draw directly onto VBE memory
    gfx_set_buf(GFX_BUF_VBE);
    //Print the header message
    gfx_vterm_println("QUARK PANIC @", 0x28);
    //Print error IP
    gfx_vterm_println_hex(ip, 0x28);
    //Print error code
    gfx_vterm_println("Error code:", 0x28);
    gfx_vterm_println_hex(code, 0x28);
}

/*
 * Dump memory on screen
 */
void gfx_memdump(unsigned int addr, int amount){
    //Print the header
    gfx_vterm_println("Dumping memory at", 0x0F);
    gfx_vterm_println_hex(addr, 0x0F);
    gfx_vterm_println("         00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F", 0x0D);
    //Calculate some stuff
    unsigned int ad = addr - (addr % 16);
    unsigned int am = amount + 16 - (addr % 16);
    for(unsigned int pos = 0; pos < am; pos += 16){
        //Construct one line as a string
        unsigned int ptr = ad + pos;
        char msg[57];
        for(int i = 0; i < 8; i++)
            msg[i] = hex_conv_const[(ptr >> ((7 - i) * 4)) & 0xF];
        msg[8] = ' ';
        for(int o = 0; o < 16; o++){
            unsigned char val = *(unsigned char*)(ptr + o);
            msg[9 + (o * 3)] = hex_conv_const[(val >> 4) & 0x0F];
            msg[9 + (o * 3) + 1] = hex_conv_const[val & 0x0F];
            msg[9 + (o * 3) + 2] = ' ';
        }
        msg[56] = 0;
        //Print it
        gfx_vterm_println(msg, 0x0F);
    }
}

/*
 * Checks if certain point is inside a rectangle
 */
uint8_t gfx_point_in_rect(p2d_t p, p2d_t pos, p2d_t sz){
    return p.x >= pos.x &&
           p.y >= pos.y &&
           p.x <= pos.x + sz.x &&
           p.y <= pos.y + sz.y;
}