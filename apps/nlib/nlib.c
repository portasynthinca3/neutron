//Neutron project
//Standard library for userland applications

#include "nlib.h"

/*
 * Issues a system call
 */
uint64_t _syscall(uint32_t func, uint32_t subfunc,
                         uint64_t p0, uint64_t p1, uint64_t p2, uint64_t p3, uint64_t p4){
    //Calculate RDI number
    uint64_t rdi = ((uint64_t)func << 32) | subfunc;
    //Complicated stuff: loads registers with function numbers
    //  and syscall arguments, executes syscall, while telling
    //  GCC about what registers we're about to clobber, and then
    //  extracts the return value from RAX
    uint64_t ret;
    asm("mov %1, %%rdi;"
        "mov %2, %%rsi;"
        "mov %3, %%rdx;"
        "mov %4, %%r8 ;"
        "mov %5, %%r9 ;"
        "mov %6, %%r10;"
        "syscall      ;"
        "mov %%rax, %0;"
        :
        "=m"(ret)
        :
        "m"(rdi),
        "m"(p0),
        "m"(p1),
        "m"(p2),
        "m"(p3),
        "m"(p4)
        :
        "rax", "rbx", "rcx", "rdx", "r8", "r9", "r10", "r11", "rdi", "rsi");
    //Return the return value
    return ret;
}

/*
 * System call serialization/deserialization functions
 */
inline uint64_t _ser_p2d_t(p2d_t p){
    return ((uint64_t)p.x << 32) | (uint64_t)p.y;
}

inline p2d_t _deser_p2d_t(uint64_t p){
    return (p2d_t){
        .x = (int32_t)(p >> 32),
        .y = (int32_t)p
    };
}

inline uint64_t _ser_color32_t(color32_t c){
    return ((uint64_t)c.a << 24) |
           ((uint64_t)c.b << 16) |
           ((uint64_t)c.g <<  8) |
           ((uint64_t)c.r);
}

inline color32_t _deser_color32_t(uint64_t c){
    return (color32_t){
        .r = (uint8_t)c,
        .g = (uint8_t)(c >> 8),
        .b = (uint8_t)(c >> 16),
        .a = (uint8_t)(c >> 24)
    };
}


// -----===== SYSTEM CALLS: GRAPHICS =====-----


/*
 * System call: Graphics: Print a string in verbose mode
 */
inline uint64_t _gfx_println_verbose(char* str){
    return _syscall(0, 0, (uint64_t)str, 0, 0, 0, 0);
}

/*
 * System call: Graphics: Get resolution
 */
inline p2d_t _gfx_get_res(void){
    return _deser_p2d_t(_syscall(0, 1, 0, 0, 0, 0, 0));
}

/*
 * System call: Graphics: Flip buffers
 */
inline sc_state_t _gfx_flip(void){
    return _syscall(0, 2, 0, 0, 0, 0, 0);
}

/*
 * System call: Graphics: Fill buffer
 */
inline sc_state_t _gfx_fill(color32_t c){
    return _syscall(0, 3, _ser_color32_t(c), 0, 0, 0, 0);
}

/*
 * System call: Graphics: Fill rectangle
 */
inline sc_state_t _gfx_fill_rect(color32_t c, p2d_t pos, p2d_t sz){
    return _syscall(0, 4,
        _ser_color32_t(c),
        _ser_p2d_t(pos),
        _ser_p2d_t(sz),
        0, 0);
}

/*
 * System call: Graphics: Draw rectangle
 */
inline sc_state_t _gfx_draw_rect(color32_t c, p2d_t pos, p2d_t sz){
    return _syscall(0, 5,
        _ser_color32_t(c),
        _ser_p2d_t(pos),
        _ser_p2d_t(sz),
        0, 0);
}

/*
 * System call: Graphics: Draw raw image
 */
inline sc_state_t _gfx_draw_raw(uint8_t* img, p2d_t pos, p2d_t sz){
    return _syscall(0, 6,
        (uint64_t)img,
        _ser_p2d_t(pos),
        _ser_p2d_t(sz),
        0, 0);
}

/*
 * System call: Graphics: Text bounds
 */
inline p2d_t _gfx_text_bounds(char* str){
    return _deser_p2d_t(_syscall(0, 7, (uint64_t)str, 0, 0, 0, 0));
}

/*
 * System call: Graphics: Print string
 */
inline sc_state_t _gfx_draw_str(p2d_t pos, color32_t fg, color32_t bg, char* str){
    return _syscall(0, 8,
        (uint64_t)str,
        _ser_p2d_t(pos),
        (_ser_color32_t(fg) << 32) | _ser_color32_t(bg),
        0, 0);
}


// -----===== SYSTEM CALLS: TASK MANAGEMENT =====-----


/*
 * System call: Task management: Get task UID
 */
inline uint64_t _task_get_uid(void){
    return _syscall(1, 0, 0, 0, 0, 0, 0);
}

/*
 * System call: Task management: Terminate task
 */
inline sc_state_t _task_terminate(uint64_t uid){
    return _syscall(1, 1, uid, 0, 0, 0, 0);
}

/*
 * System call: Task management: Load executable
 */
inline uint64_t _task_load(char* path){
    return _syscall(1, 2, (uint64_t)path, 0, 0, 0, 0);
}


// -----===== SYSTEM CALLS: FILESYSTEM =====-----


/*
 * System call: Filesystem: Open file
 */
inline sc_state_t _fs_open(char* path, uint64_t mode){
    return _syscall(2, 0, (uint64_t)path, mode, 0, 0, 0);
}
/*
 * System call: Filesystem: Open file
 */
inline sc_state_t _fs_read_bytes(FILE* file, void* buf, size_t len){
    return _syscall(2, 1, (uint64_t)file, (uint64_t)buf, len, 0, 0);
}


// -----===== FILE I/O =====-----

/*
 * Open a file
 */
FILE* fopen(const char* filename, const char* mode){
    //Parse mode
    uint64_t m = 0;
    if(strcmp(mode, "r") == 0 || strcmp(mode, "rb") == 0)
        m = FS_MODE_READ;
    else if(strcmp(mode, "w") == 0 || strcmp(mode, "wb") == 0)
        m = FS_MODE_WRITE;
    else if(strcmp(mode, "a") == 0 || strcmp(mode, "ab") == 0)
        m = FS_MODE_APPEND;
    else //invalid mode
        return NULL;
    //Make a system call
    sc_state_t state = _fs_open((char*)filename, m);
    //Parse the state
    if(state >= 0xFF)
        return (FILE*)state;
    else
        return NULL;
}

/*
 * Read file contents into buffer
 */
size_t fread(void* ptr, size_t size_of_elements, size_t number_of_elements, FILE* a_file){
    //Make a system call
    sc_state_t state = _fs_read_bytes(a_file, ptr, size_of_elements * number_of_elements);
    //Parse the state
    uint64_t act_size = size_of_elements * number_of_elements;
    switch(state >> 32){
        case FS_RD_STATUS_EOF:
            act_size = (uint32_t)state;
        case FS_STATUS_OK:
            return act_size;
        default:
            return 0;
    }
}

/*
 * Read one character from file
 */
int fgetc(FILE* fp){
    //Read one byte
    char c;
    //Return -1 on error
    if(fread(&c, 1, 1, fp) == 0)
        return -1;
    else
        return c;
}

/*
 * Read \n- or EOF-terminated string from file
 */
char* fgets(char* buf, int n, FILE* fp){
    //State variables
    uint64_t cnt = 0;
    int c = 0;
    //Read data
    while((c != 1) && (cnt < n - 1)){
        c = fgetc(fp);
        switch(c){
            case '\n': //stop on newline
                buf[cnt++] = '\n';
            case -1: //stop on EOF
                buf[cnt] = 0;
                return buf;
            default:
                buf[cnt++] = c;
        }
    }
}


// -----===== ORDINARY FUNCTIONS =====-----


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
    //Find the minimal one
    int min_len = (len1 < len2) ? len1 : len2;
    //Go through each byte
    for(int i = 0; i < min_len; i++){
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
    if(num == 0)
        return destination;
    for(uint64_t i = 0; i < num; i++)
        *(uint8_t*)((uint64_t)destination + i) = *(uint8_t*)((uint64_t)source + i);
    return destination;
}

/*
 * Copy a string to other string
 */
char* strcpy(char* dest, char* src){
    memcpy(dest, src, strlen(src));
}

/*
 * Find the first occurence of a character in a block of memory
 */
void* memchr(const void* str, int c, size_t n){
    char chr;
    uint64_t cnt = 0;
    //Walk through the string
    while(cnt++ < n){
        //Return the occurence if it was found
        if(chr == c)
            return (void*)((uint64_t)str + cnt);
    }
    //Return NULL if not
    return NULL;
}

/*
 * Find the first occurence of a character in a string
 */
char* strchr(const char* str, int c){
    char chr;
    uint64_t cnt = 0;
    //Walk through the string
    while(chr = *(str + cnt)){
        //Return the occurence if it was found
        if(chr == c)
            return (void*)((uint64_t)str + cnt);
        //Increment the counter
        cnt++;
    }
    //Return NULL if not
    return NULL;
}

/*
 * Find the first occurence of any character from str2 in str1
 */
char* strpbrk(const char *str1, const char *str2){
    char chr, chr2;
    uint64_t cnt = 0, cnt2 = 0;
    //Walk through the string
    while(chr = *(char*)(str1++)){
        //Go through each characther in str2 and compare it against the current character
        while(chr2 = *(char*)(str1 + cnt2++))
            if(chr2 == chr) //Return the occurence
                return (void*)((uint64_t)str1 + cnt);
        //Increment the counter
        cnt++;
    }
    //Return NULL if not
    return NULL;
}

/*
 * Find the first occurence of string needle in string haystack
 */
char* strstr(const char* haystack, const char* needle){
    //For each character in the haystack
    for(uint64_t i = 0; i < strlen(haystack) - strlen(needle); i++){
        //Compare
        if(memcmp(haystack + i, needle, strlen(needle)) == 0)
            return (char*)((uint64_t)haystack + i);
    }
    //Return NULL if not found
    return NULL;
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
 * Parse long number from string representation
 */
long atol(const char* str){
    long n = 0;
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
 * Print an uint64_t to the string
 */
char* _sprintu(char* str, uint64_t i, uint8_t min){
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

char hex_const[16] = "0123456789ABCDEF";

/*
 * Print an uint64_t with base 16 to the string
 */
char* _sprintub16(char* str, uint64_t i, uint8_t min){
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
 * Print formatted string
 */
int sprintf(char* str, const char* format, ...){
    //Get the number of arguments
    uint64_t argcnt = 0;
    for(uint64_t i = 0; i < strlen(format); i++)
        if(format[i] == '%' && (i > 0 && format[i - 1] != '%'))
            argcnt++;
    //Get variable arguments
    va_list valist;
    va_start(valist, argcnt);
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
                    _sprintu(buf, va_arg(valist, uint64_t), 1);
                    for(uint64_t j = 0; j < strlen(buf); j++)
                        str[str_idx++] = buf[j];
                    break;
                }
                case 'p': //hex integer
                case 'x':
                case 'X': {
                    char buf[64];
                    _sprintub16(buf, va_arg(valist, uint64_t), 1);
                    for(uint64_t j = 0; j < strlen(buf); j++)
                        str[str_idx++] = buf[j];
                    break;
                }
                default: //nothing else
                    va_end(valist);
                    return -1;
            }
        } else { //A normal character
            str[str_idx++] = format[i];
        }
    }
    //Add zero termination
    str[str_idx] = 0;
    //Return the amount of characters printed
    va_end(valist);
    return str_idx;
}

/*
 * Abnormal program termination
 */
void abort(void){
    _gfx_println_verbose("*** PROGRAM ABORTED");
    //terminate the current process
    _task_terminate(_task_get_uid());
}

//Function to call upon call of exit()
void (*__atexit_func)(void) = NULL;
/*
 * Normal program termination
 */
void exit(void){
    //Call the atexit function
    if(__atexit_func != NULL)
        __atexit_func();
    //Terminate the current process
    _task_terminate(_task_get_uid());
}

/*
 * Assign the function yo be called by exit()
 */
int atexit(void (*func)(void)){
    __atexit_func = func;
    return 0;
}

/*
 * Returns the absolute value of a number
 */
int abs(int x){
    if(x >= 0)
        return x;
    else
        return -x;
}

//Random number generator state
uint64_t __r_x = 0, __r_w = 0, __r_s = 0x65c5f23086039ca8;
/*
 * Returns a presudo-random number (Middle Square Weyl Sequence algorithm)
 */
int rand(void){
    __r_x *= __r_x;
    __r_x += (__r_w += __r_s);
    return __r_x = (__r_x >> 32) | (__r_x << 32);
}

/*
 * Sets random number generation seed
 */
void srand(unsigned int seed){
    __r_s = seed;
    __r_s |= (uint64_t)rand() << 32;
}