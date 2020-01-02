//Neutron Project
//MTask - Multitasking engine

#include "./mtask.h"
#include "../stdlib.h"
#include "../drivers/timr.h"

task_t* mtask_task_list;
uint32_t mtask_next_task;
uint64_t mtask_next_uid;
uint8_t mtask_ready = 0;
task_t* mtask_cur_task;
uint32_t mtask_cur_task_no = 0;

/*
 * Initializes the multitasking system
 */
void mtask_init(void){
    //Allocate a buffer for the task list
    mtask_task_list = (task_t*)malloc(MTASK_TASK_COUNT * sizeof(task_t));
    
    mtask_next_task = 0;
    mtask_next_uid = 0;
    mtask_ready = 0;
    mtask_cur_task_no = 0;
}

/*
 * Stops the scheduler, effectively freezing the system
 */
void mtask_stop(void){
    mtask_ready = 0;
}

/*
 * Creates a task
 * If it's the first task ever created, starts multitasking
 */
void mtask_create_task(uint64_t stack_size, char* name, void(*func)(void)){
    task_t* task = &mtask_task_list[mtask_next_task];
    //Asign an UID
    task->uid = mtask_next_uid++;
    //Copy the name
    memcpy(task->name, name, strlen(name) + 1);
    //Clear the task registers
    task->state = (task_state_t){0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    //Allocate memory for the task stack
    void* task_stack = malloc(stack_size);
    //Assign the task RSP
    task->state.rsp = (uint64_t)(task_stack + stack_size - 1);
    //Assign the task RIP
    task->state.rip = (uint64_t)func;
    //Assign the task CR3 and RFLAGS
    uint64_t cr3, rflags;
    __asm__ volatile("mov %%cr3, %0" : "=r" (cr3));
    __asm__ volatile("pushfq; pop %0" : "=r" (rflags));
    task->state.cr3 = cr3;
    task->state.rflags = rflags;
    //Mark the task as both valid and running
    task->valid = 1;
    task->running = 1;

    //Check if it's the first task ever created
    if(mtask_next_task++ == 0){
        //Assign the current task
        mtask_cur_task = task;
        mtask_cur_task_no = 0;
        //We're ready AF!
        mtask_ready = 1;
        //Initialize the scheduling timer
        timr_init();
        //Call the task function
        func();
    }
}

/*
 * Chooses the next task to be run
 */
void mtask_schedule(void){
    uint8_t found = 0;
    while(!found){
        //We scan through the task list to find a next task that's valid and running
        mtask_cur_task_no++;
        if(mtask_cur_task_no >= mtask_next_task)
            mtask_cur_task_no = 0;
        found = mtask_task_list[mtask_cur_task_no].valid && mtask_task_list[mtask_cur_task_no].running;
    }

    mtask_cur_task = &mtask_task_list[mtask_cur_task_no];
}