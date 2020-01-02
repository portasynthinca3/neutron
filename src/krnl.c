//Neutron Project
//The kernel

#include <efi.h>
#include <efilib.h>

#include "./stdlib.h"

#include "./gui/gui.h"
#include "./gui/stdgui.h"

#include "./drivers/gfx.h"
#include "./drivers/diskio.h"
#include "./drivers/pci.h"
#include "./drivers/usb.h"
#include "./drivers/ata.h"
#include "./drivers/apic.h"
#include "./drivers/timr.h"
#include "./drivers/human_io/ps2.h"
#include "./drivers/human_io/mouse.h"
#include "./drivers/acpi.h"

#include "./fonts/font_neutral.h"

#include "./images/neutron_logo.xbm"

#include "./mtask/mtask.h"

struct idt_desc idt_d;

extern void enable_a20(void);
extern void exc_wrapper(void);
extern void exc_wrapper_code(void);
extern void apic_timer_isr_wrap(void);

//Is the kernel in verbose mode or not?
uint8_t krnl_verbose;

//Pointer to the EFI system table
EFI_SYSTEM_TABLE* krnl_efi_systable;

/*
 * Gets the EFI system table pointer
 */
EFI_SYSTEM_TABLE* krnl_get_efi_systable(void){
    return krnl_efi_systable;
}

/*
 * This function is called whenever the user chooses the system color
 */
void sys_color_change(ui_event_args_t* args){
    color32_t color = stdgui_cpick_get_color();
    //Get and assign the color
    gui_get_color_scheme()->win_border = color;
    //Get and assign another color
    gui_get_color_scheme()->win_border_inactive = COLOR32(color.a, color.r + 30, color.g + 30, color.b + 30);
    //Close the window
    gui_destroy_window(args->win);
}

/*
 * This function is caled whenever the user requests the system color picker
 */
void krnl_open_sys_color_picker(ui_event_args_t* args){
    stdgui_create_color_picker(sys_color_change, gui_get_color_scheme()->win_border);
}

/*
 * This function is called whenever someone requests to shutdown
 */
void krnl_shutdown(void){
    acpi_shutdown();
}

/*
 * This function is called whenever someone requests to restart
 */
void krnl_reboot(void){
    acpi_reboot();
}

/*
 * This function is called whenewer the user presses a GUI power button
 */
void krnl_gui_callback_power_pressed(void){
    stdgui_create_shutdown_prompt();
}

/*
 * This function is called whenewer the user presses a GUI system button
 */
void krnl_gui_callback_system_pressed(void){
    stdgui_create_system_win();
}

/*
 * Display a boot progress bar
 */
void krnl_boot_status(char* str, uint32_t progress){
    //Only if we're not in verbose mode
    if(!krnl_verbose){
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
    } else {
        //If we are, however, draw print the string using verbose mode
        gfx_verbose_println(str);
    }
}

uint64_t dummy_var = 0;

void dummy_task(void){
    while(1){
        dummy_var++;
    }
}

/*
 * GUI task code
 */
void gui_task(void){
    mtask_create_task(8192, "dummy", dummy_task);
    while(1){
        ps2_poll();
        gui_update();
        mouse_frame_end();
    }
}

/*
 * The entry point for the kernel
 */
EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable){
    //Save the system table pointer
    krnl_efi_systable = SystemTable;
    //Disable the watchdog timer
    krnl_efi_systable->BootServices->SetWatchdogTimer(0, 0, 0, NULL);
    //Print the boot string
    krnl_efi_systable->ConOut->OutputString(SystemTable->ConOut, (CHAR16*)L"Neutron is starting up\r\n");

	//Disable interrupts
	__asm__ volatile("cli");
    //Initialize x87 FPU
    __asm__ volatile("finit");
    //Do some initialization stuff
    dram_init();

    //Set verbose mode
    krnl_verbose = 1;
    gfx_set_verbose(krnl_verbose);

    //Do some graphics-related initialization stuff
    gfx_init();
    gfx_set_buf(GFX_BUF_SEC); //Enable doublebuffering
    gfx_fill(COLOR32(255, 0, 0, 0));
    gfx_set_font(font_neutral);

    gfx_verbose_println(KRNL_VERSION_STR);
    gfx_verbose_println("Verbose mode is on");

    //Print the krnl version
    if(!krnl_verbose)
        gfx_puts((p2d_t){.x = (gfx_res_x() - gfx_text_bounds(KRNL_VERSION_STR).x) / 2, .y = gfx_res_y() - 8},
                 COLOR32(255, 255, 255, 255), COLOR32(0, 0, 0, 0), KRNL_VERSION_STR);
    //Draw the neutron logo
    gfx_draw_xbm((p2d_t){.x = (gfx_res_x() - neutron_logo_width) / 2, .y = 50}, neutron_logo_bits,
                 (p2d_t){.x = neutron_logo_width, .y = neutron_logo_height}, COLOR32(255, 255, 255, 255), COLOR32(255, 0, 0, 0));
    //Print the boot process
    krnl_boot_status(">>> Loading <<<", 0);

    //Initialize PS/2
    krnl_boot_status(">>> Initializing PS/2 <<<", 15);
    ps2_init();
    //Enumerate PCI devices
    krnl_boot_status(">>> Detecting PCI devices <<<", 30);
    pci_enumerate();
    //Enumerate partitions
    krnl_boot_status(">>> Detecting drive partitions <<<", 45);
    diskio_init();
    //Initialize ACPI
    krnl_boot_status(">>> Initializing ACPI <<<", 60);
    acpi_init();
    //Configure GUI
    krnl_boot_status(">>> Configuring GUI <<<", 95);
    gui_init();
    //Set up IDT
    krnl_boot_status(">>> Setting up interrupts <<<", 97);
    //Exit UEFI boot services before we can use IDT
    SystemTable->BootServices->ExitBootServices(ImageHandle, 0);
    //Get the current code selector
    uint16_t cur_cs = 0;
    __asm__ volatile("movw %%cs, %0" : "=r" (cur_cs));
    //Set up IDT
    struct idt_entry* idt = (struct idt_entry*)malloc(256 * sizeof(struct idt_entry));
    //Set every exception IDT entry using the same pattern
    for(int i = 0; i < 32; i++){
        uint64_t wrapper_address = (uint64_t)&exc_wrapper;
        if(i == 8 || (i >= 10 && i <= 14) || i == 17 || i == 30) //Set a special wrapper for exceptions that push error code onto stack
            wrapper_address = (uint64_t)&exc_wrapper_code;
        idt[i] = IDT_ENTRY_ISR(wrapper_address, cur_cs);
    }
    //Set up gates for interrupts
    idt[32] = IDT_ENTRY_ISR((uint64_t)&apic_timer_isr_wrap, cur_cs);
    //Load IDT
    idt_d.base = (void*)idt;
    idt_d.limit = 256 * sizeof(struct idt_entry);
    load_idt(&idt_d);
    //Enable interrupts
    __asm__ volatile("sti");
    //Enable non-maskable interrupts
    outb(0x70, 0);
    //Initialize the APIC
    krnl_boot_status(">>> Initializing APIC <<<", 98);
    apic_init();
    //Initialize the multitasking system
    krnl_boot_status(">>> Initializing multitasking <<<", 99);
    mtask_init();

    //The loading process is done!
    krnl_boot_status(">>> Done <<<", 100);

    mtask_create_task(16384, "System GUI", gui_task);
    //The multitasking core is designed in such a way that after the
    //  first ever call to mtask_create_task() the execution of the
    //  caller function stops forever, so we won't go any further
    //But just to double-check,
    while(1);
}

/*
 * Exception ISR
 */
void krnl_exc(void){
    //Stop the schaeduler
    mtask_stop();
    //Print some info
    uint64_t ip;
    __asm__ volatile("mov %%rdx, %0" : "=r" (ip));
    gfx_panic(ip, KRNL_PANIC_CPUEXC_CODE);

    //Load 0xDEADBEEF into EAX for ease of debugging
    __asm__ volatile("mov $0xDEADBEEF, %eax");
    //Abort
    abort();
}