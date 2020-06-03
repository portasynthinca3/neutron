//Neutron Project
//MTask - Multitasking engine

#include "./mtask.h"
#include "../stdlib.h"
#include "../drivers/timr.h"
#include "../drivers/gfx.h"
#include "../vmem/vmem.h"
#include "../krnl.h"

task_t* mtask_task_list;
uint32_t mtask_next_task;
uint64_t mtask_next_pid;
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
    mtask_task_list = (task_t*)amalloc((MTASK_TASK_COUNT * sizeof(task_t)),  16);
    //Clear it
    memset(mtask_task_list, 0, (MTASK_TASK_COUNT * sizeof(task_t)));
    
    mtask_cur_task_no = 0;
    mtask_next_task = 0;
    mtask_next_pid = 1;
    mtask_enabled = 0;
    //Initialize the scheduling timer
    timr_init();
}

/*
 * Gets a PID of the currently running task
 */
uint64_t mtask_get_pid(void){
    return mtask_task_list[mtask_cur_task_no].pid;
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
}

/*
 * Creates a task
 * If it's the first task ever created, starts multitasking
 * Returns the PID
 */
uint64_t mtask_create_task(uint64_t stack_size, char* name, uint8_t priority, uint8_t identity_map, uint64_t _cr3,
        void* suggested_stack, uint8_t start, void(*func)(void*), void* args, uint64_t privl){

    task_t* task = &mtask_task_list[mtask_next_task];
    //Clear the opened file list
    memset(task->open_files, 0, sizeof(file_handle_t*) * MTASK_MAX_OPEN_FILES);
    //Clear the task registers (except for RCX, set it to the argument pointer)
    task->state = (task_state_t){0, 0, (uint64_t)args, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    //Asign a PID
    task->pid = mtask_next_pid++;
    //Assign the priority and privileges
    task->priority = priority;
    task->prio_cnt = task->priority;
    task->privl = privl;
    //Copy the name
    memcpy(task->name, name, strlen(name) + 1);
    //Use the current address space or assign the suggested CR3
    uint64_t cr3 = _cr3;
    if(cr3 == 0)
        cr3 = vmem_get_cr3();
    task->state.cr3 = cr3;
    if(identity_map)
        vmem_map(cr3, 0, (phys_addr_t)(8ULL * 1024 * 1024 * 1024), 0);
    //Allocate memory for the task stack
    void* task_stack = suggested_stack;
    if(task_stack == NULL)
        task_stack = calloc(stack_size, 1);
    //Assign the task RSP
    task->state.rsp = (uint64_t)((uint8_t*)task_stack + stack_size);
    //Assign the task RIP
    task->state.rip = (uint64_t)func;
    //Assign the task RFLAGS
    uint64_t rflags;
    __asm__ volatile("pushfq; pop %0" : "=m" (rflags));
    rflags |= 1 << 9; //enable interrupts
    task->state.rflags = rflags;
    //Reset some vars
    task->valid = 1;
    task->state_code = start ? TASK_STATE_RUNNING : TASK_STATE_WAITING_TO_RUN;
    task->blocked_till = 0;
    task->next_alloc = (virt_addr_t)((1ULL << 46) | (1ULL << 45));
    //Set CS
    task->state.cs = 0x93;

    //Check if it's the first task ever created
    if((mtask_next_task++ == 0) && start){
        //Enable physwin
        vmem_enable_physwin();
        //Assign the current task
        mtask_cur_task = task;
        mtask_cur_task_no = 0;
        //Switch to the newly created task
        mtask_enabled = 1;
        __asm__ volatile("jmp mtask_restore_state");
    }

    return task->pid;
}

/*
 * Gets the task by the PID
 */
task_t* mtask_get_by_pid(uint64_t pid){
    if(pid == 0)
        return NULL;
    for(uint32_t i = 0; i < MTASK_TASK_COUNT; i++)
        if(mtask_task_list[i].pid == pid)
            return &mtask_task_list[i];
    return NULL;
}

/*
 * Stops the task with by the PID
 */
void mtask_stop_task(uint64_t pid){
    //Find the task and destoy it
    for(uint32_t i = 0; i < MTASK_TASK_COUNT; i++)
        if(mtask_task_list[i].pid == pid)
            mtask_task_list[i].valid = 0;
    //Hang if we're terminating the current task
    if(pid == mtask_get_pid()){
        mtask_schedule();
        __asm__ volatile("sti");
        while(1);
    }
}

/*
 * Checks if a task with the specified PID exists
 */
uint8_t mtask_exists(uint64_t pid){
    for(uint32_t i = 0; i < MTASK_TASK_COUNT; i++)
        if(mtask_task_list[i].pid == pid)
            return 1;
    return 0;
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
}

/*
 * Blocks the currently running task for a specific time in microseconds
 */
void mtask_dly_us(uint64_t us){
    //Convert milliseconds to CPU cycles
    uint64_t cycles = ((timr_get_cpu_fq() / 1000) * us) / 1000;
    //Block for that amount of cycles
    mtask_dly_cycles(cycles);
}

/*
 * Adds privileges specified in mask to the currently running process
 */
void mtask_escalate(uint64_t mask){
    if(mtask_cur_task->privl & TASK_PRIVL_SUDO_MODE) {
        mtask_cur_task->privl |= mask;
    } else {
        //TODO: some kind of user input to confirm the escalation
        mtask_cur_task->privl |= mask | TASK_PRIVL_SUDO_MODE;
    }
}

/*
 * Adds a handle pointer to the list of files opened by the handle owner PID
 */
void mtask_add_open_file(file_handle_t* ptr){
    task_t* task = mtask_get_by_pid(ptr->pid);
    //Go through the list
    if(task != NULL){
        for(int i = 0; i < MTASK_MAX_OPEN_FILES; i++){
            //Find an unused entry
            if(task->open_files[i] != 0){
                task->open_files[i] = ptr;
                break;
            }
        }
    }
}

/*
 * Removes a handle pointer from the list of files opened by the handle owner PID
 */
void mtask_remove_open_file(file_handle_t* ptr){
    task_t* task = mtask_get_by_pid(ptr->pid);
    //Go through the list
    if(task != NULL){
        for(int i = 0; i < MTASK_MAX_OPEN_FILES; i++){
            //Find the matching entry
            if(task->open_files[i] == ptr){
                task->open_files[i] = 0;
                break;
            }
        }
    }
}

/*
 * Allocates a number of memory pages and maps them for the specified process
 */
virt_addr_t mtask_palloc(uint64_t pid, uint64_t num){
    task_t* task = mtask_get_by_pid(pid);
    //Allocate pages
    virt_addr_t krnl_addr = amalloc(4096 * num, 4096);
    if(krnl_addr == NULL)
        return NULL;
    phys_addr_t phys_addr = vmem_virt_to_phys(vmem_get_cr3(), krnl_addr);
    //Find an unused allocation entry and stick the address in there
    for(int i = 0; i < MTASK_MAX_ALLOCATIONS; i++){
        if(!task->allocations[i].used){
            task->allocations[i].used = 1;
            task->allocations[i].num = num;
            task->allocations[i].krnl_map = krnl_addr;
            task->allocations[i].proc_map = task->next_alloc;
            break;
        }
    }
    //Map the actual range
    vmem_map_user(task->state.cr3, phys_addr, (phys_addr_t)((uint8_t*)phys_addr + (4096 * num)), task->next_alloc);
    //Advance the next allocation address
    virt_addr_t mapped_addr = task->next_alloc;
    task->next_alloc = (virt_addr_t)((uint8_t*)task->next_alloc + (4096 * num));
    //Return the resulting mapped address
    return mapped_addr;
}

/*
 * Deallocates a number of memory pages and unmaps them for the specified process
 */
void mtask_pfree(uint64_t pid, virt_addr_t proc_map){
    task_t* task = mtask_get_by_pid(pid);
    //Find an entry that corresponds to the mapped pages
    for(int i = 0; i < MTASK_MAX_ALLOCATIONS; i++){
        if(task->allocations[i].proc_map == proc_map){
            //Free the pages
            free(task->allocations[i].krnl_map);
            //Unmap the pages
            vmem_unmap(task->state.cr3, task->allocations[i].proc_map,
                              (uint8_t*)task->allocations[i].proc_map + (4096 * task->allocations[i].num));
            //Invalidate the entry
            task->allocations[i].used = 0;
        }
    }
}