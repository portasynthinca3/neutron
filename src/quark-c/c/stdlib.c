//Neutron Project
//C Standard Library

#include "../h/stdlib.h"

struct _mem_block _mem_blocks[STDLIB_DRAM_MEMBLOCKS];

/*
 * Trigger bochs magic breakpoint
 */
volatile void breakpoint(){
    __asm__("xchgw %bx, %bx;");
}

/*
 * Abort execution
 */
void abort(){
    while(1);
}

/*
 * Enables the A20 line, allowing us to use more than a megabyte of RAM
 */
volatile void a20_enable(void){
    //Set DS to data selector
    __asm__("movw $0x10, %ax; movw %ax, %ds");
    //Write 0xAD to port 0x64
    _a20_enable_wait();
    __asm__("outb %%al, $0x64;" : : "a" (0xAD));
    //Write 0xD0 to port 0x64
    _a20_enable_wait();
    __asm__("outb %%al, $0x64;" : : "a" (0xD0));
    //Read from port 0x60
    _a20_enable_wait_2();
    unsigned char a;
    __asm__("inb $0x60, %%al;" : "=a" (a));
    //Write 0xD1 to port 0x64
    _a20_enable_wait();
    __asm__("outb %%al, $0x64;" : : "a" (0xD1));
    //Write (a | 0b10) to port 0x64
    _a20_enable_wait();
    __asm__("outb %%al, $0x60;" : : "a" (a | 2));
    //Write 0xAE to port 0x64
    _a20_enable_wait();
    __asm__("outb %%al, $0x64;" : : "a" (0xAE));
}

/*
 * for internal a20_enable() use only
 */
void _a20_enable_wait(void){
    //Constantly check if bit 2 in port 0x64 is cleared, return if so
    unsigned char in = 0b100;
    while(in & 0b100 != 0)
        __asm__("inb $0x64, %%al;" : "=a" (in));
}

/*
 * for internal a20_enable() use only
 */
void _a20_enable_wait_2(void){
    //Constantly check if bit 1 in port 0x64 is cleared, return if so
    unsigned char in = 0b10;
    while(in & 0b10 != 0)
        __asm__("inb $0x64, %%al;" : "=a" (in));
}

/*
 * Initialize the dynamic memory allocation
 */
void dram_init(void){
    //Clear the blocks
    memset(_mem_blocks, 0, STDLIB_DRAM_MEMBLOCKS);
    //Create a beginning block
    struct _mem_block blk;
    blk.ptr = (void*)STDLIB_DRAM_START;
    blk.size = STDLIB_DRAM_SIZE;
    //Assign it
    _mem_blocks[0] = blk;
}

/*
 * Allocate a block of memory
 */
void* malloc(unsigned int size){
    //Find the best block
    //The best block is a one with the minimal size satisfying the requirement that's also free
    int best_blk_id = -1;
    unsigned int best_blk_size = 0xFFFFFFFF;
    unsigned int walk = STDLIB_DRAM_MEMBLOCKS;
    while(walk--){
        if(!_mem_blocks[walk].used && _mem_blocks[walk].size >= size && _mem_blocks[walk].size < best_blk_size){
            best_blk_id = walk;
            best_blk_size = _mem_blocks[walk].size;
        }
    }
    //If no such blocks were found, return NULL
    if(best_blk_id == -1)
        return NULL;
    //If such block was found, split it into used and unused space
    struct _mem_block free = _mem_blocks[best_blk_id];
    struct _mem_block used;
    used.ptr = free.ptr;
    used.size = size;
    used.used = 1;
    free.ptr += size;
    free.size -= size;
    free.used = 0;
    //Save the freshly generated blocks
    _mem_blocks[best_blk_id] = used;
    _mem_blocks[best_blk_id + 1] = free;
    //Return the pointer
    return used.ptr;
}

/*
 * Fill a chunk of memory with certain values
 */
void* memset(void* dst, int ch, unsigned int size){
    //Convert ch to 8 bits
    unsigned char c = (unsigned char)ch;
    //Fill the chunk with them
    while(--size)
        *(unsigned char*)(dst + size) = c;
    //Return dst
    return dst;
}