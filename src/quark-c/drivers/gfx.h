#ifndef GFX_H
#define GFX_H

#include "../stdlib.h"

//Structure defining a 2D point
typedef struct {
    uint32_t x;
    uint32_t y;
} p2d_t;

//Structure defining a 32bpp color
typedef struct {
    uint8_t b, g, r, a;
} __attribute__((packed)) color32_t;

#define GFX_BUF_VBE 1
#define GFX_BUF_SEC 2

//Macro for converting R, G and B values to color32_t
#define COLOR32(A, R, G, B) ((color32_t){.a = A, .r = R, .g = G, .b = B})

uint32_t gfx_res_x(void);
uint32_t gfx_res_y(void);
color32_t* gfx_buffer(void);

void gfx_init(void);
void gfx_flip(void);
void gfx_set_font(const unsigned char* fnt);
void gfx_set_buf(unsigned char buf);

void gfx_fill(color32_t color);
void gfx_draw_filled_rect(p2d_t pos, p2d_t size, color32_t c);
void gfx_draw_hor_line(p2d_t pos, uint16_t w, color32_t c);
void gfx_draw_vert_line(p2d_t pos, uint16_t h, color32_t c);
void gfx_draw_rect(p2d_t pos, p2d_t size, color32_t c);
void gfx_draw_xbm(p2d_t position, uint8_t* xbm_ptr, p2d_t xbm_size, color32_t color_h, color32_t color_l);
void gfx_draw_raw(p2d_t position, uint8_t* raw_ptr, p2d_t raw_size);

void gfx_putch(p2d_t pos, color32_t color, color32_t bcolor, char c);
void gfx_puts(p2d_t pos, color32_t color, color32_t bcolor, char* s);
p2d_t gfx_text_bounds(char* s);

void gfx_panic(int ip, int code);
void gfx_memdump(unsigned int addr, int amount);

uint8_t gfx_point_in_rect(p2d_t p, p2d_t pos, p2d_t sz);

#endif