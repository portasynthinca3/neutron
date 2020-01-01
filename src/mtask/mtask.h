#ifndef MTASK_H
#define MTASK_H

#include "../stdlib.h"

typedef struct {
    uint64_t rax, rbx, rcx, rdx, rsi, rdi, rbp, rsp;
    uint64_t r8,  r9,  r10, r11, r12, r13, r14, r15;
    uint64_t cr3, rip, rflags;
} __attribute__((packed)) task_state_t;

typedef struct {
    task_state_t state;
    uint64_t uid;
    char name[64];
} task_t;

#define MTASK_TASK_COUNT            32

void mtask_init(void);
void mtask_stop(void);
void mtask_create_task(uint64_t stack_size, char* name, void(*func)(void));

void mtask_save_state(void);
void mtask_restore_state(void);
void mtask_schedule(void);

#endif