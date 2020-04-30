//Neutron Project
//The kernel

#include "./krnl.h"

#include "./stdlib.h"
#include "./cpuid.h"

#include <efi.h>
#include <efilib.h>

#include "./drivers/gfx.h"
#include "./drivers/diskio.h"
#include "./drivers/pci.h"
#include "./drivers/apic.h"
#include "./drivers/timr.h"
#include "./drivers/human_io/mouse.h"
#include "./drivers/acpi.h"
#include "./drivers/initrd.h"

#include "./fonts/jb-mono-10.h"

#include "./images/neutron_logo.h"
#include "./images/boot_err.h"

#include "./mtask/mtask.h"

#include "./vmem/vmem.h"

#include "./app_drv/elf/elf.h"

struct idt_desc idt_d;

extern void enable_a20(void);
extern void apic_timer_isr_wrap(void);

//Exception wrapper definitions
extern void exc_0(void);
extern void exc_1(void);
extern void exc_2(void);
extern void exc_3(void);
extern void exc_4(void);
extern void exc_5(void);
extern void exc_6(void);
extern void exc_7(void);
extern void exc_8(void);
extern void exc_9(void);
extern void exc_10(void);
extern void exc_11(void);
extern void exc_12(void);
extern void exc_13(void);
extern void exc_14(void);
extern void exc_16(void);
extern void exc_17(void);
extern void exc_18(void);
extern void exc_19(void);
extern void exc_20(void);
extern void exc_30(void);

//Where the kernel is loaded in memory
krnl_pos_t krnl_pos;

//Stack Smashing Protection guard
uint64_t __stack_chk_guard;

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
 * Returns the information about where the in memory the kernel is located
 */
krnl_pos_t krnl_get_pos(void){
    return krnl_pos;
}

/*
 * Dumps the task state on screen
 */
void krnl_dump_task_state(task_t* task){
    char temp[500];
    temp[0] = 0;
    char temp2[20];

    //Print RAX-RBX, RSI, RDI, RSP, RBP
    strcat(temp, "  RAX=");
    strcat(temp, sprintub16(temp2, task->state.rax, 16));
    strcat(temp, " RBX=");
    strcat(temp, sprintub16(temp2, task->state.rbx, 16));
    strcat(temp, " RCX=");
    strcat(temp, sprintub16(temp2, task->state.rcx, 16));
    strcat(temp, " RDX=");
    strcat(temp, sprintub16(temp2, task->state.rdx, 16));
    strcat(temp, " RSI=");
    strcat(temp, sprintub16(temp2, task->state.rsi, 16));
    strcat(temp, " RDI=");
    strcat(temp, sprintub16(temp2, task->state.rdi, 16));
    strcat(temp, " RSP=");
    strcat(temp, sprintub16(temp2, task->state.rsp, 16));
    strcat(temp, " RBP=");
    strcat(temp, sprintub16(temp2, task->state.rbp, 16));
    gfx_verbose_println(temp);

    //Print R8-R15
    temp[0] = 0;
    strcat(temp, "  R8=");
    strcat(temp, sprintub16(temp2, task->state.r8, 16));
    strcat(temp, " R9=");
    strcat(temp, sprintub16(temp2, task->state.r9, 16));
    strcat(temp, " R10=");
    strcat(temp, sprintub16(temp2, task->state.r10, 16));
    strcat(temp, " R11=");
    strcat(temp, sprintub16(temp2, task->state.r11, 16));
    strcat(temp, " R12=");
    strcat(temp, sprintub16(temp2, task->state.r12, 16));
    strcat(temp, " R13=");
    strcat(temp, sprintub16(temp2, task->state.r13, 16));
    strcat(temp, " R14=");
    strcat(temp, sprintub16(temp2, task->state.r14, 16));
    strcat(temp, " R15=");
    strcat(temp, sprintub16(temp2, task->state.r15, 16));
    gfx_verbose_println(temp);

    //Print CR3, RIP, RFLAGS
    temp[0] = 0;
    strcat(temp, "  CR3=");
    strcat(temp, sprintub16(temp2, task->state.cr3, 16));
    strcat(temp, " RIP=");
    strcat(temp, sprintub16(temp2, task->state.rip, 16));
    strcat(temp, " RFLAGS=");
    strcat(temp, sprintub16(temp2, task->state.rflags, 16));
    gfx_verbose_println(temp);

    //Print cycles
    temp[0] = 0;
    strcat(temp, "  SW_CNT=");
    strcat(temp, sprintub16(temp2, task->state.switch_cnt, 16));
    gfx_verbose_println(temp);
}

/*
 * Dumps all the information about the system on screen
 */
void krnl_dump(void){
    //Stop the schaeduler
    mtask_stop();

    gfx_verbose_println("---=== NEUTRON KERNEL DUMP ===---");

    gfx_verbose_println("TASKS:");
    //Scan through the task list
    task_t* tasks = mtask_get_task_list();
    for(uint32_t i = 0; i < MTASK_TASK_COUNT; i++){
        //If task at that index is valid
        if(tasks[i].valid){
            //Print its details
            char temp[200];
            temp[0] = 0;
            char temp2[20];
            strcat(temp, " ");
            strcat(temp, tasks[i].name);
            strcat(temp, ", UID ");
            strcat(temp, sprintu(temp2, tasks[i].uid, 1));
            if(tasks[i].uid == mtask_get_uid())
                strcat(temp, " [DUMP CAUSE]");
            if(tasks[i].state_code != TASK_STATE_RUNNING){
                strcat(temp, " [BLOCKED TILL ");
                strcat(temp, sprintub16(temp2, tasks[i].blocked_till, 16));
                strcat(temp, " / CUR ");
                strcat(temp, sprintub16(temp2, rdtsc(), 16));
                strcat(temp, "]");
            } else {
                strcat(temp, " [RUNNING]");
            }
            gfx_verbose_println(temp);
            krnl_dump_task_state(&tasks[i]);
            gfx_verbose_println("");
        }
    }
}

/*
 * Exception ISR
 */
void krnl_exc(void){
    //Get the exception address from RDX
    uint64_t ip;
    __asm__ volatile("mov %%rdx, %0" : "=m" (ip));
    //Get the exception code from RCX
    uint64_t code;
    __asm__ volatile("mov %%rcx, %0" : "=m" (code));
    //Get the extra exception data from RBX
    uint64_t data;
    __asm__ volatile("mov %%rbx, %0" : "=m" (data));
    //Stop the schaeduler
    mtask_stop();
    //Print some info
    krnl_dump();
    gfx_panic(ip, KRNL_PANIC_CPUEXC_CODE | (code << 8) | (data << 16));

    //Load 0xDEADBEEF into EAX for ease of debugging
    __asm__ volatile("mov $0xDEADBEEF, %eax");
    //Load exception address into RBX for ease of debugging too
    __asm__ volatile("mov %0, %%rbx" : : "m" (ip));
    //Abort
    while(1);
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
void dummy(void* args){
    while(1){
        dummy_var++;
        mtask_dly_cycles(100000000);
    }
}

/*
 * Multitasking entry point
 */
void mtask_entry(void* args){
    gfx_verbose_println("Hello from mtask_entry");
    //Load a simple program
    elf_load("/initrd/rax_counter.elf", 1);
    
    //Exit the entry point
    //mtask_stop_task(mtask_get_uid());
    while(1);
}

/*
 * Stack smashing detected
 */
__attribute__((noreturn)) void __stack_chk_fail(void) {
    gfx_panic(0, KRNL_PANIC_STACK_SMASH_CODE);
    while(1);
}

/*
 * What is THIS for? I have no idea :)
 */
void ___chkstk_ms(void){
    
}

/*
 * The entry point for the kernel
 */
EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable){
    //Set the stack smashing guard
    __asm__ ("rdrand %%eax; mov %%eax, %0" : "=m" (__stack_chk_guard) : : "eax");
    //Save the system table pointer
    krnl_efi_systable = SystemTable;
    //Disable the watchdog timer
    krnl_efi_systable->BootServices->SetWatchdogTimer(0, 0, 0, NULL);
    //Print the boot string
    krnl_efi_systable->ConOut->OutputString(SystemTable->ConOut, (CHAR16*)L"Neutron is starting up\r\n");

    //Using the loaded image protocol, find out where we are loaded in the memory
    EFI_LOADED_IMAGE_PROTOCOL* efi_lip = NULL;
    SystemTable->BootServices->HandleProtocol(ImageHandle, &(EFI_GUID)EFI_LOADED_IMAGE_PROTOCOL_GUID, (void**)&efi_lip);
    krnl_pos.offset = (uint64_t)efi_lip->ImageBase;
    krnl_pos.size = efi_lip->ImageSize;

	//Disable interrupts
	__asm__ volatile("cli");
    //Initialize x87 FPU
    __asm__ volatile("finit");
    //Do some initialization stuff
    uint64_t efi_map_key = dram_init();

    //Set verbose mode
    krnl_verbose = 1;
    gfx_set_verbose(krnl_verbose);

    //Do some graphics-related initialization stuff
    gfx_init();
    gfx_set_buf(GFX_BUF_SEC); //Enable doublebuffering
    gfx_fill(COLOR32(255, 0, 0, 0));
    gfx_set_font(jb_mono_10);

    gfx_verbose_println(KRNL_VERSION_STR);
    gfx_verbose_println("Verbose mode is on");
    gfx_verbose_println("Тестируем Юникод (testing Unicode)");
    gfx_verbose_println("!!!BEWARE!!! this is almost a coplete rewrite version of the system.");

    //Print CPU info
    gfx_verbose_println("CPU info:");
    char cpuid_buf[64];
    //Print vendor
    cpuid_get_vendor(cpuid_buf, NULL);
    gfx_verbose_println(cpuid_buf);
    //Print brand string
    cpuid_get_brand(cpuid_buf);
    gfx_verbose_println(cpuid_buf);

    //Check required CPU features
    uint32_t edx_feat, ecx_feat;
    cpuid_get_feat(&edx_feat, &ecx_feat);
    uint32_t cant_boot = 0;
    if(!(edx_feat & CPUID_FEAT_EDX_PAT))
        cant_boot |= 1;
    if(!(edx_feat & CPUID_FEAT_EDX_SYSCALL))
        cant_boot |= 2;
    if(cant_boot){
        //Darken the screen
        gfx_draw_filled_rect((p2d_t){0, 0}, (p2d_t){gfx_res_x(), gfx_res_y()}, COLOR32(128, 0, 0, 0));
        //Draw the boot error image
        gfx_draw_raw((p2d_t){(gfx_res_x() - 64) / 2, (gfx_res_y() - 64) / 2}, boot_err, (p2d_t){64, 64});
        //Print the error text
        char* error_text = "Unfortunately, Neutron can't be booted on this computer due to lack of requred CPU features:";
        p2d_t error_sz = gfx_text_bounds(error_text);
        gfx_puts((p2d_t){(gfx_res_x() - error_sz.x) / 2, (gfx_res_y() - 8) / 2 + 64}, COLOR32(255, 255, 255, 255), COLOR32(0, 0, 0, 0), error_text);
        //Construct the reason text
        char reason_text[256] = "";
        if(cant_boot & 1)
            strcat(reason_text, "PAT ");
        if(cant_boot & 2)
            strcat(reason_text, "SYSCALL ");
        //Print it
        p2d_t reason_sz = gfx_text_bounds(reason_text);
        gfx_puts((p2d_t){(gfx_res_x() - reason_sz.x) / 2, (gfx_res_y() - 8) / 2 + 74}, COLOR32(255, 255, 255, 255), COLOR32(0, 0, 0, 0), reason_text);
        //Draw the buffer
        gfx_flip();
        //Hang
        while(1);
    }

    //Set video buffer memory type
    vmem_pat_set_range(vmem_get_cr3(), gfx_buf_another(), gfx_buf_another() + (gfx_res_x() * gfx_res_y()), 1);

    //Print the kernel version
    if(!krnl_verbose)
        gfx_puts((p2d_t){.x = (gfx_res_x() - gfx_text_bounds(KRNL_VERSION_STR).x) / 2, .y = gfx_res_y() - 8},
                 COLOR32(255, 255, 255, 255), COLOR32(0, 0, 0, 0), KRNL_VERSION_STR);
    //Draw the logo
    gfx_draw_raw((p2d_t){.x = (gfx_res_x() - neutron_logo_width) / 2, .y = 50}, neutron_logo,
                 (p2d_t){.x = neutron_logo_width, .y = neutron_logo_height});
    //Print the boot process
    krnl_boot_status(">>> Starting the boot sequence <<<", 0);

    //Enable SSE instructions
    krnl_boot_status(">>> Enabling SSE <<<", 5);
    uint64_t sse_temp;
    __asm__ volatile("mov %%cr0, %0" : "=r" (sse_temp));
    sse_temp &= ~(1 << 2);
    sse_temp |=  (1 << 1);
    __asm__ volatile("mov %0, %%cr0" : : "r" (sse_temp));
    __asm__ volatile("mov %%cr4, %0" : "=r" (sse_temp));
    sse_temp |=  (1 << 9);
    sse_temp |=  (1 << 18);
    //sse_temp |=  (1 << 10);
    __asm__ volatile("mov %0, %%cr4" : : "r" (sse_temp));
    //Set extended control register
    uint64_t xcr0 = (1 << 0) | (1 << 1);
    __asm__ volatile("mov %0, %%ecx;"
                     "mov %1, %%edx;"
                     "mov %2, %%eax;"
                     "xsetbv" : : "r" ((uint32_t)0), "r" ((uint32_t)(xcr0 >> 32)), "r" ((uint32_t)xcr0) : "eax", "ecx", "edx");

    //Load INITRD
    krnl_boot_status(">>> Reading INITRD <<<", 10);
    uint8_t initrd_status = initrd_init();
    if(initrd_status != 0){
        gfx_verbose_println("INITRD read error");
        while(1);
    }
    //Mount INITRD
    diskio_init();
    diskio_mount((diskio_dev_t){.bus_type = DISKIO_BUS_INITRD, .device_no = 0}, "/initrd/");
    //Try to load the font
    gfx_verbose_println("Loading the Noto Sans Semibold font from INITRD");
    file_handle_t font_file;
    if(diskio_open("/initrd/noto-sans-semi-10.vlw", &font_file, DISKIO_FILE_ACCESS_READ) == DISKIO_STATUS_OK){
        uint8_t* font_buf = (uint8_t*)malloc(font_file.info.size);
        diskio_read(&font_file, font_buf, font_file.info.size);
        diskio_close(&font_file);
        gfx_set_font(font_buf);
    } else {
        gfx_verbose_println("Failed to load the font");
    }

    //Initialize ACPI
    krnl_boot_status(">>> Initializing ACPI <<<", 45);
    acpi_init();

    //Set up IDT
    krnl_boot_status(">>> Setting up interrupts <<<", 75);
    //Exit UEFI boot services
    SystemTable->BootServices->ExitBootServices(ImageHandle, efi_map_key);
	//Disable interrupts
	__asm__ volatile("cli");
    //Get the current code selector
    uint16_t cur_cs = 0;
    __asm__ volatile("movw %%cs, %0" : "=r" (cur_cs));
    //Set up IDT
    struct idt_entry* idt = (struct idt_entry*)calloc(256, sizeof(struct idt_entry));
    //Set up gates for exceptions
    idt[0] = IDT_ENTRY_ISR((uint64_t)&exc_0, cur_cs);
    idt[1] = IDT_ENTRY_ISR((uint64_t)&exc_1, cur_cs);
    idt[2] = IDT_ENTRY_ISR((uint64_t)&exc_2, cur_cs);
    idt[3] = IDT_ENTRY_ISR((uint64_t)&exc_3, cur_cs);
    idt[4] = IDT_ENTRY_ISR((uint64_t)&exc_4, cur_cs);
    idt[5] = IDT_ENTRY_ISR((uint64_t)&exc_5, cur_cs);
    idt[6] = IDT_ENTRY_ISR((uint64_t)&exc_6, cur_cs);
    idt[7] = IDT_ENTRY_ISR((uint64_t)&exc_7, cur_cs);
    idt[8] = IDT_ENTRY_ISR((uint64_t)&exc_8, cur_cs);
    idt[9] = IDT_ENTRY_ISR((uint64_t)&exc_9, cur_cs);
    idt[10] = IDT_ENTRY_ISR((uint64_t)&exc_10, cur_cs);
    idt[11] = IDT_ENTRY_ISR((uint64_t)&exc_11, cur_cs);
    idt[12] = IDT_ENTRY_ISR((uint64_t)&exc_12, cur_cs);
    idt[13] = IDT_ENTRY_ISR((uint64_t)&exc_13, cur_cs);
    idt[14] = IDT_ENTRY_ISR((uint64_t)&exc_14, cur_cs);
    idt[16] = IDT_ENTRY_ISR((uint64_t)&exc_16, cur_cs);
    idt[17] = IDT_ENTRY_ISR((uint64_t)&exc_17, cur_cs);
    idt[18] = IDT_ENTRY_ISR((uint64_t)&exc_18, cur_cs);
    idt[19] = IDT_ENTRY_ISR((uint64_t)&exc_19, cur_cs);
    idt[20] = IDT_ENTRY_ISR((uint64_t)&exc_20, cur_cs);
    idt[30] = IDT_ENTRY_ISR((uint64_t)&exc_30, cur_cs);
    //Set up gates for interrupts
    idt[32] = IDT_ENTRY_ISR((uint64_t)(&apic_timer_isr_wrap - krnl_pos.offset) | 0xFFFF800000000000ULL, cur_cs);
    //Load IDT
    idt_d.base = (void*)idt;
    idt_d.limit = 256 * sizeof(struct idt_entry);
    load_idt(&idt_d);

    //Initialize the APIC
    krnl_boot_status(">>> Initializing APIC <<<", 98);
    apic_init();

    //Shift the DRAM region
    vmem_init();
    dram_shift();

    //Initialize the multitasking system
    krnl_boot_status(">>> Initializing multitasking <<<", 99);
    mtask_init();

    //The loading process is done!
    krnl_boot_status(">>> Done <<<", 100);
    
    //Create a virtual memory space
    uint64_t cr3 = vmem_create_pml4(vmem_create_pcid());
    //Map the kernel in the upper half
    {
        //But first, print the debug info about where we are
        char temp[128] = "Kernel loaded at 0x";
        char temp2[64];
        char temp3[64];
        gfx_verbose_println(strcat(strcat(strcat(temp, sprintub16(temp2, krnl_pos.offset, 1)), " / size 0x"), sprintub16(temp3, krnl_pos.size, 1)));
    }
    //Map the kernel in the upper half
    vmem_map(cr3, (void*)krnl_pos.offset, (void*)(krnl_pos.offset + krnl_pos.size), (void*)(0xFFFF800000000000ULL));
    //Identity map the first 8GB of the memory
    vmem_map(cr3, 0, (phys_addr_t)(8ULL * 1024 * 1024 * 1024), 0);
    mtask_create_task(16384, "Multitasking bootstrapper", 10, 0, cr3, NULL, 1, (void(*)(void*))((uint64_t)(mtask_entry - krnl_pos.offset) | 0xFFFF800000000000ULL), NULL);
    while(1);
}
