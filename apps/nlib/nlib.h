//Neutron project
//Standard library for userland projects

//Definitions
#define NLIB_VERSION "1.0.0"

//Macros
//Macro for converting R, G and B values to color32_t
#define COLOR32(A, R, G, B) ((color32_t){.a = (A), .r = (R), .g = (G), .b = (B)})
//Macro for changing the alpha value of a color
#define COLOR32A(A, C) ((color32_t){.a = (A), .r = (C).r, .g = (C).g, .b = (C).b})
//Macro for converting X and values to p2d_t
#define P2D(X, Y) ((p2d_t){.x = (X), .y = (Y)})

//Standrard type definitions

typedef unsigned char uint8_t;
typedef signed char int8_t;
typedef unsigned short uint16_t;
typedef signed short int16_t;
typedef unsigned int uint32_t;
typedef signed int int32_t;
typedef long long unsigned int uint64_t;
typedef long long signed int int64_t;
typedef uint64_t size_t;
typedef uint64_t sc_state_t;

//Structure definitions
typedef struct {
  int32_t x; //X coordinate of the point
  int32_t y; //Y coordinate of the point
} __attribute__((packed)) p2d_t;

typedef struct {
  uint8_t r; //Red value of the color
  uint8_t g; //Green value of the color
  uint8_t b; //Blue value of the color
  uint8_t a; //Alpha (transparency) value of the color
} __attribute__((packed)) color32_t;

//Function prototypes

//System calls
//General syscall function
uint64_t _syscall(uint32_t func, uint32_t subfunc,
                  uint64_t p0, uint64_t p1, uint64_t p2, uint64_t p3, uint64_t p4);
//Syscalls: Graphics
uint64_t   _gfx_println_verbose (char* str);
p2d_t      _gfx_get_res         (void);
sc_state_t _gfx_flip            (void);
sc_state_t _gfx_fill            (color32_t c);
sc_state_t _gfx_fill_rect       (color32_t c, p2d_t pos, p2d_t sz);
sc_state_t _gfx_draw_rect       (color32_t c, p2d_t pos, p2d_t sz);
sc_state_t _gfx_draw_raw        (uint8_t* img, p2d_t pos, p2d_t sz);
p2d_t      _gfx_text_bounds     (char* str);
sc_state_t _gfx_draw_str        (p2d_t pos, color32_t fg, color32_t bg, char* str);