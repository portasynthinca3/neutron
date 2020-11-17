//Neutron project
//System call support

#include "./syscall.h"
#include "../../krnl.h"
#include "../../stdlib.h"
#include "../../vmem/vmem.h"
#include "../../mtask/mtask.h"
#include "../../drivers/gfx.h"
#include "../../drivers/disk/diskio.h"
#include "../../krnl.h"
#include "../elf/elf.h"

//Kernel mode RSP
uint64_t krnl_rsp = 0;

void syscall_init(void){
    //Create a kernel mode stack
    krnl_rsp = (uint64_t)malloc(8192) + 8192;
    krnl_write_msgf(__FILE__, __LINE__, "system call RSP: 0x%x", krnl_rsp);
    //Self-modify the syscall wrapper offset
    krnl_write_msgf(__FILE__, __LINE__, "mov rsp modify address: 0x%x", &syscall_wrapper + 5);
    *(uint64_t volatile*)(&syscall_wrapper + 5) = krnl_rsp;
}

uint64_t syscall_get_krnl_rsp(void){
    return krnl_rsp;
}

/*
 * Handles a system call
 */
uint64_t syscall_handle(void){
    //Get syscall function/subfunction numbers and arguments
    uint64_t num, p0, p1, p2, p3, p4;
    asm volatile("mov %%rdi, %0;"
                 "mov %%rsi, %1;"
                 "mov %%rdx, %2;"
                 "mov %%r8,  %3;"
                 "mov %%r9,  %4;"
                 "mov %%r10, %5;" :
                 "=m"(num), "=m"(p0), "=m"(p1), "=m"(p2), "=m"(p3), "=m"(p4));
    //Function number is in higher 32 bits
    uint32_t func = num >> 32;
    //Subfunction number is in lower 32 bits
    uint32_t subfunc = (uint32_t)num;
    switch(func){
        case 1: //task management
            switch(subfunc){
                case 0: //get task UID
                    return mtask_get_pid();
                case 1: //terminate task
                    mtask_stop_task(p0);
                    return 0;
                case 2: //load executable
                    //check pointer (should be in userspace)
                    if(p0 + strlen((char*)p0) >= 0x800000000000ULL)
                        return 0xFFFFFFFFFFFFFFFF;
                    //Inherit the privileges if requested
                    if(p1 & TASK_PRIVL_INHERIT)
                        p1 = mtask_get_by_pid(mtask_get_pid())->privl & ~TASK_PRIVL_SUDO_MODE;
                    return elf_load((char*)p0, p1, mtask_get_by_pid(mtask_get_pid())->priority);
                case 3: //allocate pages
                    return (uint64_t)mtask_palloc(mtask_get_pid(), p0);
                case 4: //free pages
                    mtask_pfree(mtask_get_pid(), (virt_addr_t)p0);
                    return 0;
                default: //invalid subfunction number
                    return 0xFFFFFFFFFFFFFFFF;
            }
        case 2: //filesystem
            switch(subfunc){
                case 0: { //open file
                    //check path pointer (should be in userspace)
                    if(p0 + strlen((char*)p0) >= 0x800000000000ULL)
                        return 0xFFFFFFFFFFFFFFFF;
                    //Try to open the file
                    file_handle_t* handle = (file_handle_t*)malloc(sizeof(file_handle_t));
                    uint64_t status = diskio_open((char*)p0, handle, p1);
                    //Parse status
                    switch(status){
                        case DISKIO_STATUS_OK: {
                            //Find the handle in the process's handle list
                            uint64_t i = 0;
                            task_t* task = mtask_get_by_pid(mtask_get_pid());
                            for(i = 0; i < MTASK_MAX_OPEN_FILES; i++)
                                if(task->open_files[i] == handle)
                                    break;
                            //Return the number
                            return i + 0xFF;
                        }
                        case DISKIO_STATUS_WRITE_PROTECTED:
                            return 2;
                        case DISKIO_STATUS_FILE_NOT_FOUND:
                            return 1;
                    }
                }
                case 1: { //read bytes
                    //check buffer pointer (should be in userspace)
                    if(p1 + p2 >= 0x800000000000ULL)
                        return 0xFFFFFFFFFFFFFFFF;
                    //try to read the data
                    uint64_t status = diskio_read(mtask_get_by_pid(mtask_get_pid())->open_files[p0 - 0xFF], (void*)p1, p2);
                    //Parse status
                    switch(status & 0xFF){
                        case DISKIO_STATUS_NOT_ALLOWED:
                            return 4ULL << 32;
                        case DISKIO_STATUS_EOF:
                            return (6ULL << 32) | (status >> 32);
                        case DISKIO_STATUS_OK:
                            return p2;
                    }
                }
                case 2: { //write bytes
                    //check buffer pointer (should be in userspace)
                    if(p1 + p2 >= 0x800000000000ULL)
                        return 0xFFFFFFFFFFFFFFFF;
                    //try to write the data
                    uint64_t status = diskio_write(mtask_get_by_pid(mtask_get_pid())->open_files[p0 - 0xFF], (void*)p1, p2);
                    //Parse status
                    switch(status & 0xFF){
                        case DISKIO_STATUS_NOT_ALLOWED:
                            return 4ULL << 32;
                        case DISKIO_STATUS_EOF:
                            return (6ULL << 32) | (status >> 32);
                        case DISKIO_STATUS_OK:
                            return 0;
                    }
                }
                case 3: { //seek
                    return diskio_seek(mtask_get_by_pid(mtask_get_pid())->open_files[p0 - 0xFF], p1);
                    return DISKIO_STATUS_OK;
                }
                case 4: { //close file
                    diskio_close(mtask_get_by_pid(mtask_get_pid())->open_files[p0 - 0xFF]);
                    return DISKIO_STATUS_OK;
                }
                default: //invalid subfunction number
                    return 0xFFFFFFFFFFFFFFFF;
            }
        case 3: //kernel messages
            switch(subfunc){
                case 0: //write message
                    //check task privileges
                    if((mtask_get_by_pid(mtask_get_pid())->privl & TASK_PRIVL_KMESG) == 0)
                        return 1;
                    //check filename and message pointers (should be in userspace)
                    if(p0 + strlen((char*)p0) >= 0x800000000000ULL)
                        return 0xFFFFFFFFFFFFFFFF;
                    if(p1 + strlen((char*)p1) >= 0x800000000000ULL)
                        return 0xFFFFFFFFFFFFFFFF;
                    //write the message
                    krnl_write_msg((char*)p0, 0, (char*)p1);
                    return 0;
                default: //invalid subfunction number
                    return 0xFFFFFFFFFFFFFFFF;
            }
        default: //invalid function number
            return 0xFFFFFFFFFFFFFFFF;
    }
}