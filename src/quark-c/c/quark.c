//Neutron Project
//Quark-C - the C kernel
//(there previously was Quark, written in assembly language)

#include "../h/stdlib.h"
#include "../h/gfx.h"
#include "../h/font_neutral.h"

struct idt_desc idt_d;

void enable_a20(void);
void isr_wrapper(void);
void isr_wrapper_code(void);

/*
 * The entry point for the kernel
 */
void main(void){
	//Disable interrupts
	__asm__ volatile("cli");
    //Do some initialization stuff
    enable_a20();
    dram_init();

    gfx_init();
    gfx_set_buf(GFX_BUF_VBE); //Start with logging directly onto VBE memory
    gfx_fill(0x0F);
    gfx_draw_checker(0, 0x0F);
    gfx_set_font(font_neutral);

    //Set up IDT
    struct idt_entry* idt = (struct idt_entry*)malloc(256 * sizeof(struct idt_entry));
    //Set every IDT using the same pattern
    for(int i = 0; i < 256; i++){
        int wrapper_address = (int)&isr_wrapper;
        if(i == 8 || (i >= 10 && i <= 14) || i == 17 || i == 30) //Set a special wrapper for exceptions that push error code onto stack
            wrapper_address = (int)&isr_wrapper_code;
        idt[i].offset_lower = wrapper_address & 0xFFFF;
        idt[i].offset_higher = (wrapper_address >> 16) & 0xFFFF;
        idt[i].code_selector = 0x08;
        idt[i].zero = 0;
        idt[i].type_attr = 0b10001110;
    }
    idt_d.base = (void*)idt;
    idt_d.limit = 256 * sizeof(struct idt_entry);
    load_idt(&idt_d);

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

/*
 * Interrupt Service Routine
 */
void quark_isr(void){
    //Print some info
    unsigned int ip;
    __asm__ volatile("mov %0, %%edx" : "=r" (ip));
    gfx_panic(ip, 0);

    //Hang
    while(1);
}
