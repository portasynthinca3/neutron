//Neutron project
//Standard GUI manager - graphics library

#include "nlib.h"

#include "gfx.h"

//Display resolution
uint16_t res_x;
uint16_t res_y;
//Second buffer pointer
color32_t* d_buf;
//Framebuffer file
FILE* framebuf;

/*
 * Copies the second buffer into the main framebuffer
 */
void gfx_flip(void){
    fseek(framebuf, 0);
    fwrite(d_buf, 4, res_x * res_y, framebuf);
}

/*
 * Reads and parses display resolution from /sys/dres
 */
void gfx_get_res(void){
    //Read the system file
    char buf[64];
    FILE* f = fopen("/sys/dres", "r");
    fgets(buf, 64, f);
    //Split X and Y parts into separate strings
    char* delimiter_pos = strchr(buf, 'x');
    *delimiter_pos = 0;
    //Parse X and Y parts
    res_x = atoi(buf);
    res_y = atoi(delimiter_pos + 1);
    //Close the file
    fclose(f);
}

/*
 * Ininitalizes stuff
 */
void gfx_init(void){
    gfx_get_res();
    d_buf = (color32_t*)malloc(4 * res_x * res_y);
    framebuf = fopen("/dev/fb", "wb");
}

/*
 * Returns display resolution
 */
p2d_t gfx_res(void){
    return (p2d_t){.x = res_x, .y = res_y};
}

/*
 * Fills the screen with one color
 */
void gfx_fill(color32_t color){
    //Set each pixel
    for(uint32_t i = 0; i < res_x * res_y; i++)
        d_buf[i] = color;
}

/*
 * Draws a horizontal line
 */
void gfx_draw_hor_line(p2d_t pos, uint64_t w, color32_t c){
    //Calculate the scanline start
    uint64_t st = pos.y * res_x;
    //Draw each pixel in the line
    for(uint64_t x = pos.x; x < pos.x + w; x++)
        d_buf[st + x] = gfx_blend_colors(d_buf[st + x], c, c.a);
}

/*
 * Draws a vertical line
 */
void gfx_draw_vert_line(p2d_t pos, uint64_t h, color32_t c){
    //Calculate the scanline start
    uint32_t st = (pos.y * res_x) + pos.x;
    //Draw each pixel in the line
    for(uint64_t o = 0; o <= h * res_x; o += res_x)
        d_buf[st + o] = gfx_blend_colors(d_buf[st + o], c, c.a);
}

/*
 * Draws a filled rectangle
 */
void gfx_draw_filled_rect(p2d_t pos, p2d_t size, color32_t c){
    //Draw each horizontal line in the rectangle
    for(uint64_t y = pos.y; y < pos.y + size.y; y++)
        gfx_draw_hor_line((p2d_t){.x = pos.x, .y = y}, size.x, c);
}

/*
 * Draws a rectangle
 */
void gfx_draw_rect(p2d_t pos, p2d_t size, color32_t c){
    //Draw two horizontal lines
    gfx_draw_hor_line((p2d_t){.x = pos.x, .y = pos.y}, (uint64_t)size.x, c);
    gfx_draw_hor_line((p2d_t){.x = pos.x, .y = pos.y + size.y}, (uint64_t)size.x, c);
    //Draw two vertical lines
    gfx_draw_vert_line((p2d_t){.x = pos.x, .y = pos.y}, (uint64_t)size.y, c);
    gfx_draw_vert_line((p2d_t){.x = pos.x + size.x, .y = pos.y}, (uint64_t)size.y, c);
}

/*
 * Draws an XBM image
 */
void gfx_draw_xbm(p2d_t position, uint8_t* xbm_ptr, p2d_t xbm_size, color32_t color_h, color32_t color_l){
    //Create some local variables
    uint8_t* ptr = xbm_ptr;
    p2d_t pos = position;
    while(1){
        //Fetch a byte
        uint8_t data = *(ptr++);
        //Cycle through its bits
        for(uint16_t x = 0; x < 8; x++){
            //Check the position
            if(pos.x - position.x < xbm_size.x){
                //If it is in bounds, draw the pixel
                if(((data >> x) & 1) && color_h.a != 0)
                    d_buf[(pos.y * res_x) + pos.x] = color_h;
                else if(color_l.a != 0)
                    d_buf[(pos.y * res_x) + pos.x] = color_l;
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
 * Draw a raw image
 */
void gfx_draw_raw(p2d_t position, uint8_t* raw_ptr, p2d_t raw_size){
    //Create a counter
    uint32_t pos = 0;
    //Go through each pixel
    for(uint32_t y = position.y; y < position.y + raw_size.y; y++){
        for(uint32_t x = position.x; x < position.x + raw_size.x; x++){
            //Fetch the data
            uint8_t r = raw_ptr[pos++];
            uint8_t g = raw_ptr[pos++];
            uint8_t b = raw_ptr[pos++];
            uint8_t a = raw_ptr[pos++];
            //Draw the pixel
            if(a != 0)
                d_buf[(y * res_x) + x] = gfx_blend_colors(d_buf[(y * res_x) + x], COLOR32(255, r, g, b), a);
        }
    }
}

/*
 * Checks if a point is inside a rectangle
 */
uint8_t gfx_point_in_rect(p2d_t p, p2d_t pos, p2d_t sz){
    return p.x >= pos.x &&
           p.y >= pos.y &&
           p.x <= pos.x + sz.x &&
           p.y <= pos.y + sz.y;
}

/*
 * Blends from background to foreground accounting the alpha value
 */
color32_t gfx_blend_colors(color32_t b, color32_t f, uint8_t a){
    if(a == 0)
        return b;
    if(a == 255)
        return f;
    return COLOR32(255, (((uint32_t)f.r * (uint32_t)a) + ((uint32_t)b.r * (uint32_t)(255 - a))) / 255,
                        (((uint32_t)f.g * (uint32_t)a) + ((uint32_t)b.g * (uint32_t)(255 - a))) / 255,
                        (((uint32_t)f.b * (uint32_t)a) + ((uint32_t)b.b * (uint32_t)(255 - a))) / 255);
}