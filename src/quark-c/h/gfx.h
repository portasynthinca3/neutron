#ifndef GFX_H
#define GFX_H

#define GFX_BUF_VBE 1
#define GFX_BUF_SEC 2

void gfx_init(void);
void gfx_set_font(const unsigned char* fnt);
void gfx_set_buf(unsigned char buf);

void gfx_fill(unsigned char color);
void gfx_draw_checker(unsigned char c1, unsigned char c2);

void gfx_putch(unsigned short pos_x, unsigned short pos_y, unsigned char color, char c);
void gfx_putch_bg(unsigned short pos_x, unsigned short pos_y, unsigned char color, unsigned char bcolor, char c);
void gfx_puts(unsigned short pos_x, unsigned short pos_y, unsigned char color, char* s);
void gfx_puts_bg(unsigned short pos_x, unsigned short pos_y, unsigned char color, unsigned char bcolor, char* s);

void gfx_vterm_println(char* s, unsigned char color);
void gfx_vterm_println_hex(int value, unsigned char color);
void gfx_panic(int ip, int code);

#endif