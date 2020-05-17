#ifndef SYSCALL_H
#define SYSCALL_H

#include "../../krnl.h"
#include "../../stdlib.h"

void syscall_init(void);
uint64_t syscall_get_krnl_rsp(void);

uint64_t syscall_handle(void);
void syscall_wrapper(void);

#endif