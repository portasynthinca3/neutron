//Neutron Project
//Quark-C - the C kernel
//(there previously was Quark, written in assembly language)

#include "../h/stdlib.h"
#include "../h/gfx.h"
#include "../h/font_neutral.h"

/*
 * The entry point for the kernel
 */
void main(void){
    //Do some initialization stuff
    a20_enable();
    dram_init();

    gfx_init();
    gfx_set_buf(GFX_BUF_VBE); //Start with logging directly onto VBE memory
    gfx_fill(0x0F);
    gfx_draw_checker(0, 0x0F);
    gfx_set_font(font_neutral);

    //Tell the world about our success of getting here!
    gfx_vterm_println("NEUTRON QUARK is running", 0x2F);
    gfx_vterm_println(QUARK_VERSION_STR, 0x0F);

    //TODO: real kernel logic

    //Print an error message, the end of a kernel should never be reached
    int ip;
    __asm__("mov $., %0" : "=r" (ip));
    gfx_panic(ip, QUARK_PANIC_CODE_END);

    //Hang
    while(1);
}