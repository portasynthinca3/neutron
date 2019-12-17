//Neutron Project
//Quark-C - the C kernel
//(there previously was Quark, written in assembly language)

#include "./stdlib.h"
#include "./gui.h"

#include "./drivers/gfx.h"
#include "./drivers/diskio.h"
#include "./drivers/pci.h"
#include "./drivers/usb.h"
#include "./drivers/ata.h"
#include "./drivers/pic.h"
#include "./drivers/pit.h"
#include "./drivers/acpi.h"

#include "./fonts/font_neutral.h"

#include "./images/neutron_logo.xbm"

struct idt_desc idt_d;

extern void enable_a20(void);
extern void exc_wrapper(void);
extern void exc_wrapper_code(void);
extern void irq0_wrap(void);
extern void irq1_wrap(void);
extern void irq2_wrap(void);
extern void irq3_wrap(void);
extern void irq4_wrap(void);
extern void irq5_wrap(void);
extern void irq6_wrap(void);
extern void irq7_wrap(void);
extern void irq8_wrap(void);
extern void irq9_wrap(void);
extern void irq10_wrap(void);
extern void irq11_wrap(void);
extern void irq12_wrap(void);
extern void irq13_wrap(void);
extern void irq14_wrap(void);
extern void irq15_wrap(void);

uint8_t quark_verbose;

/*
 * This function is called whenever the user presses the shutdown button
 */
void quark_shutdown(ui_event_args_t* args){
    acpi_shutdown();
}

/*
 * This function is called whenever the user presses the shutdown button
 */
void quark_restart(ui_event_args_t* args){
    acpi_reboot();
}

/*
 * This function is called whenewer the user presses a GUI power button
 */
void quark_gui_callback_power_pressed(void){
    p2d_t shutdown_win_size = (p2d_t){.x = 300, .y = 150};
    //Create a window
    window_t* shutdown_window = gui_create_window("Shutdown", GUI_WIN_FLAGS_STANDARD,
                                                  (p2d_t){.x = (gfx_res_x() - shutdown_win_size.x) / 2,
                                                          .y = (gfx_res_y() - shutdown_win_size.y) / 2}, shutdown_win_size);
    //Create a label and two buttons
    char* msg = "What do you want to do?";
    p2d_t msg_bounds = gfx_text_bounds(msg);
    gui_create_label(shutdown_window, (p2d_t){.x = (shutdown_win_size.x - msg_bounds.x) / 2,
                                              .y = (shutdown_win_size.y - msg_bounds.y - 25) / 2}, msg_bounds, msg, COLOR32(255, 0, 0, 0), COLOR32(0, 0, 0, 0));
    gui_create_button(shutdown_window, (p2d_t){.x = 0, .y = shutdown_win_size.y - 25 - 13}, (p2d_t){.x = shutdown_win_size.x / 2, .y = 25}, "Shutdown", quark_shutdown,
                      COLOR32(255, 255, 255, 255), COLOR32(255, 0, 116, 255), COLOR32(255, 0, 116, 255), COLOR32(255, 0, 53, 128));
    gui_create_button(shutdown_window, (p2d_t){.x = shutdown_win_size.x / 2 - 2, .y = shutdown_win_size.y - 25 - 13}, (p2d_t){.x = shutdown_win_size.x / 2, .y = 25}, "Restart", quark_restart,
                      COLOR32(255, 255, 255, 255), COLOR32(255, 0, 116, 255), COLOR32(255, 0, 116, 255), COLOR32(255, 0, 53, 128));
    //Enable focus monopoly
    gui_set_focus_monopoly(1);
}

/*
 * Display a boot progress bar
 */
void quark_boot_status(char* str, uint32_t progress){
    //Only if we're not in verbose mode
    if(!quark_verbose){
        //Draw the screen
        gfx_draw_filled_rect((p2d_t){.x = 0, .y = gfx_res_y() / 2},
                            (p2d_t){.x = gfx_res_x(), .y = 8}, COLOR32(255, 0, 0, 0));
        gfx_puts((p2d_t){.x = (gfx_res_x() - gfx_text_bounds(str).x) / 2, .y = gfx_res_y() / 2},
                 COLOR32(255, 255, 255, 255), COLOR32(0, 0, 0, 0), str);
        gfx_draw_filled_rect((p2d_t){.x = gfx_res_x() / 3, .y = gfx_res_y() * 3 / 4}, 
                             (p2d_t){.x = gfx_res_x() / 3, .y = 2}, COLOR32(255, 64, 64, 64));
        gfx_draw_filled_rect((p2d_t){.x = gfx_res_x() / 3, .y = gfx_res_y() * 3 / 4},
                             (p2d_t){.x = gfx_res_x() / 300 * progress, .y = 2}, COLOR32(255, 255, 255, 255));
        gfx_flip();
    }
}

/*
 * The entry point for the kernel
 */
void main(void){
	//Disable interrupts
	__asm__ volatile("cli");
    //Initialize x87 FPU
    __asm__ volatile("finit");
    //Do some initialization stuff
    gui_reset_ps2_kbd();
    enable_a20();
    dram_init();

    //Set verbose mode
    quark_verbose = *(uint8_t*)(0x8FC10);
    gfx_set_verbose(quark_verbose);

    //Initialize PICs
    pic_init(32, 40); //Remap IRQs
    //Set up IDT
    struct idt_entry* idt = (struct idt_entry*)malloc(256 * sizeof(struct idt_entry));
    //Set every exception IDT entry using the same pattern
    for(int i = 0; i < 32; i++){
        int wrapper_address = (int)&exc_wrapper;
        if(i == 8 || (i >= 10 && i <= 14) || i == 17 || i == 30) //Set a special wrapper for exceptions that push error code onto stack
            wrapper_address = (int)&exc_wrapper_code;
        idt[i].offset_lower = wrapper_address & 0xFFFF;
        idt[i].offset_higher = (wrapper_address >> 16) & 0xFFFF;
        idt[i].code_selector = 0x08;
        idt[i].zero = 0;
        idt[i].type_attr = 0b10001110;
    }
    //Set up gates for IRQs
    idt[32] = IDT_ENTRY_ISR((uint32_t)&irq0_wrap);
    idt[33] = IDT_ENTRY_ISR((uint32_t)&irq1_wrap);
    idt[34] = IDT_ENTRY_ISR((uint32_t)&irq2_wrap);
    idt[35] = IDT_ENTRY_ISR((uint32_t)&irq3_wrap);
    idt[36] = IDT_ENTRY_ISR((uint32_t)&irq4_wrap);
    idt[37] = IDT_ENTRY_ISR((uint32_t)&irq5_wrap);
    idt[38] = IDT_ENTRY_ISR((uint32_t)&irq6_wrap);
    idt[39] = IDT_ENTRY_ISR((uint32_t)&irq7_wrap);
    idt[40] = IDT_ENTRY_ISR((uint32_t)&irq8_wrap);
    idt[41] = IDT_ENTRY_ISR((uint32_t)&irq9_wrap);
    idt[42] = IDT_ENTRY_ISR((uint32_t)&irq10_wrap);
    idt[43] = IDT_ENTRY_ISR((uint32_t)&irq11_wrap);
    idt[44] = IDT_ENTRY_ISR((uint32_t)&irq12_wrap);
    idt[45] = IDT_ENTRY_ISR((uint32_t)&irq13_wrap);
    idt[46] = IDT_ENTRY_ISR((uint32_t)&irq14_wrap);
    idt[47] = IDT_ENTRY_ISR((uint32_t)&irq15_wrap);
    //Load IDT
    idt_d.base = (void*)idt;
    idt_d.limit = 256 * sizeof(struct idt_entry);
    load_idt(&idt_d);
    //Initialize PIT
    pit_configure_irq0_ticks(PIT_FQ / 1000); //Generate an interrupt 1000 times a second
    //Enable interrupts
    __asm__ volatile("sti");
    //Enable non-maskable interrupts
    outb(0x70, 0);

    //Do some graphics-related initialization stuff
    gfx_init();
    gfx_set_buf(GFX_BUF_SEC); //Enable doublebuffering
    gfx_fill(COLOR32(255, 0, 0, 0));
    gfx_set_font(font_neutral);

    gfx_verbose_println(QUARK_VERSION_STR);
    gfx_verbose_println("Verbose mode is on");

    //Print the quark version
    if(!quark_verbose)
        gfx_puts((p2d_t){.x = (gfx_res_x() - gfx_text_bounds(QUARK_VERSION_STR).x) / 2, .y = gfx_res_y() - 8},
                 COLOR32(255, 255, 255, 255), COLOR32(0, 0, 0, 0), QUARK_VERSION_STR);
    //Draw the neutron logo
    gfx_draw_xbm((p2d_t){.x = (gfx_res_x() - neutron_logo_width) / 2, .y = 50}, neutron_logo_bits,
                 (p2d_t){.x = neutron_logo_width, .y = neutron_logo_height}, COLOR32(255, 255, 255, 255), COLOR32(255, 0, 0, 0));
    //Print the boot process
    quark_boot_status("Loading...", 0);

    //Enumerate PCI devices
    quark_boot_status("Detecting PCI devices", 15);
    pci_enumerate();
    //Enumerate partitions
    quark_boot_status("Detecting drive partitions", 30);
    diskio_init();
    //Initialize ACPI
    quark_boot_status("Initializing ACPI", 45);
    acpi_init();
    //Configure GUI
    quark_boot_status("Configuring GUI", 90);
    gui_init();
    //The loading process is done!
    quark_boot_status("Done!", 100);

    //Constantly update the GUI
    while(1){
        gui_update();
    }
}

/*
 * Exception ISR
 */
void quark_exc(void){
    //Print some info
    unsigned int ip;
    __asm__ volatile("mov %0, %%edx" : "=r" (ip));
    gfx_panic(ip, QUARK_PANIC_CPUEXC_CODE);

    //Hang
    while(1);
}

/*
 * IRQ ISR
 */
void quark_irq(uint32_t irq_no){
    //IRQ0 is sent by the PIT
    if(irq_no == 0)
        pit_irq0();
    //Signal End Of Interrupt
    pic_send_eoi(irq_no);
}