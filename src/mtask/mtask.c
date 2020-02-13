//Neutron Project
//MTask - Multitasking engine

#include "./mtask.h"
#include "../stdlib.h"
#include "../drivers/timr.h"

task_t* mtask_task_list;
uint32_t mtask_next_task;
uint64_t mtask_next_uid;
uint32_t mtask_cur_task_no;
uint8_t mtask_enabled;
task_t* mtask_cur_task;

task_t* mtask_get_cur_task(){
    return mtask_cur_task;
}

/*
 * Initializes the multitasking system
 */
void mtask_init(void){
    //Allocate a buffer for the task list
    mtask_task_list = (task_t*)calloc(MTASK_TASK_COUNT, sizeof(task_t));
    
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
    void* task_stack = calloc(stack_size, 1);
    //Assign the task RSP
    task->state.rsp = (uint64_t)(task_stack + stack_size - 1);
    //Assign the task RIP
    task->state.rip = (uint64_t)func;
    //Assign the task CR3 and RFLAGS
    uint64_t cr3, rflags;
    __asm__ volatile("mov %%cr3, %0" : "=r" (cr3));
    __asm__ volatile("pushfq; pop %0" : "=m" (rflags));
    task->state.cr3 = cr3;
    task->state.rflags = rflags;
    //Mark the task as valid
    task->valid = 1;

    //Check if it's the first task ever created
    if(mtask_next_task++ == 0){
        //Assign the current task
        mtask_cur_task = task;
        mtask_cur_task_no = 0;
        //Call the switcher
        //It should switch to the newly created task
        __asm__ volatile("cli");
        mtask_enabled = 1;
        char temp[20];
        gfx_verbose_println(sprintub16(temp, (uint64_t)&mtask_cur_task, 16));
        __asm__ volatile("jmp mtask_restore_state");
    }
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
    while(1){
        //We scan through the task list to find a next task that's valid and running
        mtask_cur_task_no++;
        if(mtask_cur_task_no >= mtask_next_task)
            mtask_cur_task_no = 0;
        if(mtask_task_list[mtask_cur_task_no].valid)
            break;
    }

    mtask_cur_task = &mtask_task_list[mtask_cur_task_no];
}