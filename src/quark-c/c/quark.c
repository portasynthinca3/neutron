//Neutron Project
//Quark-C - the C kernel
//(there previously was Quark, written in assembly language)

#include "../h/stdlib.h"
#include "../h/gfx.h"
#include "../h/gui.h"
#include "../h/font_neutral.h"
#include "../h/diskio.h"
#include "../h/pci.h"
#include "../h/usb.h"

struct idt_desc idt_d;

void enable_a20(void);
void isr_wrapper(void);
void isr_wrapper_code(void);

char* floppy_name[] = { "Not present", "5.25\" 360KB", "5.25\" 1.2MB", "3.5\" 720KB", "3.5\" 1.44MB", "3.5\" 2.88MB" };
const char hcc[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

/*
 * Display a boot progress bar
 */
void krnl_boot_status(char* str, uint32_t progress){
    //Draw the screen
    gfx_draw_filled_rect(0, gfx_res_y() / 2, gfx_res_x(), 8, 0x1E);
    gfx_puts((gfx_res_x() - (strlen(str) * 6)) / 2, gfx_res_y() / 2, 0, str);
    gfx_draw_hor_line(gfx_res_x() / 3, gfx_res_y() * 3 / 4, gfx_res_x() / 3, 0x15);
    gfx_draw_hor_line(gfx_res_x() / 3, gfx_res_y() * 3 / 4, gfx_res_x() / 300 * progress, 0x2F);
    gfx_flip();
}

/*
 * The entry point for the kernel
 */
void main(void){
	//Disable interrupts
	__asm__ volatile("cli");
    //Do some initialization stuff
    enable_a20();
    dram_init();

    //Do some graphics-related initialization stuff
    gfx_init();
    gfx_set_buf(GFX_BUF_SEC); //Enable doublebuffering
    gfx_fill(0x1E);
    gfx_set_font(font_neutral);

    //Print the quark version
    gfx_puts((gfx_res_x() - (strlen(QUARK_VERSION_STR) * 6)) / 2, 0, 0, QUARK_VERSION_STR);
    //Print the boot process
    krnl_boot_status("N.Quark-C is running", 5);

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
    //Load IDT
    load_idt(&idt_d);

    //Enumerate PCI devices
    krnl_boot_status("Detecting hardware", 15);
    pci_enumerate();
    //Configure GUI
    krnl_boot_status("Configuring GUI", 90);
    gui_init();

    //Constantly update the GUI
    while(1){
        gui_update();
    }

    //Print an error message, the end of the kernel should never be reached
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
