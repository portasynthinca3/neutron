//Neutron Project
//C Standard Library

#include "../h/stdlib.h"
#include "../h/gfx.h"

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
    gfx_panic(0, 0xDEADBEEF);
    while(1);
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
    for(unsigned int i = 0; i < STDLIB_DRAM_MEMBLOCKS; i++){
        if(!_mem_blocks[i].used && _mem_blocks[i].size >= size && _mem_blocks[i].size < best_blk_size){
            best_blk_id = i;
            best_blk_size = _mem_blocks[i].size;
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
    while(size--)
        *(unsigned char*)(dst + size) = c;
    //Return dst
    return dst;
}

/*
 * Load Interrupt Descriptor Table
 */
void load_idt(struct idt_desc* idt){
    __asm__("lidt %0" : : "m" (*idt));
}

/*
 * Convert big endian doubleword to little endian one and vice versa
 */
void bswap_dw(int* value){
    __asm__("bswapl %%eax" : "=a" (*value) : "a" (*value));
}

/*
 * Output doubleword to I/O port
 */
void outl(unsigned short port, unsigned int value){
    __asm__ volatile("outl %%eax, %%dx" : : "a" (value), "d" (port));
}

/*
 * Read doubleword from I/O port
 */
unsigned int inl(unsigned short port){
    unsigned int value;
    __asm__ volatile("inl %%dx, %%eax" : "=a" (value) : "d" (port));
    return value;
}

/*
 * Output word to I/O port
 */
void outw(unsigned short port, unsigned short value){
    __asm__ volatile("outw %%ax, %%dx" : : "a" (value), "d" (port));
}

/*
 * Read word from I/O port
 */
unsigned short inw(unsigned short port){
    unsigned short value;
    __asm__ volatile("inw %%dx, %%ax" : "=a" (value) : "d" (port));
    return value;
}

/*
 * Output byte to I/O port
 */
void outb(unsigned short port, unsigned char value){
    __asm__ volatile("outb %%al, %%dx" : : "a" (value), "d" (port));
}

/*
 * Read byte from I/O port
 */
unsigned char inb(unsigned short port){
    unsigned char value;
    __asm__ volatile("inb %%dx, %%al" : "=a" (value) : "d" (port));
    return value;
}

/*
 * Manages pushing bytes into the FIFO buffer
 */
void fifo_pushb(unsigned char* buffer, unsigned short* head, unsigned char value){
    buffer[(*head)++] = value;
}

/*
 * Manages popping bytes from the FIFO buffer
 */
unsigned char fifo_popb(unsigned char* buffer, unsigned short* head, unsigned short* tail){
    unsigned char value = buffer[(*tail)++];
    if(*tail >= *head)
        *tail = *head = 0;
    return value;
}

/*
 * Returns the count of bytes available for read in the FIFO buffer
 */
unsigned char fifo_av(unsigned short* head, unsigned short* tail){
    return *head - *tail;
}