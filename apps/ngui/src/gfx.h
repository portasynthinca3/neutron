#pragma once

//Structure definitions

//Structure defining a 2D point
typedef struct {
    int64_t x;
    int64_t y;
} p2d_t;

//Structure defining a 32bpp color
typedef struct {
    uint8_t b, g, r, a;
} __attribute__((packed)) color32_t;

//Definitions

//Macro for converting R, G and B values to color32_t
#define COLOR32(A, R, G, B) ((color32_t){.a = (A), .r = (R), .g = (G), .b = (B)})
//Macro for changing the alpha value of a color
#define COLOR32A(A, C) ((color32_t){.a = (A), .r = (C).r, .g = (C).g, .b = (C).b})
//Macro for converting X and values to p2d_t
#define P2D(X, Y) ((p2d_t){.x = (X), .y = (Y)})

//Function prototypes

//Control stuff
void  gfx_flip    (void);
void  gfx_get_res (void);
void  gfx_init    (void);
p2d_t gfx_res     (void);
//Draw operations
void gfx_fill              (color32_t color);
void gfx_draw_hor_line     (p2d_t pos, uint64_t w, color32_t c);
void gfx_draw_vert_line    (p2d_t pos, uint64_t h, color32_t c);
void gfx_draw_filled_rect  (p2d_t pos, p2d_t size, color32_t c);
void gfx_draw_rect         (p2d_t pos, p2d_t size, color32_t c);
void gfx_draw_xbm          (p2d_t position, uint8_t* xbm_ptr, p2d_t xbm_size, color32_t color_h, color32_t color_l);
void gfx_draw_raw          (p2d_t position, uint8_t* raw_ptr, p2d_t raw_size);
//Utility
uint8_t   gfx_point_in_rect (p2d_t p, p2d_t pos, p2d_t sz);
color32_t gfx_blend_colors  (color32_t b, color32_t f, uint8_t a);