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
    //Load IDT
    load_idt(&idt_d);

    gfx_vterm_println_hex((int)font_neutral, 0x0F);

    //Tell the world about our success of getting here!
    gfx_vterm_println("NEUTRON QUARK is running", 0x2F);
    gfx_vterm_println(QUARK_VERSION_STR, 0x0F);

    //Print the hardware info
    gfx_vterm_println("Detecting hardware", 0x34);
    gfx_vterm_println("Storage:", 0x34);
    //Starting with boot divice detected floppies
    gfx_vterm_println("BIOS boot disk no.:", 0x0F);
    gfx_vterm_println_hex((int)(*(unsigned char*)(0x8FC08)), 0x0F);
    unsigned char floppies = diskio_get_floppy_drives();
    gfx_vterm_println("Floppy A:", 0x0F);
    gfx_vterm_println(floppy_name[(floppies & 0xF0) >> 4], 0x0F);
    gfx_vterm_println("Floppy B:", 0x0F);
    gfx_vterm_println(floppy_name[floppies & 0x0F], 0x0F);
    //Then, PCI devices
    gfx_vterm_println("PCI devices:", 0x34);
    pci_enumerate();
    //Hardware detection and initialization is pretty much done at this point

    //Tell the world about us initializing GUI
    gfx_vterm_println("Initializing GUI", 0x2F);
    gfx_flip(); //Transfer the VBE buffer to the secondary one
    gfx_set_buf(GFX_BUF_SEC); //Switch to the secondary buffer, enabling doublebuffering
    gfx_vterm_println("Doublebuffering enabled", 0x0F);
    gfx_flip(); //Transfer the secondary buffer to the VBE one
    //Initialize the GUI, finally
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
