//Neutron Project
//Quark-C - the C kernel
//(there previously was Quark, written in assembly language)

#include "../h/stdlib.h"
#include "../h/gfx.h"
#include "../h/font_neutral.h"
#include "../h/diskio.h"
#include "../h/pci.h"
#include "../h/usb.h"

struct idt_desc idt_d;

void enable_a20(void);
void isr_wrapper(void);
void isr_wrapper_code(void);

char* floppy_name[] = { "no drive", "5.25\" 360KB", "5.25\" 1.2MB", "3.5\" 720KB", "3.5\" 1.44MB", "3.5\" 2.88MB" };
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
    gfx_vterm_println("  NEUTRON QUARK is running", 0x2F);
    gfx_vterm_println(QUARK_VERSION_STR, 0x0F);

    //Print the hardware info
    gfx_vterm_println("  Detecting hardware", 0x34);
    gfx_vterm_println("Floppies:", 0x34);
    //Starting with detected floppies
    unsigned char floppies = diskio_get_floppy_drives();
    gfx_vterm_println("Floppy A:", 0x0F);
    gfx_vterm_println(floppy_name[(floppies & 0xF0) >> 4], 0x0F);
    gfx_vterm_println("Floppy B:", 0x0F);
    gfx_vterm_println(floppy_name[floppies & 0x0F], 0x0F);
    gfx_vterm_println("BIOS boot disk no.:", 0x0F);
    gfx_vterm_println_hex((int)(*(unsigned char*)(0x8FC08)), 0x0F);
    //Then, PCI devices
    gfx_vterm_println("Known PCI devices:", 0x34);
    //Scan through devices
    for(int b = 0; b < 256; b++){
        for(int d = 0; d < 256; d++){
            //Read device VID
            short vendor = pci_read_config_16(b, d, 0, 0);
            if(vendor != 0xFFFFFFFF){ //VID=FFFF means there is no device
                short product = pci_read_config_16(b, d, 0, 2); //Read PID
                short class_sub = pci_read_config_16(b, d, 0, 10); //Read class and subclass
                short if_rev = pci_read_config_16(b, d, 0, 8); //Read interface and revision
                //Try to detect if it's a known device
                if((class_sub == 0x00000C03) && ((if_rev & 0xFF00) == 0x2000)){ //Firstly, EHCI controllers (C=0C, S=03, IF=0x20)
                    gfx_vterm_println("EHCI controller found:", 0x0F);
                    unsigned char ehci_no = ehci_add_cont(b, d);
                    ehci_cont_t ehci_cont = ehci_get_cont(ehci_no);
                    gfx_vterm_println_hex((int)ehci_cont.capreg_addr, 0x0F);
                    gfx_vterm_println_hex(ehci_cont.port_cnt, 0x0F);
                    gfx_vterm_println_hex(ehci_cont.dbg_port, 0x0F);
                    gfx_vterm_println(ehci_cont.hci_ver, 0x0F);
                }
            }
        }
    }

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
