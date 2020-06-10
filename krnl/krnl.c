//Neutron Project
//The main file

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
#include "./drivers/acpi.h"
#include "./drivers/initrd.h"
#include "./drivers/ps2.h"
#include "./drivers/cmos.h"

#include "./fonts/jb-mono-10.h"

#include "./images/neutron_logo.h"
#include "./images/boot_err.h"

#include "./mtask/mtask.h"

#include "./vmem/vmem.h"

#include "./app_drv/elf/elf.h"
#include "./app_drv/syscall/syscall.h"

//ISR wrappers
extern void apic_timer_isr_wrap(void);
extern void ps21_isr_wrap(void);
extern void ps22_isr_wrap(void);
extern void rtc_isr_wrap(void);

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
uint16_t krnl_cs = 0;
uint16_t krnl_ds = 0;
//First and last kernel message pointers
krnl_msg_t* first_msg;
krnl_msg_t* last_msg;
//Stack Smashing Protection guard
uint64_t __stack_chk_guard;
//Is the kernel in verbose mode or not?
uint8_t krnl_verbose = 0;
//EFI stuff
EFI_SYSTEM_TABLE* krnl_efi_systable;
EFI_HANDLE krnl_efi_img_handle;
uint64_t krnl_efi_map_key;

/*
 * Prints a kernel message in verbose mode
 */
void krnl_print_msg(krnl_msg_t* m){
    char buf[MAX_KRNL_MSG_SZ + 256];
    memset(buf, 0, sizeof(buf));
    sprintf(buf, "[%i ms] %s: %s", 1000 * (m->tsc - first_msg->tsc) / timr_get_cpu_fq(), m->file, m->msg);
    gfx_verbose_println(buf);
}

/*
 * Writes a message to the kernel message buffer
 */
void krnl_write_msg(char* file, char* msg){
    //Print the message
    //Allocate memory for the message
    krnl_msg_t* m = (krnl_msg_t*)calloc(1, sizeof(krnl_msg_t));
    //Copy filename and message
    strcpy(m->msg, msg);
    strcpy(m->file, file);
    //Read timestamp counter
    m->tsc = rdtsc();
    //Assign the pointer to the previous message
    if(last_msg == NULL){
        first_msg = m;
        last_msg = m;
    } else {
        last_msg->next = m;
        last_msg = m;
    }

    if(krnl_verbose)
        krnl_print_msg(m);
}

/*
 * Writes a formatted message to the kernel message buffer
 */
void krnl_write_msgf(char* file, char* msg, ...){
    //Formatted print to a string
    char buf[MAX_KRNL_MSG_SZ];
    va_list valist;
    va_start(valist, _sprintf_argcnt(msg));
    _sprintf(buf, msg, valist);
    krnl_write_msg(file, buf);
    va_end(valist);
}

/*
 * Prints a formatted message to the UEFI ConsoleOut protocol
 */
void krnl_writec_f(char* msg, ...){
    //Print the formatted message
    char buf[1024];
    va_list valist;
    va_start(valist, _sprintf_argcnt(msg));
    _sprintf(buf, msg, valist);
    //Expand the buffer, becuase UEFI uses retarded 16-bit characters
    for(int i = 1023; i >= 1; i -= 2){
        buf[i] = buf[i / 2];
        buf[i / 2] = 0;
    }
    //Print the buffer
    krnl_efi_systable->ConOut->OutputString(krnl_efi_systable->ConOut, (CHAR16*)(buf + 1));
    va_end(valist);
}

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
 * Converts exception vector to its corresponding name
 */
char* krnl_exc_vect_to_str(uint8_t vect){
    switch(vect){
        case 0:
            return "division by zero exception";
        case 1:
            return "debug trap";
        case 2:
            return "NMI";
        case 3:
            return "breakpoint excpetion";
        case 4:
            return "overflow excpetion";
        case 5:
            return "bound range exceeded exception";
        case 6:
            return "invalid opcode exception";
        case 7:
            return "FPU not available exception";
        case 8:
            return "double fault";
        case 9:
            return "coprocessor segment overrun";
        case 10:
            return "invalid TSS exception";
        case 11:
            return "segment not present exception";
        case 12:
            return "stack-segment fault";
        case 13:
            return "general protection fault";
        case 14:
            return "page fault";
        case 16:
            return "FPU exception";
        case 17:
            return "alignment check exception";
        case 18:
            return "machine check exception";
        case 19:
            return "SIMD exception";
        case 20:
            return "virtualization exception";
        case 30:
            return "security exception";
        case 255:
            return "[no exception]";
        default:
            return "[reserved exception number]";
    }
}

/*
 * Dumps the task state on screen
 */
void krnl_dump_task_state(task_t* task){
    char temp[500];
    temp[0] = 0;
    char temp2[20];

    //Print RAX-RBX, RSI, RDI, RSP, RBP
    strcat(temp, "RAX=");
    strcat(temp, sprintub16(temp2, task->state.rax, 1));
    strcat(temp, " RBX=");
    strcat(temp, sprintub16(temp2, task->state.rbx, 1));
    strcat(temp, " RCX=");
    strcat(temp, sprintub16(temp2, task->state.rcx, 1));
    strcat(temp, " RDX=");
    strcat(temp, sprintub16(temp2, task->state.rdx, 1));
    strcat(temp, " RSI=");
    strcat(temp, sprintub16(temp2, task->state.rsi, 1));
    strcat(temp, " RDI=");
    strcat(temp, sprintub16(temp2, task->state.rdi, 1));
    strcat(temp, " RSP=");
    strcat(temp, sprintub16(temp2, task->state.rsp, 1));
    strcat(temp, " RBP=");
    strcat(temp, sprintub16(temp2, task->state.rbp, 1));
    krnl_write_msg(__FILE__, temp);

    //Print R8-R15
    temp[0] = 0;
    strcat(temp, "R8=");
    strcat(temp, sprintub16(temp2, task->state.r8, 1));
    strcat(temp, " R9=");
    strcat(temp, sprintub16(temp2, task->state.r9, 1));
    strcat(temp, " R10=");
    strcat(temp, sprintub16(temp2, task->state.r10, 1));
    strcat(temp, " R11=");
    strcat(temp, sprintub16(temp2, task->state.r11, 1));
    strcat(temp, " R12=");
    strcat(temp, sprintub16(temp2, task->state.r12, 1));
    strcat(temp, " R13=");
    strcat(temp, sprintub16(temp2, task->state.r13, 1));
    strcat(temp, " R14=");
    strcat(temp, sprintub16(temp2, task->state.r14, 1));
    strcat(temp, " R15=");
    strcat(temp, sprintub16(temp2, task->state.r15, 1));
    krnl_write_msg(__FILE__, temp);

    //Print CR3, RIP, RFLAGS
    temp[0] = 0;
    strcat(temp, "CR3=");
    strcat(temp, sprintub16(temp2, task->state.cr3, 1));
    strcat(temp, " RIP=");
    strcat(temp, sprintub16(temp2, task->state.rip, 1));
    strcat(temp, " RFL=");
    strcat(temp, sprintub16(temp2, task->state.rflags, 1));
    strcat(temp, " CS=");
    strcat(temp, sprintub16(temp2, task->state.cs, 1));
    krnl_write_msg(__FILE__, temp);

    //Print exception vector
    temp[0] = 0;
    strcat(temp, "EXC_VECT=");
    strcat(temp, sprintub16(temp2, task->state.exc_vector, 2));
    strcat(temp, " (");
    strcat(temp, krnl_exc_vect_to_str(task->state.exc_vector));
    strcat(temp, ")");
    krnl_write_msg(__FILE__, temp);

    //Print SIMD exception information
    if(task->state.exc_vector == 0x13){
        uint32_t mxcsr = 0;
        asm("stmxcsr %0" : "=m"(mxcsr));
        temp[0] = 0;
        strcat(temp, "MXCSR=");
        strcat(temp, sprintub16(temp2, mxcsr, 8));
        krnl_write_msg(__FILE__, temp);
    }
    //Print PF information
    if(task->state.exc_vector == 0x0E){
        uint64_t cr2 = 0;
        __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));
        temp[0] = 0;
        strcat(temp, "CR2=");
        strcat(temp, sprintub16(temp2, cr2, 1));
        krnl_write_msg(__FILE__, temp);
    }
}

/*
 * Dumps all the information about the system on screen
 */
void krnl_dump(void){
    //Stop the schaeduler
    mtask_stop();

    krnl_write_msg(__FILE__, "full kernel dump:");

    krnl_write_msg(__FILE__, "tasks:");
    //Scan through the task list
    task_t* tasks = mtask_get_task_list();
    for(uint32_t i = 0; i < MTASK_TASK_COUNT; i++){
        //If task at that index is valid
        if(tasks[i].valid){
            //Print its details
            char temp[200];
            temp[0] = 0;
            char temp2[20];
            strcat(temp, tasks[i].name);
            strcat(temp, ", PID ");
            strcat(temp, sprintu(temp2, tasks[i].pid, 1));
            if(tasks[i].pid == mtask_get_pid())
                strcat(temp, " [running at dump]");
            if(tasks[i].state_code != TASK_STATE_RUNNING){
                strcat(temp, " [blocked till cycle ");
                strcat(temp, sprintub16(temp2, tasks[i].blocked_till, 1));
                strcat(temp, " / current ");
                strcat(temp, sprintub16(temp2, rdtsc(), 1));
                strcat(temp, "]");
            }
            krnl_write_msg(__FILE__, temp);
            krnl_dump_task_state(&tasks[i]);
            krnl_write_msg(__FILE__, "");
        }
    }
}

/*
 * Exception ISR
 */
void krnl_exc(void){
    //Get the task that caused the exception
    task_t* task = mtask_get_cur_task();
    //If that task was running in userspace
    if(task->state.cs != krnl_cs){
        //Print the exception info
        char symbol[256];
        elf_get_sym(task, task->state.rip, symbol);
        krnl_write_msgf(__FILE__, "Task %s with PID %i caused %s at RIP=0x%x <%s>",
            task->name, task->pid, krnl_exc_vect_to_str(task->state.exc_vector), task->state.rip, symbol);
        krnl_write_msg(__FILE__, "Task state at exception:");
        krnl_dump_task_state(task);
        //Stop that task
        mtask_stop_task(task->pid);
        while(1);
    }
    //Otherwise, the exception happened in kernel-space
    //Panic! in the Kernel
    mtask_stop();
    krnl_dump();
    gfx_panic(task->state.rip, KRNL_PANIC_CPUEXC_CODE);
}

/*
 * Display a boot progress bar
 */
void krnl_boot_status(char* str, uint32_t progress){
    //Only if we're not in verbose mode
    if(krnl_verbose)
        return;
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

/*
 * Stack smashing detected
 */
__attribute__((noreturn)) void __stack_chk_fail(void) {
    krnl_dump();
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
	//Disable interrupts
	__asm__ volatile("cli");
    //Set the stack smashing guard
    __asm__ ("rdrand %%eax; mov %%eax, %0" : "=m" (__stack_chk_guard) : : "eax");
    //Save the system table pointer
    krnl_efi_systable = SystemTable;
    krnl_efi_img_handle = ImageHandle;
    //Disable the watchdog timer
    krnl_efi_systable->BootServices->SetWatchdogTimer(0, 0, 0, NULL);
    //Print the boot string
    krnl_writec_f("Neutron version %s\r\n", KRNL_VERSION_STR);

    //Using the loaded image protocol, find out where we are loaded in the memory
    EFI_LOADED_IMAGE_PROTOCOL* efi_lip = NULL;
    SystemTable->BootServices->HandleProtocol(ImageHandle, &(EFI_GUID)EFI_LOADED_IMAGE_PROTOCOL_GUID, (void**)&efi_lip);
    krnl_pos.offset = (uint64_t)efi_lip->ImageBase;
    krnl_pos.size = efi_lip->ImageSize;
    krnl_writec_f("Loaded at 0x%x, size 0x%x\r\n", krnl_pos.offset, krnl_pos.size);

    //Do some initialization stuff
    krnl_efi_map_key = dram_init();
    vmem_init();
    dram_shift();

    krnl_write_msgf(__FILE__, "Neutron kernel version %s (%i), compiled on %s %s",
                              KRNL_VERSION_STR, KRNL_VERSION_NUM, __DATE__, __TIME__);
    krnl_write_msgf(__FILE__, "load address/size: 0x%x/0x%x", krnl_pos.offset, krnl_pos.size);
    krnl_write_msgf(__FILE__, "dynamic memory physical base: 0x%x", stdlib_physbase());

    //Measure CPU frequency
    timr_measure_cpu_fq();

    //Do some graphics-related initialization stuff
    gfx_init();
    gfx_set_buf(GFX_BUF_SEC); //Enable doublebuffering
    gfx_fill(COLOR32(255, 0, 0, 0));
    gfx_set_font(jb_mono_11);

    //Set verbose mode
    krnl_verbose = 1;
    gfx_set_verbose(krnl_verbose);
    
    //Draw the logo
    gfx_draw_raw((p2d_t){.x = (gfx_res_x() - neutron_logo_width) / 2, .y = 50}, neutron_logo,
                 (p2d_t){.x = neutron_logo_width, .y = neutron_logo_height});

    //Map default regions
    vmem_map_defaults(vmem_get_cr3());
    gfx_shift_buf();

    //Print all kernel messages that occured before graphics initialization
    {
        krnl_msg_t* msg = first_msg;
        do
            krnl_print_msg(msg);
        while((msg = msg->next));
    }

    krnl_write_msgf(__FILE__, "mapped default regions to upper half");

    //Print CPU info
    char cpuid_buf[64];
    //Print vendor
    cpuid_get_vendor(cpuid_buf, NULL);
    krnl_write_msgf(__FILE__, "cpu vendor: %s", cpuid_buf);
    //Print brand string
    cpuid_get_brand(cpuid_buf);
    krnl_write_msgf(__FILE__, "cpu name: %s", cpuid_buf);

    //Check required CPU features
    uint32_t edx_feat, ecx_feat;
    cpuid_get_feat(&edx_feat, &ecx_feat);
    krnl_write_msgf(__FILE__, "cpu features: edx: 0x%x, ecx: 0x%x", edx_feat, ecx_feat);
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
    
    //Print the boot process
    krnl_boot_status("Starting", 0);

    //Enable SSE instructions
    krnl_boot_status("Enabling SSE", 5);
    uint64_t sse_temp;
    __asm__ volatile("mov %%cr0, %0" : "=r" (sse_temp));
    sse_temp &= ~(1 << 2);
    sse_temp |=  (1 << 1);
    __asm__ volatile("mov %0, %%cr0" : : "r" (sse_temp));
    __asm__ volatile("mov %%cr4, %0" : "=r" (sse_temp));
    sse_temp |=  (1 << 9);
    sse_temp |=  (1 << 10);
    sse_temp |=  (1 << 18);
    __asm__ volatile("mov %0, %%cr4" : : "r" (sse_temp));
    __asm__ volatile("stmxcsr %0" : "=m"(sse_temp));
    sse_temp |= 0xFC0;
    __asm__ volatile("ldmxcsr %0" : : "m"(sse_temp));
    //Set extended control register
    uint64_t xcr0 = (1 << 0) | (1 << 1);
    __asm__ volatile("mov %0, %%ecx;"
                     "mov %1, %%edx;"
                     "mov %2, %%eax;"
                     "xsetbv" : : "r" ((uint32_t)0), "r" ((uint32_t)(xcr0 >> 32)), "r" ((uint32_t)xcr0) : "eax", "ecx", "edx");

    //Load INITRD
    krnl_boot_status("Reading INITRD", 10);
    initrd_init();
    diskio_init();
    diskio_mount((diskio_dev_t){.bus_type = DISKIO_BUS_INITRD, .device_no = 0}, "/initrd/");
    /*
    //Try to load the font
    file_handle_t font_file;
    if(diskio_open("/initrd/bitstream-mono-10.vlw", &font_file, DISKIO_FILE_ACCESS_READ) == DISKIO_STATUS_OK){
        uint8_t* font_buf = (uint8_t*)malloc(font_file.info.size);
        diskio_read(&font_file, font_buf, font_file.info.size);
        diskio_close(&font_file);
        gfx_set_font(font_buf);
    }
    */

    //Initialize ACPI
    krnl_boot_status("Initializing ACPI", 45);
    acpi_init();

    //Set up IDT
    krnl_boot_status("Setting up interrupts", 75);
    //Exit UEFI boot services
    krnl_get_efi_systable()->BootServices->ExitBootServices(krnl_efi_img_handle, krnl_efi_map_key);
    krnl_write_msgf(__FILE__, "exited UEFI boot services");
	//Disable interrupts
	__asm__ volatile("cli");
    //Get the current code and data selectors
    krnl_cs = 0;
    krnl_ds = 0;
    __asm__ volatile("movw %%cs, %0" : "=r" (krnl_cs));
    __asm__ volatile("movw %%ds, %0" : "=r" (krnl_ds));
    krnl_write_msgf(__FILE__, "current selectors: cs: 0x%x, ds: 0x%x", krnl_cs, krnl_ds);
    //Set up IDT
    idt_entry_t* idt = (idt_entry_t*)calloc(256, sizeof(idt_entry_t));
    idt_desc_t idt_d;
    //Set up gates for exceptions
    idt[0] = IDT_ENTRY_ISR((uint64_t)(&exc_0 - krnl_pos.offset) | 0xFFFF800000000000ULL, krnl_cs);
    idt[1] = IDT_ENTRY_ISR((uint64_t)(&exc_1 - krnl_pos.offset) | 0xFFFF800000000000ULL, krnl_cs);
    idt[2] = IDT_ENTRY_ISR((uint64_t)(&exc_2 - krnl_pos.offset) | 0xFFFF800000000000ULL, krnl_cs);
    idt[3] = IDT_ENTRY_ISR((uint64_t)(&exc_3 - krnl_pos.offset) | 0xFFFF800000000000ULL, krnl_cs);
    idt[4] = IDT_ENTRY_ISR((uint64_t)(&exc_4 - krnl_pos.offset) | 0xFFFF800000000000ULL, krnl_cs);
    idt[5] = IDT_ENTRY_ISR((uint64_t)(&exc_5 - krnl_pos.offset) | 0xFFFF800000000000ULL, krnl_cs);
    idt[6] = IDT_ENTRY_ISR((uint64_t)(&exc_6 - krnl_pos.offset) | 0xFFFF800000000000ULL, krnl_cs);
    idt[7] = IDT_ENTRY_ISR((uint64_t)(&exc_7 - krnl_pos.offset) | 0xFFFF800000000000ULL, krnl_cs);
    idt[8] = IDT_ENTRY_ISR((uint64_t)(&exc_8 - krnl_pos.offset) | 0xFFFF800000000000ULL, krnl_cs);
    idt[9] = IDT_ENTRY_ISR((uint64_t)(&exc_9 - krnl_pos.offset) | 0xFFFF800000000000ULL, krnl_cs);
    idt[10] = IDT_ENTRY_ISR((uint64_t)(&exc_10 - krnl_pos.offset) | 0xFFFF800000000000ULL, krnl_cs);
    idt[11] = IDT_ENTRY_ISR((uint64_t)(&exc_11 - krnl_pos.offset) | 0xFFFF800000000000ULL, krnl_cs);
    idt[12] = IDT_ENTRY_ISR((uint64_t)(&exc_12 - krnl_pos.offset) | 0xFFFF800000000000ULL, krnl_cs);
    idt[13] = IDT_ENTRY_ISR((uint64_t)(&exc_13 - krnl_pos.offset) | 0xFFFF800000000000ULL, krnl_cs);
    idt[14] = IDT_ENTRY_ISR((uint64_t)(&exc_14 - krnl_pos.offset) | 0xFFFF800000000000ULL, krnl_cs);
    idt[16] = IDT_ENTRY_ISR((uint64_t)(&exc_16 - krnl_pos.offset) | 0xFFFF800000000000ULL, krnl_cs);
    idt[17] = IDT_ENTRY_ISR((uint64_t)(&exc_17 - krnl_pos.offset) | 0xFFFF800000000000ULL, krnl_cs);
    idt[18] = IDT_ENTRY_ISR((uint64_t)(&exc_18 - krnl_pos.offset) | 0xFFFF800000000000ULL, krnl_cs);
    idt[19] = IDT_ENTRY_ISR((uint64_t)(&exc_19 - krnl_pos.offset) | 0xFFFF800000000000ULL, krnl_cs);
    idt[20] = IDT_ENTRY_ISR((uint64_t)(&exc_20 - krnl_pos.offset) | 0xFFFF800000000000ULL, krnl_cs);
    idt[30] = IDT_ENTRY_ISR((uint64_t)(&exc_30 - krnl_pos.offset) | 0xFFFF800000000000ULL, krnl_cs);
    //Set up gates for interrupts
    idt[32] = IDT_ENTRY_ISR((uint64_t)(&apic_timer_isr_wrap - krnl_pos.offset) | 0xFFFF800000000000ULL, krnl_cs);
    idt[33] = IDT_ENTRY_ISR((uint64_t)(&ps21_isr_wrap - krnl_pos.offset) | 0xFFFF800000000000ULL, krnl_cs);
    idt[34] = IDT_ENTRY_ISR((uint64_t)(&ps22_isr_wrap - krnl_pos.offset) | 0xFFFF800000000000ULL, krnl_cs);
    idt[35] = IDT_ENTRY_ISR((uint64_t)(&rtc_isr_wrap - krnl_pos.offset) | 0xFFFF800000000000ULL, krnl_cs);
    //Load IDT
    idt_d.base = (void*)idt;
    idt_d.limit = 256 * sizeof(idt_entry_t);
    load_idt(&idt_d);
    krnl_write_msgf(__FILE__, "loaded IDT");

    //Move the GDT, adding userspace descriptors
    gdt_desc_t gdt_d;
    __asm__ volatile("sgdt %0" : : "m"(gdt_d));
    uint64_t* new_gdt = malloc(65536);
    memcpy(new_gdt, gdt_d.base, gdt_d.limit);
    uint16_t user_cs = 0x90;
    uint16_t user_ds = 0x88;
    new_gdt[krnl_cs / 8 + 1] = new_gdt[krnl_ds / 8];
    new_gdt[user_cs / 8] = new_gdt[krnl_cs / 8];
    new_gdt[user_ds / 8] = new_gdt[krnl_ds / 8];
    new_gdt[user_cs / 8] |= 3ULL << 45; //set privilege level to 3
    new_gdt[user_ds / 8] |= 3ULL << 45;
    //Set up the task state segment and its descriptor
    uint16_t tsss = 0x100;
    tss_t* tss = calloc(1, sizeof(tss_t));
    tss->rsp0 = (uint64_t)malloc(16384) + 16384;
    uint64_t tss_addr = (uint64_t)tss;
    uint64_t tss_size = sizeof(tss_t);
    uint64_t tss_desc_hi =   tss_addr               >> 32;   //Limit and base
    uint64_t tss_desc_lo =  (tss_size & 0xFFFF)            |
                           ((tss_addr & 0xFFFF)     << 16) |
                           ((tss_addr >> 16 & 0xFF) << 32) |
                           ((tss_size >> 16 & 0xF ) << 48) |
                           ((tss_addr >> 24 & 0xF ) << 56) |
                           (0b0000000011101001ULL   << 40);  //Access and flags
                            //G--A----PDD-TYPE
    new_gdt[tsss / 8]     = tss_desc_lo;
    new_gdt[tsss / 8 + 1] = tss_desc_hi;
    //Load the new GDT
    gdt_d.base = new_gdt;
    gdt_d.limit = 65535;
    __asm__ volatile("lgdt %0" : : "m"(gdt_d));
    krnl_write_msgf(__FILE__, "loaded GDT");
    //Load TR
    __asm__ volatile("ltr %0" : : "r"(tsss));
    krnl_write_msgf(__FILE__, "loaded TR");

    //Set the system call stuff
    wrmsr(MSR_IA32_EFER, (rdmsr(MSR_IA32_EFER) & ~(0xFFFFFULL << 45)) | 1);
    wrmsr(MSR_IA32_LSTAR, (uint64_t)(&syscall_wrapper - krnl_pos.offset) | 0xFFFF800000000000ULL);
    wrmsr(MSR_IA32_SFMASK, 1 << 9); //Disable interrupts on syscalls
    wrmsr(MSR_IA32_STAR, ((uint64_t)krnl_cs << 32) | ((uint64_t)(user_cs - 16) << 48));
    krnl_write_msgf(__FILE__, "initialized SYSCALL instr");

    //Initialize the APIC
    krnl_boot_status("Initializing APIC", 80);
    apic_init();

    //Initialize... stuff...
    krnl_boot_status("Initializing some hardware", 85);
    ps2_init();
    rtc_init();

    //Initialize the multitasking system
    krnl_boot_status("Initializing multitasking", 90);
    mtask_init();

    //The loading process is done!
    krnl_boot_status("Done", 100);

    //Okay. This is jank level 100 here.
    //THIS way of doing things is BAD. Nevermand in an operating system kernel!
    //I really hope this is a temporary fix.

    //RETARDED RETARDED RETARDED     VVVV     RETARDED RETARDED RETARDED

    //Scan through the loaded kernel image and find integers values of which belong to this range.
    //Replace them with their upper-half variants.
    //    (basically, this is poor man's relocation)
    //    (please, forgive me. I wasn't able to tell my compiler not to
    //     generate code with these .refptr. references :< )
    uint64_t orig_pos = krnl_pos.offset;
    for(uint64_t o = orig_pos; o <= orig_pos + krnl_pos.size - 8; o++){
        uint64_t* ptr = (uint64_t*)o;
        uint64_t val = *ptr;
        if(val >= orig_pos && val <= orig_pos + krnl_pos.size){
            val = (val - orig_pos) | 0xFFFF800000000000ULL;
            *ptr = val;
        }
    }
    krnl_pos.offset = orig_pos;

    krnl_write_msgf(__FILE__, "finished \"relocating\"");

    //Run the initialization task
    syscall_init();
    krnl_write_msgf(__FILE__, "running init");
    uint64_t status = elf_load("/initrd/init.elf", TASK_PRIVL_EVERYTHING, 2);
    if(status != ELF_STATUS_OK)
        krnl_write_msgf(__FILE__, "running init failed: error code %i", status);
    while(1);
}
