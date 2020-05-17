#ifndef MTASK_H
#define MTASK_H

#include "../stdlib.h"

//Structure definitions

typedef struct {
    uint64_t rax, rbx, rcx, rdx, rsi, rdi, rbp, rsp;
    uint64_t r8,  r9,  r10, r11, r12, r13, r14, r15;
    uint64_t cr3, rip, rflags, switch_cnt;
    uint16_t cs;
    uint8_t padding[14];
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

    uint64_t privl;

    uint8_t padding[52];
} __attribute__((packed)) task_t;

//Settings

#define MTASK_TASK_COUNT                    128

//Task state codes

#define TASK_STATE_RUNNING                  0
#define TASK_STATE_BlOCKED_CYCLES           1
#define TASK_STATE_WAITING_TO_RUN           3
#define TASK_STATE_WAITING_FOR_PRIVL_ESC    4

//Task privileges

#define TASK_PRIVL_EVERYTHING               0x7FFFFFFFFFFFFFFFULL
#define TASK_PRIVL_INHERIT                  (1ULL << 63)
#define TASK_PRIVL_KMESG                    (1ULL << 0)

//Function prototypes

void mtask_init(void);
void mtask_stop(void);
uint64_t mtask_create_task(uint64_t stack_size, char* name, uint8_t priority, uint8_t identity_map, uint64_t _cr3,
        void* suggested_stack, uint8_t start, void(*func)(void*), void* args, uint64_t privl);
task_t* mtask_get_by_uid(uint64_t uid);
void mtask_stop_task(uint64_t uid);
uint64_t mtask_get_uid(void);
task_t* mtask_get_task_list(void);

void mtask_save_state(void);
void mtask_restore_state(void);
void mtask_schedule(void);

void mtask_dly_cycles(uint64_t cycles);
void mtask_dly_us(uint64_t us);

void mtask_escalate(uint64_t mask);

#endif