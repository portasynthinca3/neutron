#ifndef STDLIB_H
#define STDLIB_H

#include <stdarg.h>

#ifndef NULL
#define NULL ((void*)0)
#endif

//Use WC for memcpy() transfers?
//#define STDLIB_MEMCPY_WC

//Standard type definitions

typedef unsigned char uint8_t;
typedef signed char int8_t;
typedef unsigned short uint16_t;
typedef signed short int16_t;
typedef unsigned int uint32_t;
typedef signed int int32_t;
typedef long long unsigned int uint64_t;
typedef long long signed int int64_t;
typedef uint64_t size_t;

/*
 * Structure defining a linked list node
 */
struct list_node {
    void* data;
    struct list_node* prev;
    struct list_node* next;
};
typedef struct list_node list_node_t;

/*
 * Structure defining a free memory block
 */
typedef struct _free_block_s {
    char signature[4];
    struct _free_block_s* next;
    struct _free_block_s* prev;
    size_t size;
} free_block_t;

//Don't forget to comment this on a release version :)
#define STDLIB_CARSH_ON_ALLOC_ERR

/*
 * Structure describing the Interrupt Descripotor Table Descriptor
 */
typedef struct {
    uint16_t limit;
    void* base;
} __attribute__((packed)) idt_desc_t;

typedef idt_desc_t gdt_desc_t;

/*
 * Structure describing an IDT entry
 */
typedef struct {
    uint16_t offset_1;
    uint16_t selector;
    uint8_t  intr_stack_table;
    uint8_t  type_attr;
    uint16_t offset_2;
    uint32_t offset_3;
    uint32_t reserved;
} __attribute__((packed)) idt_entry_t;

/*
 * Structure describing a GDT entry
 */
typedef struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  limit_hi_flags;
    uint8_t  base_hi;
} __attribute__((packed)) gdt_entry_t;

/*
 * Structure describing a task state segment
 */
typedef struct {
    uint32_t rsvd0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t rsvd1;
    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint64_t rsvd2;
    uint16_t rsvd3;
    uint16_t io_base;
} __attribute__((packed)) tss_t;

//A macro that creates generic IDT entries
#define IDT_ENTRY(OFFS, CSEL, TYPE) ((idt_entry_t){.offset_1 = (OFFS) & 0xFFFF, .selector = (CSEL), .intr_stack_table = 0, .type_attr = (TYPE), .offset_2 = (OFFS) >> 16, .offset_3 = (OFFS) >> 32, .reserved = 0})
//A macro that creates Kernel ISR IDT entries
#define IDT_ENTRY_ISR(OFFS, CS) (IDT_ENTRY((OFFS), (CS), 0b10001110))

//Panic codes

#define KRNL_PANIC_NOMEM_CODE              1
#define KRNL_PANIC_NOMEM_MSG               "malloc() failed due to lack of free memory"
#define KRNL_PANIC_PANTEST_CODE            2
#define KRNL_PANIC_PANTEST_MSG             "not an error: called gfx_panic() for testing purposes"
#define KRNL_PANIC_CPUEXC_CODE             3
#define KRNL_PANIC_CPUEXC_MSG              "CPU-generated exception"
#define KRNL_PANIC_STACK_SMASH_CODE        4
#define KRNL_PANIC_STACK_SMASH_MSG         "Stack smashing detected"
#define KRNL_PANIC_INVL_SYSCALL_CODE       5
#define KRNL_PANIC_INVL_SYSCALL_MSG        "Invalid syscall number"
#define KRNL_PANIC_UNKNOWN_MSG             "<unknown code>"

//Debug functions

volatile void breakpoint();
void abort();
void puts_e9(char* str);

//Low-level functions

void load_idt(idt_desc_t* idt);
void bswap_dw(int* value);
uint64_t rdtsc(void);
uint8_t read_rtc_time(uint16_t* h, uint16_t* m, uint16_t* s, uint16_t* d, uint16_t* mo, uint16_t* y);
void gdt_create(uint16_t sel, uint32_t base, uint32_t limit, uint8_t flags, uint8_t access);
int memcmp(const void* lhs, const void* rhs, size_t cnt);
uint64_t rdmsr(uint32_t msr);
void wrmsr(uint32_t msr, uint64_t val);
uint32_t rand(void);
uint64_t popcnt(uint64_t n);

//Dynamic memory functions

void* stdlib_physbase(void);
uint64_t stdlib_usable_ram(void);
uint64_t stdlib_used_ram(void);
uint64_t dram_init(void);
void dram_shift(void);
void* malloc(size_t size);
void* amalloc(size_t size, size_t gran);
void free(void* ptr);
void* calloc(uint64_t num, size_t size);

//Memory operation functions

void* memset(void* dst, int ch, size_t size);
void* memcpy(void* destination, const void* source, size_t num);
char* strcpy(char* dest, char* src);

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

//Linked list operations

list_node_t* list_append(list_node_t* first, void* element);
void* list_get_at_idx(list_node_t* first, uint32_t idx);

//String functions

size_t strlen(const char* str);
char* sprintu(char* str, uint64_t i, uint8_t min);
char* sprintub16(char* str, uint64_t i, uint8_t min);
int _sprintf_argcnt(char* fmt);
int _sprintf(char* str, const char* format, va_list valist);
int sprintf(char* str, const char* format, ...);
char* strcat(char* dest, char* src);
int strcmp(const char* str1, const char* str2);

#endif
