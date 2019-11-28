#ifndef STDLIB_H
#define STDLIB_H

//Null pointer
#define NULL 0
//The amount of memory blocks for dynamic memory allocation functions to use
#define STDLIB_DRAM_MEMBLOCKS 1024
//The address that dynamic memory allocation starts from
#define STDLIB_DRAM_START 0x500000
//The total size of the memory available to the dynamic memory allocator
#define STDLIB_DRAM_SIZE (128 * 0x100000)

//The quark version displayed on startup
#define QUARK_VERSION_STR "  Quark version is 0.0.2"
//Quark panic code for reaching the end
#define QUARK_PANIC_CODE_END 0xABADBABE

/*
 * Structure defining a memory block
 * A lot of them are stored in a specific location in memory
 *   for malloc(), free(), realloc() and others to use
 */
struct _mem_block{
    unsigned char used;
    void* ptr;
    unsigned int size;
};

/*
 * Structure describing the Interrupt Descripotor Table Descriptor
 */
struct idt_desc{
    unsigned short limit;
    void* base;
} __attribute__((packed));

/*
 * Structure describing an IDT entry
 */
struct idt_entry {
   unsigned short offset_lower;
   unsigned short code_selector;
   unsigned char zero;
   unsigned char type_attr;
   unsigned short offset_higher;
} __attribute__ ((packed));

//Debug functions

volatile void breakpoint();
void abort();

//Low-level functions

void load_idt(struct idt_desc* idt);

//Dynamic memory allocation functions

void dram_init(void);
void* malloc(unsigned int size);

//Memory operation functions

void* memset(void* dst, int ch, unsigned int size);

#endif
