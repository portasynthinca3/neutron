//Neutron project
//System call support

#include "./syscall.h"
#include "../../krnl.h"
#include "../../stdlib.h"
#include "../../vmem/vmem.h"
#include "../../mtask/mtask.h"
#include "../../drivers/gfx.h"
#include "../elf/elf.h"

//Kernel mode RSP
uint64_t krnl_rsp = 0;

void syscall_init(void){
    //Create a kernel mode stack
    krnl_rsp = (uint64_t)malloc(65536) + 65536;
}

uint64_t syscall_get_krnl_rsp(void){
    return krnl_rsp;
}

/*
 * Various serialization/deserialization functions
 */
uint64_t _ser_p2d_t(p2d_t p){
    return ((uint64_t)p.x << 32) | (uint64_t)p.y;
}

p2d_t _deser_p2d_t(uint64_t p){
    return (p2d_t){
        .x = (int32_t)(p >> 32),
        .y = (int32_t)p
    };
}

uint64_t _ser_color32_t(color32_t c){
    return ((uint64_t)c.a << 24) |
           ((uint64_t)c.b << 16) |
           ((uint64_t)c.g <<  8) |
           ((uint64_t)c.r);
}

color32_t _deser_color32_t(uint64_t c){
    return (color32_t){
        .r = (uint8_t)c,
        .g = (uint8_t)(c >> 8),
        .b = (uint8_t)(c >> 16),
        .a = (uint8_t)(c >> 24)
    };
}

/*
 * Handles a system call
 */
uint64_t syscall_handle(void){
    //Get syscall function/subfunction numbers and arguments
    uint64_t num, p0, p1, p2, p3, p4;
    asm volatile("mov %%rdi, %0;"
                 "mov %%rsi, %1;"
                 "mov %%rdx, %2;"
                 "mov %%r8,  %3;"
                 "mov %%r9,  %4;"
                 "mov %%r10, %5;" :
                 "=m"(num), "=m"(p0), "=m"(p1), "=m"(p2), "=m"(p3), "=m"(p4));
    //Function number is in higher 32 bits
    uint32_t func = num >> 32;
    //Subfunction number is in lower 32 bits
    uint32_t subfunc = (uint32_t)num;
    switch(func){
        case 0: //graphics
            switch(subfunc){
                case 0: //print in verbose mode
                    //check range (should be in userland)
                    if(p0 + strlen((char*)p0) >= 0x800000000000ULL)
                        return 0xFFFFFFFFFFFFFFFF;
                    //print the string
                    gfx_verbose_println((char*)p0);
                    return 0;
                case 1: //get screen resolution
                    return _ser_p2d_t((p2d_t){
                        .x = gfx_res_x(),
                        .y = gfx_res_y()
                    });
                case 2: //flip buffers
                    gfx_flip();
                    return 0;
                case 3: //fill buffer
                    gfx_fill(_deser_color32_t(p0));
                    return 0;
                case 4: { //fill rectangle
                        //check range (should not exceed buffer limits)
                        p2d_t pos = _deser_p2d_t(p1);
                        p2d_t sz = _deser_p2d_t(p2);
                        uint64_t last_pt =  (pos.x + sz.x) +
                                        ((pos.y + sz.y) * gfx_res_x());
                        if(pos.x < 0 || pos.y < 0 || sz.x < 0 || sz.y < 0)
                            return 0xFFFFFFFFFFFFFFFF;
                        if(last_pt > gfx_res_x() * gfx_res_y())
                            return 0xFFFFFFFFFFFFFFFF;
                        //draw if everything is OK
                        gfx_draw_filled_rect(pos, sz, _deser_color32_t(p0));
                        return 0;
                    }
                case 5: { //draw rectangle
                        //check range (should not exceed buffer limits)
                        p2d_t pos = _deser_p2d_t(p1);
                        p2d_t sz = _deser_p2d_t(p2);
                        uint64_t last_pt =  (pos.x + sz.x) +
                                        ((pos.y + sz.y) * gfx_res_x());
                        if(pos.x < 0 || pos.y < 0 || sz.x < 0 || sz.y < 0)
                            return 0xFFFFFFFFFFFFFFFF;
                        if(last_pt > gfx_res_x() * gfx_res_y())
                            return 0xFFFFFFFFFFFFFFFF;
                        //draw if everything is OK
                        gfx_draw_rect(pos, sz, _deser_color32_t(p0));
                        return 0;
                    }
                case 6: { //draw raw image
                        //check range (should not exceed buffer limits)
                        p2d_t pos = _deser_p2d_t(p1);
                        p2d_t sz = _deser_p2d_t(p2);
                        uint64_t last_pt =  (pos.x + sz.x) +
                                        ((pos.y + sz.y) * gfx_res_x());
                        if(pos.x < 0 || pos.y < 0 || sz.x < 0 || sz.y < 0)
                            return 0xFFFFFFFFFFFFFFFF;
                        if(last_pt > gfx_res_x() * gfx_res_y())
                            return 0xFFFFFFFFFFFFFFFF;
                        //draw if everything is OK
                        gfx_draw_raw(pos, (uint8_t*)p0, sz);
                        return 0;
                    }
                case 7: { //calculate text bounds
                        //check pointer (should be in userspace)
                        if(p0 + strlen((char*)p0) >= 0x800000000000ULL)
                            return 0xFFFFFFFFFFFFFFFF;
                        //return the result if everything is OK
                        return _ser_p2d_t(gfx_text_bounds((char*)p0));
                    }
                case 8: { //print string
                        //check pointer (should be in userspace)
                        if(p0 + strlen((char*)p0) >= 0x800000000000ULL)
                            return 0xFFFFFFFFFFFFFFFF;
                        //check range (should not exceed the buffer limits)
                        p2d_t pos = _deser_p2d_t(p1);
                        p2d_t sz = gfx_text_bounds((char*)p3);
                        uint64_t last_pt =  (pos.x + sz.x) +
                                        ((pos.y + sz.y) * gfx_res_x());
                        if(pos.x < 0 || pos.y < 0 || sz.x < 0 || sz.y < 0)
                            return 0xFFFFFFFFFFFFFFFF;
                        if(last_pt > gfx_res_x() * gfx_res_y())
                            return 0xFFFFFFFFFFFFFFFF;
                        //draw if everything is OK
                        gfx_puts(pos, _deser_color32_t(p2 >> 32), _deser_color32_t(p2), (char*)p0);
                        return 0;
                    }
                default: //invalid subfunction number
                    return 0xFFFFFFFFFFFFFFFF;
            }
        default: //invalid function number
            return 0xFFFFFFFFFFFFFFFF;
    }
}