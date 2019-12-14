#ifndef STDLIB_H
#define STDLIB_H

//Null pointer
#define NULL 0
//The amount of memory blocks for dynamic memory allocation functions to use
#define STDLIB_DRAM_MEMBLOCKS 1024
//The address that dynamic memory allocation starts from
#define STDLIB_DRAM_START 0x500000

//The quark version displayed on startup
#define QUARK_VERSION_STR "Quark v0.1.0"

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
 * Structure defining a linked list node
 */
struct list_node {
    void* data;
    struct list_node* prev;
    struct list_node* next;
};
typedef struct list_node list_node_t;

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

//A macro that creates generic IDT entries
#define IDT_ENTRY(OFFS, CSEL, TYPE) ((struct idt_entry){.offset_lower = (OFFS) & 0xFFFF, .code_selector = CSEL, .zero = 0, .type_attr = TYPE, .offset_higher = (((OFFS) >> 16) & 0xFFFF)})
//A macro that creates Quark ISR IDT entries
#define IDT_ENTRY_ISR(OFFS) (IDT_ENTRY(OFFS, 8, 0b10001110))

//Debug functions

volatile void breakpoint();
void abort();
void puts_e9(char* str);

//Low-level functions

void load_idt(struct idt_desc* idt);
void bswap_dw(int* value);
uint64_t rdtsc();
int read_rtc_time(uint8_t* h, uint8_t* m, uint8_t* s);
void gdt_create(uint16_t sel, uint32_t base, uint32_t limit, uint8_t flags, uint8_t access);

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

//Linked list operations

list_node_t* list_append(list_node_t* first, void* element);
void* list_get_at_idx(list_node_t* first, uint32_t idx);

//String functions

size_t strlen(const char* str);
char* sprintu(char* str, uint32_t i, uint8_t min);
char* strcat(char* dest, char* src);
int strcmp(const char* str1, const char* str2);

#endif
