#ifndef MTASK_H
#define MTASK_H

#include "../stdlib.h"

typedef struct {
    uint64_t rax, rbx, rcx, rdx, rsi, rdi, rbp, rsp;
    uint64_t r8,  r9,  r10, r11, r12, r13, r14, r15;
    uint64_t cr3, rip, rflags, switch_cnt;
    uint8_t align[16];
    uint8_t xstate[1024];
} __attribute__((packed)) task_state_t;

typedef struct {
    task_state_t state;

    uint8_t valid;
    uint64_t uid;
    char name[64];

    uint8_t priority;
    uint8_t prio_cnt;
    volatile uint8_t state_code;
    uint64_t blocked_till;

    uint8_t padding[60];
} __attribute__((packed)) task_t;

#define MTASK_TASK_COUNT                    32

#define TASK_STATE_RUNNING                  0
#define TASK_STATE_BlOCKED_CYCLES           1
#define TASK_STATE_BLOCKED_MS               2
#define TASK_STATE_WAITING_TO_RUN           3

void mtask_init(void);
void mtask_stop(void);
uint64_t mtask_create_task(uint64_t stack_size, char* name, uint8_t priority, uint8_t identity_map, uint64_t _cr3,
        void* suggested_stack, uint8_t start, void(*func)(void*), void* args);
task_t* mtask_get_by_uid(uint64_t uid);
void mtask_stop_task(uint64_t uid);
uint64_t mtask_get_uid(void);
task_t* mtask_get_task_list(void);

void mtask_save_state(void);
void mtask_restore_state(void);
void mtask_schedule(void);

void mtask_dly_cycles(uint64_t cycles);

#endif