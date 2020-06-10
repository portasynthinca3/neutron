//Neutron project
//Standard GUI manager - graphics library

#include "nlib.h"
#include "app_desc.h"

#include "gfx.h"

//Screen buffer
raw_img_t screen;
//Framebuffer file
FILE* framebuf;

/*
 * Copies the second buffer into the main framebuffer
 */
void gfx_flip(void){
    fseek(framebuf, 0);
    fwrite(screen.data, 4, screen.size.x * screen.size.y, framebuf);
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
    screen.size.x = atoi(buf);
    screen.size.y = atoi(delimiter_pos + 1);
    //Close the file
    fclose(f);
}

/*
 * Ininitalizes stuff
 */
void gfx_init(void){
    gfx_get_res();
    screen.data = (color32_t*)malloc(4 * screen.size.x * screen.size.y);
    framebuf = fopen("/dev/fb", "wb");
}

/*
 * Returns the screen image
 */
raw_img_t gfx_screen(void){
    return screen;
}

/*
 * Fills the screen with one color
 */
void gfx_fill(raw_img_t buf, color32_t color){
    //Set each pixel
    for(uint32_t i = 0; i < buf.size.x * buf.size.y; i++)
        buf.data[i] = color;
}

/*
 * Draws a horizontal line
 */
void gfx_draw_hor_line(raw_img_t buf, p2d_t pos, uint64_t w, color32_t c){
    if(pos.y >= buf.size.y || pos.y < 0)
        return;
    //Calculate the scanline start
    uint64_t st = pos.y * buf.size.x;
    //Draw each pixel in the line
    for(uint64_t x = pos.x; x < pos.x + w; x++)
        if(x < buf.size.x)
            buf.data[st + x] = gfx_blend_colors(buf.data[st + x], c, c.a);
}

/*
 * Draws a vertical line
 */
void gfx_draw_vert_line(raw_img_t buf, p2d_t pos, uint64_t h, color32_t c){
    //Calculate the scanline start
    uint32_t st = (pos.y * buf.size.x) + pos.x;
    //Draw each pixel in the line
    for(uint64_t o = 0; o <= h * buf.size.x; o += buf.size.x)
        if(st + o < buf.size.x * buf.size.y)
            buf.data[st + o] = gfx_blend_colors(buf.data[st + o], c, c.a);
}

/*
 * Draws a filled rectangle
 */
void gfx_draw_filled_rect(raw_img_t buf, p2d_t pos, p2d_t size, color32_t c){
    //Draw each horizontal line in the rectangle
    for(int64_t y = pos.y; y < pos.y + size.y; y++)
        gfx_draw_hor_line(buf, P2D(pos.x, y), size.x, c);
}

/*
 * Draws a round rect
 */
void gfx_draw_round_rect(raw_img_t buf, p2d_t pos, p2d_t size, int32_t r, color32_t c){
    gfx_draw_filled_rect(buf, P2D(pos.x, pos.y + r), P2D(size.x, size.y - r - r), c);
    gfx_fill_circ_helper(buf, P2D(pos.x + r, pos.y + size.y - r - 1), r, 1, size.x - r - r - 1, c);
    gfx_fill_circ_helper(buf, P2D(pos.x + r, pos.y + r), r, 2, size.x - r - r - 1, c);
}

/*
 * Helper function for gfx_draw_round_rect()
 */
void gfx_fill_circ_helper(raw_img_t buf, p2d_t pos, int32_t r, uint8_t corners, int32_t d, color32_t c){
    int32_t f = 1 - r;
    p2d_t dd_f = P2D(1, -r - r);
    int32_t y = 0;

    d++;

    while(y < r){
        if(f >= 0){
            if(corners & 1)
                gfx_draw_hor_line(buf, P2D(pos.x - y, pos.y + r), y + y + d, c);
            if(corners & 2)
                gfx_draw_hor_line(buf, P2D(pos.x - y, pos.y - r), y + y + d, c);
            r--;
            dd_f.y += 2;
            f += dd_f.y;
        }

        y++;
        dd_f.x += 2;
        f += dd_f.x;

        if(corners & 1)
            gfx_draw_hor_line(buf, P2D(pos.x - r, pos.y + y), r + r + d, c);
        if(corners & 2)
            gfx_draw_hor_line(buf, P2D(pos.x - r, pos.y - y), r + r + d, c);
    }
}

/*
 * Draws a rectangle
 */
void gfx_draw_rect(raw_img_t buf, p2d_t pos, p2d_t size, color32_t c){
    //Draw two horizontal lines
    gfx_draw_hor_line(buf, pos, (uint64_t)size.x, c);
    gfx_draw_hor_line(buf, P2D(pos.x, pos.y + size.y), (uint64_t)size.x, c);
    //Draw two vertical lines
    gfx_draw_vert_line(buf, pos, (uint64_t)size.y, c);
    gfx_draw_vert_line(buf, P2D(pos.x + size.x, pos.y), (uint64_t)size.y, c);
}

/*
 * Draws an XBM image
 */
void gfx_draw_xbm(raw_img_t buf, p2d_t pos, uint8_t* xbm_ptr, p2d_t xbm_size, color32_t color_h, color32_t color_l){
    //Create some local variables
    uint8_t* ptr = xbm_ptr;
    while(1){
        //Fetch a byte
        uint8_t data = *(ptr++);
        //Cycle through its bits
        for(uint16_t x = 0; x < 8; x++){
            //Check the position
            if(pos.x - pos.x < xbm_size.x){
                //If it is in bounds, draw the pixel
                if(((data >> x) & 1) && color_h.a != 0)
                    buf.data[(pos.y * buf.size.x) + pos.x] = color_h;
                else if(color_l.a != 0)
                    buf.data[(pos.y * buf.size.x) + pos.x] = color_l;
            }
            //Increment the position
            pos.x++;
        }
        //If the X coordinate has reached the limit, reset it and increment the Y coordinate
        if(pos.x - pos.x >= xbm_size.x){
            pos.x = pos.x;
            pos.y++;
        }
        //If the Y coordinate has reached the limit, return
        if(pos.y - pos.y >= xbm_size.y)
            return;
    }
}

/*
 * Draw a raw image
 */
void gfx_draw_raw(raw_img_t buf, p2d_t position, uint8_t* raw_ptr, p2d_t raw_size){
    uint32_t pos = 0;
    //Go through each pixel
    for(int64_t y = position.y; y < position.y + raw_size.y; y++){
        for(int64_t x = position.x; x < position.x + raw_size.x; x++){
            //Fetch the data
            uint8_t r = raw_ptr[pos++];
            uint8_t g = raw_ptr[pos++];
            uint8_t b = raw_ptr[pos++];
            uint8_t a = raw_ptr[pos++];
            //Draw the pixel
            if(a != 0 && y < buf.size.y && x < buf.size.x && y > 0 && x > 0)
                buf.data[(y * buf.size.x) + x] = gfx_blend_colors(buf.data[(y * buf.size.x) + x], COLOR32(255, r, g, b), a);
        }
    }
}

/*
 * Set the font that will be used in future
 */
font_t* gfx_load_font(const char* path){
    font_t* font = (font_t*)malloc(sizeof(font_t));
    //Read the font file
    FILE* file = fopen(path, "rb");
    if(file == NULL){
        _km_write(__APP_SHORT_NAME, "Error loading font");
        return NULL;
    }
    uint8_t* data_buf = (uint8_t*)malloc(64 * 1024);
    fread(data_buf, 1, 64 * 1024, file);
    fclose(file);
    font->data = data_buf;
    //Get the font info (keep in mind that all VLW values are in the retarded big-endian)
    font->g_count = *(uint32_t*)(font->data + 0); bswap_dw(&font->g_count);
    font->ver = *(uint32_t*)(font->data + 4); bswap_dw(&font->ver);
    font->size = *(uint32_t*)(font->data + 8); bswap_dw(&font->size);
    font->ascent = *(uint32_t*)(font->data + 16); bswap_dw(&font->ascent);
    font->descent = *(uint32_t*)(font->data + 20); bswap_dw(&font->descent);
    //Allocate some space for the bitmap pointer cache
    font->bmp = (uint32_t*)malloc(font->g_count * sizeof(uint32_t));
    //Walk through the font file to cache each glyph's bitmap position
    uint32_t bmp_offs = 24 + (28 * font->g_count);
    for(uint32_t i = 0; i < font->g_count; i++){
        //Get glyph info pointer
        const uint8_t* ginfo = font->data + 24 + (28 * i);
        //Get bitmap width and height
        int32_t bmp_width = *(int32_t*)(ginfo + 8); bswap_dw((uint32_t*)&bmp_width);
        int32_t bmp_height = *(int32_t*)(ginfo + 4); bswap_dw((uint32_t*)&bmp_height);
        //Cache the bitmap offset
        font->bmp[i] = bmp_offs;
        //Add to the total offset
        bmp_offs += bmp_width * bmp_height;
    }
    //Return the loaded font
    return font;
}

/*
 * Put a glyph with backgrund color in video buffer, return the size of the drawn character
 */
p2d_t gfx_glyph(raw_img_t buf, font_t* font, p2d_t pos, color32_t color, color32_t bcolor, uint32_t c){
    //Find the glyph
    const uint8_t* glyph_ptr = NULL;
    uint32_t glyph_no = 0;
    for(uint32_t i = 0; i < font->g_count; i++){
        //Calculate the pointer
        const uint8_t* ptr = font->data + 24 + (28 * i);
        //Check the codepoint
        uint32_t cp = *(uint32_t*)ptr; bswap_dw(&cp);
        //Check the equality
        if(cp == c){
            glyph_ptr = ptr;
            glyph_no = i;
            break;
        }
    }
    //If no such codepoint exists, draw a rectangle and return
    if((glyph_ptr == NULL) && (c != ' ')){
        gfx_draw_rect(buf, (p2d_t){pos.x + 1, pos.y - font->ascent}, (p2d_t){font->size / 2, font->ascent + font->descent - 3}, color);
        return (p2d_t){font->size / 2 + 2, font->size};
    } else if(c == ' ') //Treat the missing space character specially
        return (p2d_t){font->size / 2, font->size};
    //Get the glyph properties
    int32_t height = *(int32_t*)(glyph_ptr + 4); bswap_dw((uint32_t*)&height);
    int32_t width = *(int32_t*)(glyph_ptr + 8); bswap_dw((uint32_t*)&width);
    int32_t x_advance = *(int32_t*)(glyph_ptr + 12); bswap_dw((uint32_t*)&x_advance);
    int32_t dy = *(int32_t*)(glyph_ptr + 16); bswap_dw((uint32_t*)&dy);
    int32_t dx = *(int32_t*)(glyph_ptr + 20); bswap_dw((uint32_t*)&dx);
    //Calculate the video buffer offset
    int64_t buf_offset = ((pos.y - dy) * buf.size.x) + pos.x + dx;
    //Render the bitmap
    uint8_t* bmp_offs = (uint8_t*)font->data + font->bmp[glyph_no];
    for(uint32_t y = 0; y < height; y++){
        for(uint32_t x = 0; x < width; x++){
            //Get the alpha value
            uint8_t alpha = *bmp_offs;
            //Check buffer boundary
            if(buf_offset < buf.size.x * buf.size.y && buf_offset >= 0){
                //Interpolate between the background color and the foreground color
                color32_t c = buf.data[buf_offset];
                if(color.a > 0){
                    c = gfx_blend_colors(c, bcolor, bcolor.a);
                    color32_t f = gfx_blend_colors(c, color, color.a);
                    c = gfx_blend_colors(c, f, alpha);
                }
                buf.data[buf_offset] = c;
            }
            //Advance the video buffer pointer
            buf_offset++;
            bmp_offs++;
        }
        //Advance the video buffer pointer by one line
        buf_offset += buf.size.x - width;
    }
    //Report the character width and height
    return (p2d_t){x_advance, font->size};
}

/*
 * Put a string in video buffer
 */
p2d_t gfx_draw_str(raw_img_t buf, font_t* font, p2d_t pos, color32_t color, color32_t bcolor, char* s){
    p2d_t sz = (p2d_t){.x = 0, .y = font->size};
    //Data byte, position, state, currently decoded codepoint and counter
    char c = 0;
    uint32_t codepoint = 0;
    uint8_t utf8_len = 0;
    p2d_t pos_actual = pos;
    uint32_t state = 0;
    uint32_t i = 0;
    //Fetch the next character
    while((c = s[i++]) != 0){
        if(state == 0){ //First UTF-8 byte
            codepoint = 0; //Reset the codepoint
            //Check if it's a 1, 2, 3 or a 4-byte encoding
            if((c & 0b10000000) == 0){
                utf8_len = 1;
                codepoint = c & 0b01111111;
                state = 4; //Reading done
            } else if((c & 0b11100000) == 0b11000000){
                utf8_len = 2;
                codepoint = c & 0b00011111;
                codepoint <<= 6;
                state = 1; //Second byte
            } else if((c & 0b11110000) == 0b11100000){
                utf8_len = 3;
                codepoint = c & 0b00001111;
                codepoint <<= 12;
                state = 2; //Third byte
            } else if((c & 0b11111000) == 0b11110000){
                utf8_len = 3;
                codepoint = c & 0b00000111;
                codepoint <<= 17;
                state = 3; //Fourth byte
            }
        } else if(state == 1){ //Second UTF-8 byte
            if((c & 0b11000000) != 0b10000000)
                return sz; //Invalid UTF-8 sequence
            codepoint |= (uint32_t)(c & 0b00111111) << ((utf8_len - 2) * 6); //Extract 6 least significant bits and shift them in place
            if(utf8_len == 2)
                state = 4; //Reading done
            else
                state = 2; //Read third byte
        } else if(state == 2){ //Third UTF-8 byte
            if((c & 0b11000000) != 0b10000000)
                return sz; //Invalid UTF-8 sequence
            codepoint |= (uint32_t)(c & 0b00111111) << ((utf8_len - 3) * 6); //Extract 6 least significant bits and shift them in place
            if(utf8_len == 3)
                state = 4; //Reading done
            else
                state = 2; //Read fourth byte
        } else if(state == 3){ //Fourth UTF-8 byte
            if((c & 0b11000000) != 0b10000000)
                return sz; //Invalid UTF-8 sequence
            codepoint |= c & 0b00111111; //Extract 6 least significant bits
            state = 4; //Reading done
        }
        if(state == 4){ //Reading done
            switch(codepoint){
                case '\n': //Carriage return
                    pos_actual.x = pos.x;
                    pos_actual.y += font->size;
                    sz.y += font->size;
                    break;
                default: { //Print the char and advance its position
                    p2d_t char_size = gfx_glyph(buf, font, pos_actual, color, bcolor, codepoint);
                    pos_actual.x += char_size.x;
                    if(pos_actual.x - pos.x > sz.x)
                        sz.x = pos_actual.x - pos.x;
                    break;
                }
            }
            state = 0; //Back to reading the first byte
        }
    }
    return sz;
}

/*
 * Calculate the bounds of a string if it was rendered on screen
 */
p2d_t gfx_text_bounds(raw_img_t buf, font_t* font, char* s){
    //Print the text transparently to retrieve the size
    return gfx_draw_str(buf, font, P2D(0, 0), COLOR32(0, 0, 0, 0), COLOR32(0, 0, 0, 0), s);
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