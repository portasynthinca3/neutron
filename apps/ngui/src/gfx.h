#pragma once

//Structure definitions

//2D floating-point point
typedef struct {
    double x;
    double y;
} p2df_t;

//2D point
typedef struct {
    int64_t x;
    int64_t y;
} p2d_t;

//32bpp color
typedef struct {
    uint8_t b, g, r, a;
} __attribute__((packed)) color32_t;

//Font
typedef struct {
    uint8_t* data;
    uint32_t g_count;
    uint32_t ver;
    uint32_t size;
    uint32_t ascent;
    uint32_t descent;
    uint32_t* bmp;
} font_t;

//Raw image in memory
typedef struct {
    p2d_t size;
    color32_t* data;
} raw_img_t;

//Definitions

//Macro for converting R, G and B values to color32_t
#define COLOR32(A, R, G, B) ((color32_t){.a = (A), .r = (R), .g = (G), .b = (B)})
//Macro for changing the alpha value of a color
#define COLOR32A(A, C) ((color32_t){.a = (A), .r = (C).r, .g = (C).g, .b = (C).b})
//Macro for converting X and Y values to p2d_t
#define P2D(X, Y) ((p2d_t){.x = (X), .y = (Y)})
//Macro for converting X and Y values to p2df_t
#define P2DF(X, Y) ((p2df_t){.x = (X), .y = (Y)})

//Function prototypes

//Control stuff
void      gfx_flip    (void);
void      gfx_get_res (void);
void      gfx_init    (void);
raw_img_t gfx_screen  (void);
//Draw operations
void gfx_fill             (raw_img_t buf, color32_t color);
void gfx_draw_hor_line    (raw_img_t buf, p2d_t pos, uint64_t w, color32_t c);
void gfx_draw_vert_line   (raw_img_t buf, p2d_t pos, uint64_t h, color32_t c);
void gfx_draw_filled_rect (raw_img_t buf, p2d_t pos, p2d_t size, color32_t c);
void gfx_draw_round_rect  (raw_img_t buf, p2d_t pos, p2d_t size, int32_t r, color32_t c);
void gfx_fill_circ_helper (raw_img_t buf, p2d_t pos, int32_t r, uint8_t corners, int32_t d, color32_t c);
void gfx_draw_rect        (raw_img_t buf, p2d_t pos, p2d_t size, color32_t c);
void gfx_draw_xbm         (raw_img_t buf, p2d_t pos, uint8_t* xbm_ptr, p2d_t xbm_size, color32_t color_h, color32_t color_l);
void gfx_draw_raw         (raw_img_t buf, p2d_t pos, raw_img_t raw);
void gfx_draw_raw_rgba    (raw_img_t buf, p2d_t position, uint8_t* raw_ptr, p2d_t raw_size);
//Text drawing functions
font_t* gfx_load_font   (const char* path);
p2d_t   gfx_glyph       (raw_img_t buf, font_t* font, p2d_t pos, color32_t color, color32_t bcolor, uint32_t c);
p2d_t   gfx_draw_str    (raw_img_t buf, font_t* font, p2d_t pos, color32_t color, color32_t bcolor, char* s);
p2d_t   gfx_text_bounds (font_t* font, char* s);
//Utility
uint8_t   gfx_point_in_rect (p2d_t p, p2d_t pos, p2d_t sz);
color32_t gfx_blend_colors  (color32_t b, color32_t f, uint8_t a);