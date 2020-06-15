//Neutron Project
//C Standard Library

#include <efi.h>
#include <efilib.h>

#include "./stdlib.h"
#include "./drivers/gfx.h"
#include "./mtask/mtask.h"
#include "./vmem/vmem.h"
#include "./krnl.h"

alloc_block_t* first_alloc_block = NULL;
alloc_block_t* last_alloc_block  = NULL;

uint64_t bad_ram_bytes = 0;
uint64_t total_ram_bytes = 0;
uint64_t used_ram_bytes = 0;

uint32_t region_count;
memory_region_t ram_regions[128];

/*
 * Returns the amount of usable RAM
 */
uint64_t stdlib_usable_ram(void){
    return total_ram_bytes;
}

/*
 * Returns the amount of RAM currently being used by Neutron
 */
uint64_t stdlib_used_ram(void){
    return used_ram_bytes;
}

/*
 * Abort execution
 */
void abort(){
    //Stop the scheduler
    mtask_stop();
    //Hang
    while(1);
}

/*
 * Initialize the dynamic memory allocator
 */
uint64_t dram_init(void){
    //Get the memory map from EFI
    EFI_MEMORY_DESCRIPTOR* buf;
    uint64_t desc_size;
    uint32_t desc_ver;
    uint64_t size, map_key, region_sz;
    EFI_MEMORY_DESCRIPTOR* desc;
    EFI_STATUS status;
    uint32_t i = 0;
    //Allocate some memory
    size = sizeof(EFI_MEMORY_DESCRIPTOR) * 31;
    mem_map_retry:
    size += sizeof(EFI_MEMORY_DESCRIPTOR) * 31;
    status = krnl_get_efi_systable()->BootServices->AllocatePool(EfiLoaderData, size, (void*)&buf);
    if(EFI_ERROR(status)){
        krnl_get_efi_systable()->ConOut->OutputString(krnl_get_efi_systable()->ConOut,
            (CHAR16*)L"Failed to allocate memory for the memory map\r\n");
        while(1);
    }
    //Map the memory
    status = krnl_get_efi_systable()->BootServices->GetMemoryMap(&size, buf, &map_key, &desc_size, &desc_ver);
    //Re-allocate the buffer with a different size if the current one isn't sufficient
    if(EFI_ERROR(status)){
        if(status == EFI_BUFFER_TOO_SMALL){
            krnl_get_efi_systable()->BootServices->FreePool(buf);
            goto mem_map_retry;
        } else {
            krnl_get_efi_systable()->ConOut->OutputString(krnl_get_efi_systable()->ConOut,
                (CHAR16*)L"Failed to get the memory map\r\n");
            while(1);
        }
    }

    desc = buf;
    //Fetch the next descriptor
    while((uint8_t*)desc < ((uint8_t*)buf + size)){
        region_sz = desc->NumberOfPages * EFI_PAGE_SIZE;

        //If a new free memory block was found, record it
        if(desc->Type == EfiConventionalMemory && region_sz > 0){
            krnl_writec_f("Found 0x%x bytes of RAM at physical 0x%x\r\n", region_sz, desc->PhysicalStart);
            total_ram_bytes += region_sz;
            //Add the region to the list of regions
            ram_regions[region_count].phys_start      = (phys_addr_t)desc->PhysicalStart;
            ram_regions[region_count].virt_start      = (virt_addr_t)desc->PhysicalStart;
            ram_regions[region_count].virt_start_orig = (virt_addr_t)desc->PhysicalStart;
            ram_regions[region_count].size            = region_sz;
            region_count++;
            //Add the region to the list of allocation blocks
            alloc_block_t* fb_ptr = (alloc_block_t*)desc->PhysicalStart;
            fb_ptr->size      = region_sz;
            fb_ptr->prev      = last_alloc_block;
            fb_ptr->next      = NULL;
            fb_ptr->used      = 0;
            fb_ptr->region    = region_count - 1;
            if(first_alloc_block != NULL)
                last_alloc_block->next = fb_ptr;
            else
                first_alloc_block = fb_ptr;
            last_alloc_block = fb_ptr;
        }
        //Record bad RAM
        else if(desc->Type == EfiUnusableMemory)
            bad_ram_bytes += region_sz;

        desc = (EFI_MEMORY_DESCRIPTOR*)((uint64_t)desc + desc_size);
        i++;
    }

    if(first_alloc_block == NULL){
        krnl_writec_f("No usable memory was found");
        while(1);
    } else {
        krnl_writec_f("%d KiB of RAM found\r\n", total_ram_bytes / 1024);
    }

    return map_key;
}

/*
 * Maps all dynamic memory regions for the specified memory space
 */
void dram_map(uint64_t cr3){
    for(int i = 0; i < region_count; i++){
        vmem_map(cr3, ram_regions[i].phys_start,
                      (uint8_t*)ram_regions[i].phys_start + ram_regions[i].size,
                      ram_regions[i].virt_start);
    }
}

/*
 * Shifts the dynamic memory region in the current address space to the higher quarter
 */
void dram_shift(void){
    //Shift the ranges
    krnl_writec_f("Shifting the dynamic RAM ranges\r\n");
    uint64_t next_region = 0xFFFFC00000000000ULL;
    for(int i = 0; i < region_count; i++){
        ram_regions[i].virt_start = (virt_addr_t)next_region;
        next_region += ram_regions[i].size;
    }
    //Map the ranges
    krnl_writec_f("Mapping the dynamic RAM ranges\r\n");
    dram_map(vmem_get_cr3());
    //Enable address translation
    krnl_writec_f("Enabling software address translation\r\n");
    vmem_enable_trans();
    //Shift the allocation blocks
    krnl_writec_f("Shifting the allocation blocks\r\n");
    alloc_block_t* ab = first_alloc_block;
    while(1){
        if(ab->next == NULL)
            break;
        if(ab->next->used > 2)
            break;
        alloc_block_t* prev_next = ab->next;
        if(ab->next != NULL)
            ab->next = UPPER_AB(ab->next);
        if(ab->prev != NULL)
            ab->prev = UPPER_AB(ab->prev);
        ab = prev_next;
    }
    ab->next = NULL;
    first_alloc_block = UPPER_AB(first_alloc_block);
    last_alloc_block  = UPPER_AB(ab);
    krnl_writec_f("Done shifting\r\n");
}

/*
 * Allocate a block of memory
 */
void* malloc(size_t size){
    return amalloc(size, 1);
}

/*
 * Aligned malloc()
 */
void* amalloc(size_t size, size_t gran){
    if(size == 0 || gran == 0)
        return NULL;
    //Find a free allocation block that satisfies the size requirement
    size_t total_size = size + sizeof(alloc_block_t);
    alloc_block_t* block = first_alloc_block;
    do {
        if(block->size >= total_size && block->used == 0){
            //Recalculate the size requirement based on the alignment of the block
            size_t actual_size = total_size;
            size_t scrapped = 0;
            if(((uint64_t)block + sizeof(alloc_block_t)) % gran != 0){
                scrapped = gran - (((uint64_t)block + sizeof(alloc_block_t)) % gran);
                actual_size += scrapped;
            }
            if(block->size < actual_size)
                continue;
            //Decrease the size
            block->size -= actual_size;
            //Move it
            alloc_block_t* new = block;
            block->prev->next = (alloc_block_t*)((uint64_t)block + actual_size);
            *block->prev->next = *block;
            block = block->prev->next;
            //Create a new block and mark it as used
            new->used       = 1;
            new->prev       = block->prev;
            new->prev->next = new;
            new->next       = block;
            new->size       = actual_size;
            new->region     = block->region;
            block = first_alloc_block;
            //Add the to total bytes of memory
            used_ram_bytes += actual_size;
            //Return the address
            return (void*)((uint64_t)new + scrapped + sizeof(alloc_block_t));
        }
    } while(block->used < 2 && (block = block->next));
    //We have't found such block
    #ifdef STDLIB_CRASH_ON_ALLOC_ERR
        gfx_panic(0, KRNL_PANIC_NOMEM_CODE);
    #else
        return NULL;
    #endif
}

/*
 * Free a memory block allocated by malloc(), calloc() and others
 */
void free(void* ptr){
    //Move the pointer to the left, so that it points to the block control structure
    ptr = (uint8_t*)ptr - sizeof(alloc_block_t);
    //Find a used block that is pointed to by the pointer
    alloc_block_t* block = first_alloc_block;
    do {
        if(block->used && block == ptr){
            //Mark it as unused
            block->used = 0;
            //Merge it with the previous block if possible
            if(block->prev != NULL && !block->prev->used){
                block->prev->size += block->size;
                block->prev->next = block->next;
                block = block->prev;
            }
            //Merge it with the next block if possible
            if(block->next != NULL && !block->next->used){
                block->size += block->next->size;
                block->next = block->next->next;
            }
            break;
        }
    } while(block->used < 2 && (block = block->next));
}

/*
 * Allocate a block of memory and fill it with zeroes
 */
void* calloc(uint64_t num, size_t size){
    //Allocate the memory using malloc()
    void* malloc_res = malloc(num * size);
    if(malloc_res != NULL) //If the returned pointer isn't null, memset() with zeroes and return it
        return memset(malloc_res, 0, num * size);
    else //Else, return null too
        return NULL;
}

/*
 * Fill a chunk of memory with given values
 */
void* memset(void* dst, int ch, size_t size){
    //Convert ch to 8 bits
    uint8_t c = (uint8_t)ch;
    //Fill the chunk with them
    while(size--)
        *(uint8_t*)((uint8_t*)dst + size) = c;
    //Return dst
    return dst;
}

/*
 * Copy a block of memory
 */
void* memcpy(void* destination, const void* source, size_t num){
    //Q = 8 bytes at a time
    //D = 4 bytes at a time
    //W = 2 bytes at a time
    //B = 1 byte  at a time
    if(num % 8 == 0)
        __asm__ volatile("rep movsq" : : "D" (destination), "S" (source), "c" (num / 8));
    else if(num % 4 == 0)
        __asm__ volatile("rep movsd" : : "D" (destination), "S" (source), "c" (num / 4));
    else if(num % 2 == 0)
        __asm__ volatile("rep movsw" : : "D" (destination), "S" (source), "c" (num / 2));
    else
        __asm__ volatile("rep movsb" : : "D" (destination), "S" (source), "c" (num));
    return destination;
}

/*
 * Copy a string to other string
 */
char* strcpy(char* dest, char* src){
    return memcpy(dest, src, strlen(src) + 1);
}

/*
 * Copy a block of memory to an overlapping block of memory
 */
void* memmove(void* dest, const void* src, size_t count){
    if(dest <= src){
        //Use memcpy if the destination is below the source
        return memcpy(dest, src, count);
    } else {
        //Use another, non-optimized algorithm
        for(int i = count; i >= 0; i--)
            *(uint8_t*)((uint8_t*)dest + i) = *(uint8_t*)((uint8_t*)src + i);
        return dest;
    }
}

/*
 * Load Interrupt Descriptor Table
 */
void load_idt(idt_desc_t* idt){
    __asm__("lidt %0" : : "m" (*idt));
}

/*
 * Load Global Descriptor Table
 */
void load_gdt(){

}

/*
 * Convert big endian doubleword to little endian one and vice versa
 */
void bswap_dw(uint32_t* value){
    __asm__("bswapl %%eax" : "=a" (*value) : "a" (*value));
}

/*
 * Output doubleword to I/O port
 */
void outl(uint16_t port, uint32_t value){
    __asm__ volatile("outl %%eax, %%dx" : : "a" (value), "d" (port));
}

/*
 * Read doubleword from I/O port
 */
uint32_t inl(uint16_t port){
    uint32_t value;
    __asm__ volatile("inl %%dx, %%eax" : "=a" (value) : "d" (port));
    return value;
}

/*
 * Output word to I/O port
 */
void outw(uint16_t port, uint16_t value){
    __asm__ volatile("outw %%ax, %%dx" : : "a" (value), "d" (port));
}

/*
 * Read word from I/O port
 */
uint16_t inw(uint16_t port){
    uint16_t value;
    __asm__ volatile("inw %%dx, %%ax" : "=a" (value) : "d" (port));
    return value;
}

/*
 * Read a number of words from I/O port and store it in memory
 */
void rep_insw(uint16_t port, uint32_t count, uint16_t* buf){
    __asm__ volatile("rep insw" : : "c" (count), "D" (buf), "d" (port));
}

/*
 * Output byte to I/O port
 */
void outb(uint16_t port, uint8_t value){
    __asm__ volatile("outb %%al, %%dx" : : "a" (value), "d" (port));
}

/*
 * Read byte from I/O port
 */
uint8_t inb(uint16_t port){
    uint8_t value;
    __asm__ volatile("inb %%dx, %%al" : "=a" (value) : "d" (port));
    return value;
}

/*
 * Reads a model-specific register (MSR)
 */
uint64_t rdmsr(uint32_t msr){
    uint64_t val_h;
    uint64_t val_l;
    /*
    __asm__ volatile("mov %0, %%ecx" : : "m" (msr));
    __asm__ volatile("rdmsr" : : : "eax", "edx");
    __asm__ volatile("mov %%edx, %0" : "=m" (val_h));
    __asm__ volatile("mov %%eax, %0" : "=m" (val_l));
    */
    __asm__ volatile("mov %2, %%ecx;"
                     "rdmsr;"
                     "mov %%edx, %0;"
                     "mov %%eax, %1" : "=m"(val_h), "=m"(val_l) : "m"(msr) : "eax", "edx", "ecx");
    return (val_h << 32) | val_l;
}

/*
 * Writes a value to model-specific register (MSR)
 */
void wrmsr(uint32_t msr, uint64_t val){
    uint32_t val_h = val >> 32;
    uint32_t val_l = val & 0xFFFFFFFF;
    __asm__ volatile("mov %0, %%ecx" : : "m" (msr));
    __asm__ volatile("mov %0, %%edx" : : "m" (val_h));
    __asm__ volatile("mov %0, %%eax" : : "m" (val_l));
    __asm__ volatile("wrmsr");
}

/*
 * Read the amount of cycles executed by the CPU
 */
uint64_t rdtsc(void){
    uint32_t h, l;
    __asm__ volatile("mfence; lfence; rdtsc" : "=d" (h), "=a" (l));
    return (uint64_t)((uint64_t)h << 32) | l;
}

/*
 * Get the length of a zero-terminated string
 */
size_t strlen(const char* str){
    size_t i = 0;
    while(str[i++] != 0);
    return i - 1;
}

/*
 * Print an uint64_t to the string
 */
char* sprintu(char* str, uint64_t i, uint8_t min){
    //Create some variables
    uint8_t pos = 0;
    uint64_t div = 1000000000000000000; //Start with the leftmost digit
    uint8_t started = 0;
    for(int j = 1; j <= 19; j++){
        //Fetch the next digit
        uint8_t digit = (i / div) % 10;
        //If the conversion hasn't started already and the current digit
        //  is greater than zero OR we exceeded the maximum amount of dropped
        //  digits, assume that the conversion has started
        if((!started && digit > 0) || (19 - j < min))
            started = 1;
        //If the conversion has started, write a digit to the string
        if(started)
            str[pos++] = digit + '0';
        //Move to the next digit
        div /= 10;
    }
    //Mark the end of the string
    str[pos] = 0;
    //Return the string
    return str;
}

const char hex_const[16] = "0123456789ABCDEF";

/*
 * Print an uint64_t with base 16 to the string
 */
char* sprintub16(char* str, uint64_t i, uint8_t min){
    //Create some variables
    uint8_t pos = 0;
    uint64_t div = 1ULL << 60; //Start with the leftmost digit
    uint8_t started = 0;
    for(uint8_t j = 1; j <= 16; j++){
        //Fetch the next digit
        uint8_t digit = (i / div) % 16;
        //If the conversion hasn't started already and the current digit
        //  is greater than zero OR we exceeded the maximum amount of dropped
        //  digits, assume that the conversion has started
        if((!started && digit > 0) || (16 - j < min))
            started = 1;
        //If the conversion has started, write a digit to the string
        if(started)
            str[pos++] = hex_const[digit];
        //Move to the next digit
        div >>= 4;
    }
    //Mark the end of the string
    str[pos] = 0;
    //Return the string
    return str;
}

/*
 * Append the string at src to the end of the string at dest
 */
char* strcat(char* dest, char* src){
    //Get the lengths of the strings
    size_t dest_len = strlen(dest);
    size_t src_len = strlen(src);
    //Copy the source string
    memcpy((void*)(dest + dest_len), (const void*)src, src_len);
    //Mark the end
    dest[dest_len + src_len] = 0;
    //Return the destination
    return dest;
}

/*
 * Returns the number of arguments sprintf will require on a string fmt
 */
int _sprintf_argcnt(char* fmt){
    uint64_t argcnt = 0;
    for(uint64_t i = 0; i < strlen(fmt); i++)
        if(fmt[i] == '%' && (i > 0 && fmt[i - 1] != '%'))
            argcnt++;
    return argcnt;
}

/*
 * Print formatted string
 */
int _sprintf(char* str, const char* format, va_list valist){
    //Parse the format
    uint64_t str_idx = 0;
    for(uint64_t i = 0; i < strlen(format); i++){
        //If it's a percentage sign, print something special
        if(format[i] == '%'){
            char fmt = format[++i];
            switch(fmt){
                case 's': { //string
                    char* str2 = va_arg(valist, char*);
                    for(uint64_t j = 0; j < strlen(str2); j++)
                        str[str_idx++] = str2[j];
                    break;
                }
                case 'c': //character
                    str[str_idx++] = va_arg(valist, int);
                    break;
                case '%': //percentage sign
                    str[str_idx++] = '%';
                    break;
                case 'n': //nothing
                    break;
                case 'd': //integer
                case 'u':
                case 'i': {
                    char buf[64];
                    sprintu(buf, va_arg(valist, uint64_t), 1);
                    for(uint64_t j = 0; j < strlen(buf); j++)
                        str[str_idx++] = buf[j];
                    break;
                }
                case 'p': //hex integer
                case 'x':
                case 'X': {
                    char buf[64];
                    sprintub16(buf, va_arg(valist, uint64_t), 1);
                    for(uint64_t j = 0; j < strlen(buf); j++)
                        str[str_idx++] = buf[j];
                    break;
                }
                default: //nothing else
                    return -1;
            }
        } else { //A normal character
            str[str_idx++] = format[i];
        }
    }
    //Add zero termination
    str[str_idx] = 0;
    //Return the amount of characters printed
    return str_idx;
}

/*
 * Print formatted string (wrapper)
 */
int sprintf(char* str, const char* format, ...){
    va_list valist;
    va_start(valist, _sprintf_argcnt((char*)format));
    int result = _sprintf(str, format, valist);
    va_end(valist);
    return result;
}

/*
 * Compare two memory blocks
 */
int memcmp(const void* lhs, const void* rhs, size_t cnt){
    //Go through each byte
    for(int i = 0; i < cnt; i++){
        //Return if the blocks aren't equal
        if(((const uint8_t*)lhs)[i] > ((const uint8_t*)rhs)[i])
            return 1;
        else if(((const uint8_t*)lhs)[i] < ((const uint8_t*)rhs)[i])
            return -1;
    }
    //If we didn't return, the blocks are equal
    return 0;
}

/*
 * Compare two zero-terminated strings
 */
int strcmp(const char* str1, const char* str2){
    //Calculate the length of both strings
    int len1 = strlen(str1);
    int len2 = strlen(str2);
    //Strings are not equal if they have different lengths (duh)
    if(len1 != len2)
        return (str1[0] > str2[0]) ? 1 : -1;
    //Go through each byte
    for(int i = 0; i < len1; i++){
        //Return if the strings aren't equal
        if(str1[i] > str2[i])
            return 1;
        else if(str1[i] < str2[i])
            return -1;
    }
    //If we didn't return, the strings are equal
    return 0;
}

/*
 * Parse number from string representation
 */
int atoi(const char* str){
    int n = 0;
    char c;
    //Fetch next character
    while((c = *(str++)) != 0){
        //If it's a digit, append it
        if(c >= '0' && c <= '9'){
            n *= 10;
            n += c - '0';
        } else {
            //Error
            return 0;
        }
    }
    return n;
}

/*
 * Generates a random number
 */
uint32_t rand(void){
    uint32_t num;
    __asm__ volatile("rdrand %%eax; mov %%eax, %0" : "=m"(num) : : "eax");
    return num;
}

/*
 * Returns the count of set bits
 */
uint64_t popcnt(uint64_t n){
    uint64_t c = 0;
    __asm__ volatile("popcnt %1, %0" : "=r"(c) : "r"(n));
    return c;
}