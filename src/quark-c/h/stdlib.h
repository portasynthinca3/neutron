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
#define QUARK_VERSION_STR "Quark v0.0.7"
//Quark panic code for reaching the end
#define QUARK_PANIC_CODE_END 0xABADBABE

//Standard type definitions

typedef unsigned int size_t;
typedef unsigned char uint8_t;
typedef signed char int8_t;
typedef unsigned short uint16_t;
typedef signed char int16_t;
typedef unsigned int uint32_t;
typedef signed int int32_t;
typedef unsigned long long uint64_t;
typedef signed long long int64_t;
typedef unsigned int size_t;

/*
 * Structure defining a memory block
 * A lot of them are stored in a specific location in memory
 *   for malloc(), free(), realloc() and others to use
 */
struct _mem_block {
    uint8_t used;
    void* ptr;
    uint32_t size;
};

/*
 * Structure describing the Interrupt Descripotor Table Descriptor
 */
struct idt_desc {
    uint16_t limit;
    void* base;
} __attribute__((packed));

/*
 * Structure describing an IDT entry
 */
struct idt_entry {
   uint16_t offset_lower;
   uint16_t code_selector;
   uint8_t zero;
   uint8_t type_attr;
   uint16_t offset_higher;
} __attribute__ ((packed));

//Debug functions

volatile void breakpoint();
void abort();
void puts_e9(char* str);

//Low-level functions

void load_idt(struct idt_desc* idt);
void bswap_dw(int* value);
uint64_t rdtsc();
int read_rtc_time(uint8_t* h, uint8_t* m, uint8_t* s);

//Dynamic memory allocation functions

void dram_init(void);
void* malloc(size_t size);
void free(void* ptr);
void* calloc(uint32_t num, size_t size);

//Memory operation functions

void* memset(void* dst, int ch, uint32_t size);
void* memcpy(void* destination, const void* source, uint32_t num);

//I/O port operation functions

void outl(uint16_t port, uint32_t value);
uint32_t inl(uint16_t port);
void outw(uint16_t port, uint16_t value);
uint16_t inw(uint16_t port);
void outb(uint16_t port, uint8_t value);
uint8_t inb(uint16_t port);
void rep_insw(uint16_t port, uint32_t count, uint16_t* buf);

//FIFO buffer operations

void fifo_pushb(uint8_t* buffer, uint16_t* head, uint8_t value);
uint8_t fifo_popb(uint8_t* buffer, uint16_t* head, uint16_t* tail);
uint8_t fifo_av(uint16_t* head, uint16_t* tail);

//String functions

size_t strlen(const char* str);
char* sprintu(char* str, uint32_t i, uint8_t min);
char* strcat(char* dest, char* src);
int strcmp(const char* str1, const char* str2);

#endif
