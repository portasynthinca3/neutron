#ifndef MTASK_H
#define MTASK_H

#include "../stdlib.h"
#include "../drivers/diskio.h"
#include "../vmem/vmem.h"

//Settings

#define MTASK_TASK_COUNT                    128
#define MTASK_MAX_OPEN_FILES                256
#define MTASK_MAX_ALLOCATIONS               1024

//Structure definitions

typedef struct {
    uint8_t used;
    virt_addr_t proc_map;
    virt_addr_t krnl_map;
    uint64_t num;
} page_alloc_t;

typedef struct {
    uint64_t rax, rbx, rcx, rdx, rsi, rdi, rbp, rsp;
    uint64_t r8,  r9,  r10, r11, r12, r13, r14, r15;
    uint64_t cr3, rip, rflags, switch_cnt;
    uint16_t cs;
    uint8_t exc_vector;
    uint8_t padding[29];
    uint8_t xstate[1024];
} __attribute__((packed)) task_state_t;

typedef struct {
    task_state_t state;

    uint8_t valid;
    uint64_t pid;
    char name[64];

    uint8_t priority;
    uint8_t prio_cnt;
    volatile uint8_t state_code;
    uint64_t blocked_till;

    uint64_t privl;

    file_handle_t* open_files[MTASK_MAX_OPEN_FILES];

    virt_addr_t next_alloc;
    page_alloc_t allocations[MTASK_MAX_ALLOCATIONS];

    uint8_t* symtab;
    uint8_t* strtab;
} task_t;

//Task state codes

#define TASK_STATE_RUNNING                  0
#define TASK_STATE_BLOCKED_CYCLES           1
#define TASK_STATE_WAITING_TO_RUN           3
#define TASK_STATE_WAITING_FOR_PRIVL_ESC    4

//Task privileges

#define TASK_PRIVL_EVERYTHING               (0xFFFFFFFFFFFFFFFFULL & ~TASK_PRIVL_INHERIT & ~TASK_PRIVL_SUDO_MODE)
#define TASK_PRIVL_INHERIT                  (1ULL << 63)
#define TASK_PRIVL_KMESG                    (1ULL << 0)
#define TASK_PRIVL_SUDO_MODE                (1ULL << 1)
#define TASK_PRIVL_SYSFILES                 (1ULL << 2)
#define TASK_PRIVL_DEVFILES                 (1ULL << 3)

//Function prototypes

//Global control
void mtask_init (void);
void mtask_stop (void);
//Task creating/destruction/getting/setting/etc.
uint64_t mtask_create_task(uint64_t stack_size, char* name, uint8_t priority, uint8_t identity_map, uint64_t _cr3,
                           void* suggested_stack, uint8_t start, void(*func)(void*), void* args, uint64_t privl, uint8_t* symtab,
                           uint8_t* strtab);
void     mtask_stop_task     (uint64_t pid);
task_t*  mtask_get_by_pid    (uint64_t pid);
uint64_t mtask_get_pid       (void);
uint8_t  mtask_exists        (uint64_t pid);
task_t*  mtask_get_task_list (void);
task_t*  mtask_get_cur_task  (void);
void     mtask_escalate      (uint64_t mask);
//Save/restore/schedule
void mtask_save_state    (void);
void mtask_restore_state (void);
void mtask_schedule      (void);
//Delays
void mtask_dly_cycles (uint64_t cycles);
void mtask_dly_us     (uint64_t us);
//Opened files control
void mtask_add_open_file    (file_handle_t* ptr);
void mtask_remove_open_file (file_handle_t* ptr);
//Memory allocation control
virt_addr_t mtask_palloc (uint64_t pid, uint64_t num);
void        mtask_pfree  (uint64_t pid, virt_addr_t proc_map);

#endif