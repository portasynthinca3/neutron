#ifndef GFX_H
#define GFX_H

#include "../h/stdlib.h"

//Structure defining a 2D point
typedef struct {
    signed short x;
    signed short y;
} p2d_t;

typedef unsigned char color8_t;

#define GFX_BUF_VBE 1
#define GFX_BUF_SEC 2

//Transparent color
#define COLOR_TRANSPARENT ((color8_t)255)

unsigned short gfx_res_x(void);
unsigned short gfx_res_y(void);
unsigned char* gfx_buffer(void);

void gfx_init(void);
void gfx_flip(void);
void gfx_set_font(const unsigned char* fnt);
void gfx_set_buf(unsigned char buf);

void gfx_fill(unsigned char color);
void gfx_draw_filled_rect(unsigned short sx, unsigned short sy, unsigned short w, unsigned short h, color8_t c);
void gfx_draw_hor_line(uint16_t sx, uint16_t sy, uint16_t w, color8_t c);
void gfx_draw_rect(unsigned short sx, unsigned short sy, unsigned short w, unsigned short h, color8_t c);
void gfx_draw_checker(unsigned char c1, unsigned char c2);
void gfx_draw_xbm(p2d_t position, uint8_t* xbm_ptr, p2d_t xbm_size, color8_t color_h, color8_t color_l);

void gfx_putch(p2d_t pos, color8_t color, color8_t bcolor, char c);
void gfx_puts(p2d_t pos, color8_t color, color8_t bcolor, char* s);
p2d_t gfx_text_bounds(char* s);

void gfx_vterm_println(char* s, unsigned char color);
void gfx_vterm_println_hex(int value, unsigned char color);
void gfx_panic(int ip, int code);
void gfx_memdump(unsigned int addr, int amount);

uint8_t gfx_point_in_rect(p2d_t p, p2d_t pos, p2d_t sz);

#endif