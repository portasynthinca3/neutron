//Neutron project
//Standard library for userland applications

#include "nlib.h"

/*
 * Issues a system call
 */
uint64_t _syscall(uint32_t func, uint32_t subfunc,
                         uint64_t p0, uint64_t p1, uint64_t p2, uint64_t p3, uint64_t p4){
    //Calculate RDI number
    uint64_t rdi = ((uint64_t)func << 32) | subfunc;
    //Complicated stuff: loads registers with function numbers
    //  and syscall arguments, executes syscall, while telling
    //  GCC about what registers we're about to clobber, and then
    //  extracts the return value from RAX
    uint64_t ret;
    asm("mov %1, %%rdi;"
        "mov %2, %%rsi;"
        "mov %3, %%rdx;"
        "mov %4, %%r8 ;"
        "mov %5, %%r9 ;"
        "mov %6, %%r10;"
        "syscall      ;"
        "mov %%rax, %0;"
        :
        "=m"(ret)
        :
        "m"(rdi),
        "m"(p0),
        "m"(p1),
        "m"(p2),
        "m"(p3),
        "m"(p4)
        :
        "rax", "rcx", "rdx", "r8", "r9", "r10", "r11", "rdi", "rsi");
    //Return the return value
    return ret;
}

/*
 * System call serialization/deserialization functions
 */
inline uint64_t _ser_p2d_t(p2d_t p){
    return ((uint64_t)p.x << 32) | (uint64_t)p.y;
}

inline p2d_t _deser_p2d_t(uint64_t p){
    return (p2d_t){
        .x = (int32_t)(p >> 32),
        .y = (int32_t)p
    };
}

inline uint64_t _ser_color32_t(color32_t c){
    return ((uint64_t)c.a << 24) |
           ((uint64_t)c.b << 16) |
           ((uint64_t)c.g <<  8) |
           ((uint64_t)c.r);
}

inline color32_t _deser_color32_t(uint64_t c){
    return (color32_t){
        .r = (uint8_t)c,
        .g = (uint8_t)(c >> 8),
        .b = (uint8_t)(c >> 16),
        .a = (uint8_t)(c >> 24)
    };
}

/*
 * System call: Graphics: Print a string in verbose mode
 */
inline uint64_t _gfx_println_verbose(char* str){
    return _syscall(0, 0, (uint64_t)str, 0, 0, 0, 0);
}

/*
 * System call: Graphics: Get resolution
 */
inline p2d_t _gfx_get_res(void){
    return _deser_p2d_t(_syscall(0, 1, 0, 0, 0, 0, 0));
}

/*
 * System call: Graphics: Flip buffers
 */
inline sc_state_t _gfx_flip(void){
    return _syscall(0, 2, 0, 0, 0, 0, 0);
}

/*
 * System call: Graphics: Fill buffer
 */
inline sc_state_t _gfx_fill(color32_t c){
    return _syscall(0, 3, _ser_color32_t(c), 0, 0, 0, 0);
}

/*
 * System call: Graphics: Fill rectangle
 */
inline sc_state_t _gfx_fill_rect(color32_t c, p2d_t pos, p2d_t sz){
    return _syscall(0, 4,
        _ser_color32_t(c),
        _ser_p2d_t(pos),
        _ser_p2d_t(sz),
        0, 0);
}

/*
 * System call: Graphics: Draw rectangle
 */
inline sc_state_t _gfx_draw_rect(color32_t c, p2d_t pos, p2d_t sz){
    return _syscall(0, 5,
        _ser_color32_t(c),
        _ser_p2d_t(pos),
        _ser_p2d_t(sz),
        0, 0);
}

/*
 * System call: Graphics: Draw raw image
 */
inline sc_state_t _gfx_draw_raw(uint8_t* img, p2d_t pos, p2d_t sz){
    return _syscall(0, 6,
        (uint64_t)img,
        _ser_p2d_t(pos),
        _ser_p2d_t(sz),
        0, 0);
}

/*
 * System call: Graphics: Text bounds
 */
inline p2d_t _gfx_text_bounds(char* str){
    return _deser_p2d_t(_syscall(0, 7, (uint64_t)str, 0, 0, 0, 0));
}

/*
 * System call: Graphics: Print string
 */
inline sc_state_t _gfx_draw_str(p2d_t pos, color32_t fg, color32_t bg, char* str){
    return _syscall(0, 8,
        (uint64_t)str,
        _ser_p2d_t(pos),
        (_ser_color32_t(fg) << 32) | _ser_color32_t(bg),
        0, 0);
}