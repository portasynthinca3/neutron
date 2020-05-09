//Neutron project
//System call support

#include "./syscall.h"
#include "../../krnl.h"
#include "../../stdlib.h"
#include "../../vmem/vmem.h"
#include "../../mtask/mtask.h"
#include "../../drivers/gfx.h"
#include "../elf/elf.h"

/*
 * Handles a system call
 */
uint64_t handle_syscall(void){
    uint64_t num, p0, p1, p2, p3, p4;
    asm volatile("mov %%rdi, %0;"
                 "mov %%rsi, %1;"
                 "mov %%rdx, %2;"
                 "mov %%rcx, %3;"
                 "mov %%r8,  %4;"
                 "mov %%r9,  %5;" :
                 "=m"(num), "=m"(p0), "=m"(p1), "=m"(p2), "=m"(p3), "=m"(p4));
    switch(num){
        case 0: //print a string in verbose mode
            gfx_verbose_println((char*)p0);
            return 0;
        case 1: //terminate the current application
            mtask_stop_task(mtask_get_uid());
            return 0;
        case 2: //launch another application
            elf_load((char*)p0, 0);
            return 0;
        default:
            gfx_panic(0, KRNL_PANIC_INVL_SYSCALL_CODE);
            break;
    }
}