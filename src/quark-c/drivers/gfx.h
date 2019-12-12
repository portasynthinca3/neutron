#ifndef GFX_H
#define GFX_H

#include "../stdlib.h"

//Structure defining a 2D point
typedef struct {
    signed short x;
    signed short y;
} p2d_t;

//Structure defining a 24bpp color
typedef struct {
    uint8_t b, g, r;
} __attribute__((packed)) color24_t;

//Structure defining a 32bpp color
typedef struct {
    uint8_t a, r, g, b;
} __attribute__((packed)) color32_t;

#define GFX_BUF_VBE 1
#define GFX_BUF_SEC 2

//Macro for converting R, G and B values to color32_t
#define COLOR32(A, R, G, B) ((color32_t){.a = A, .r = R, .g = G, .b = B})
//Macro for dropping the alpha channel of color32_t
#define COLOR24(C) ((color24_t){.r = C.r, .g = C.g, .b = C.b})

//GFX VBE function call errors

#define GFX_VBE_OK                              0
#define GFX_VBE_ERR_NO_PMIB                     1

uint16_t gfx_res_x(void);
uint16_t gfx_res_y(void);
color24_t* gfx_buffer(void);

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

void gfx_putch(p2d_t pos, color32_t color, color32_t bcolor, char c);
void gfx_puts(p2d_t pos, color32_t color, color32_t bcolor, char* s);
p2d_t gfx_text_bounds(char* s);

void gfx_vterm_println(char* s, color32_t color);
void gfx_vterm_println_hex(int value, color32_t color);
void gfx_panic(int ip, int code);
void gfx_memdump(unsigned int addr, int amount);

uint8_t gfx_point_in_rect(p2d_t p, p2d_t pos, p2d_t sz);

#endif