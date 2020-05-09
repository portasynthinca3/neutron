#ifndef SYSCALL_H
#define SYSCALL_H

#include "../../krnl.h"
#include "../../stdlib.h"

uint64_t handle_syscall(void);
void syscall_wrapper(void);

#endif