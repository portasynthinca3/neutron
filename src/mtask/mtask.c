//Neutron Project
//MTask - Multitasking engine

#include "./mtask.h"
#include "../stdlib.h"
#include "../drivers/timr.h"
#include "../drivers/gfx.h"
#include "../vmem/vmem.h"

task_t* mtask_task_list;
uint32_t mtask_next_task;
uint64_t mtask_next_uid;
uint32_t mtask_cur_task_no;
uint8_t mtask_enabled;
task_t* mtask_cur_task;

/*
 * Returns the current task pointer
 * (used only by mtask_sw.s)
 */
task_t* mtask_get_cur_task(void){
    return mtask_cur_task;
}

/*
 * Is multitasking enabled?
 * (used only by mtask_sw.s)
 */
uint64_t mtask_is_enabled(void){
    return mtask_enabled;
}

/*
 * Initializes the multitasking system
 */
void mtask_init(void){
    //Allocate a buffer for the task list
    mtask_task_list = (task_t*)malloc((MTASK_TASK_COUNT * sizeof(task_t)) + 16);
    //Clear it
    memset(mtask_task_list, 0, (MTASK_TASK_COUNT * sizeof(task_t)) + 16);
    //It needs to be 16-byte aligned
    mtask_task_list = (task_t*)((uint64_t)mtask_task_list + (uint64_t)(16 - ((uint64_t)mtask_task_list % 16)));
    
    mtask_cur_task_no = 0;
    mtask_next_task = 0;
    mtask_next_uid = 0;
    mtask_enabled = 0;
    //Initialize the scheduling timer
    timr_init();
}

/*
 * Gets an UID of the currently running task
 */
uint64_t mtask_get_uid(void){
    return mtask_task_list[mtask_cur_task_no].uid;
}

/*
 * Returns the task list
 */
task_t* mtask_get_task_list(void){
    return mtask_task_list;
}

/*
 * Stops the scheduler, effectively freezing the system
 */
void mtask_stop(void){
    mtask_enabled = 0;
    timr_stop();
}

/*
 * Creates a task
 * If it's the first task ever created, starts multitasking
 * Returns the UID
 */
uint64_t mtask_create_task(uint64_t stack_size, char* name, uint8_t priority, void(*func)(void*), void* args){
    task_t* task = &mtask_task_list[mtask_next_task];
    //Clear the task registers (except for RCX, set it to the argument pointer)
    task->state = (task_state_t){0, 0, (uint64_t)args, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    //Asign an UID
    task->uid = mtask_next_uid++;
    //Assign the priority
    task->priority = priority;
    task->prio_cnt = task->priority;
    //Copy the name
    memcpy(task->name, name, strlen(name) + 1);
    //Create a new PML4 and assign the CR3
    uint64_t cr3 = vmem_create_pml4(task->uid);
    task->state.cr3 = cr3;
    //Allocate memory for the task stack
    void* task_stack = calloc(stack_size, 1);
    //Map the memory
    vmem_map(cr3, 0, (phys_addr_t)(4ULL * 1024 * 1024 * 1024), 0);
    //Assign the task RSP
    task->state.rsp = (uint64_t)((uint8_t*)task_stack + stack_size - 1);
    //Assign the task RIP
    task->state.rip = (uint64_t)func;
    //Assign the task and RFLAGS
    uint64_t rflags;
    __asm__ volatile("pushfq; pop %0" : "=m" (rflags));
    task->state.rflags = rflags;
    //Reset some vars
    task->valid = 1;
    task->state_code = TASK_STATE_RUNNING;
    task->blocked_till = 0;

    //Check if it's the first task ever created
    if(mtask_next_task++ == 0){
        //Assign the current task
        mtask_cur_task = task;
        mtask_cur_task_no = 0;
        //Call the switcher
        //It should switch to the newly created task
        __asm__ volatile("cli");
        mtask_enabled = 1;
        vmem_init();
        __asm__ volatile("jmp mtask_restore_state");
    }

    return task->uid;
}

/*
 * Destroys the task with a certain UID
 */
void mtask_stop_task(uint64_t uid){
    //Find the task and destoy it
    for(uint32_t i = 0; i < MTASK_TASK_COUNT; i++)
        if(mtask_task_list[i].uid == uid)
            mtask_task_list[i].valid = 0;
    //Hang if we're terminating the current task
    if(uid == mtask_get_uid())
        while(1);
}

/*
 * Chooses the next task to be run
 */
void mtask_schedule(void){
    //If the currently running task still has time available
    if(mtask_cur_task->prio_cnt > 0) {
        //Decrease its available time
        mtask_cur_task->prio_cnt--;
    } else {
        //If not, restore its time
        mtask_cur_task->prio_cnt = mtask_cur_task->priority;
        //Go find a new task
        while(1){
            //We scan through the task list to find a next task that's valid and not blocked
            mtask_cur_task_no++;
            if(mtask_cur_task_no >= mtask_next_task)
                mtask_cur_task_no = 0;

            //Remove blocks on tasks that need to be unblocked
            if(mtask_task_list[mtask_cur_task_no].state_code == TASK_STATE_BlOCKED_CYCLES){
                if(rdtsc() >= mtask_task_list[mtask_cur_task_no].blocked_till){
                    mtask_task_list[mtask_cur_task_no].state_code = TASK_STATE_RUNNING;
                    mtask_task_list[mtask_cur_task_no].blocked_till = 0;
                }
            }

            if(mtask_task_list[mtask_cur_task_no].valid && (mtask_task_list[mtask_cur_task_no].state_code == TASK_STATE_RUNNING))
                break;
        }
    }

    mtask_cur_task = &mtask_task_list[mtask_cur_task_no];
}

/*
 * Blocks the currently running task for a specific amount of CPU cycles
 */
void mtask_dly_cycles(uint64_t cycles){
    //Set the block
    mtask_cur_task->blocked_till = rdtsc() + cycles;
    mtask_cur_task->state_code = TASK_STATE_BlOCKED_CYCLES;
    //This variable will be set by the scheduler
    while(mtask_cur_task->state_code != TASK_STATE_RUNNING);
}