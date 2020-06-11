//Neutron project
//Standard library for userland applications

#include "nlib.h"

/*
 * Issues a system call
 */
uint64_t _syscall(uint32_t func, uint32_t subfunc,
                         uint64_t p0, uint64_t p1, uint64_t p2, uint64_t p3, uint64_t p4){
    //Calculate RDI value
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
    return ret;
}


// -----===== SYSTEM CALLS: TASK MANAGEMENT =====-----


/*
 * System call: Task management: Get task PID
 */
inline uint64_t _task_get_pid(void){
    return _syscall(1, 0, 0, 0, 0, 0, 0);
}

/*
 * System call: Task management: Terminate task
 */
inline sc_state_t _task_terminate(uint64_t pid){
    return _syscall(1, 1, pid, 0, 0, 0, 0);
}

/*
 * System call: Task management: Load executable
 */
inline uint64_t _task_load(char* path, uint64_t privl){
    return _syscall(1, 2, (uint64_t)path, privl, 0, 0, 0);
}

/*
 * System call: Task management: Allocate pages
 */
inline void* _task_palloc(uint64_t num){
    return (void*)_syscall(1, 3, num, 0, 0, 0, 0);
}

/*
 * System call: Task management: Free pages
 */
inline sc_state_t _task_pfree(void* start){
    return _syscall(1, 4, (uint64_t)start, 0, 0, 0, 0);
}


// -----===== SYSTEM CALLS: FILESYSTEM =====-----


/*
 * System call: Filesystem: Open file
 */
inline sc_state_t _fs_open(char* path, uint64_t mode){
    return _syscall(2, 0, (uint64_t)path, mode, 0, 0, 0);
}

/*
 * System call: Filesystem: Read bytes
 */
inline sc_state_t _fs_read_bytes(FILE* file, void* buf, size_t len){
    return _syscall(2, 1, (uint64_t)file, (uint64_t)buf, len, 0, 0);
}

/*
 * System call: Filesystem: Write bytes
 */
inline sc_state_t _fs_write_bytes(FILE* file, void* buf, size_t len){
    return _syscall(2, 2, (uint64_t)file, (uint64_t)buf, len, 0, 0);
}

/*
 * System call: Filesystem: Seek
 */
inline sc_state_t _fs_seek(FILE* file, uint64_t pos){
    return _syscall(2, 3, (uint64_t)file, pos, 0, 0, 0);
}

/*
 * System call: Filesystem: Close file
 */
inline sc_state_t _fs_close(FILE* file){
    return _syscall(2, 4, (uint64_t)file, 0, 0, 0, 0);
}


// -----===== SYSTEM CALLS: KERNEL MESSAGES =====-----


/*
 * System call: Kernel messages: Write message
 */
inline sc_state_t _km_write(char* file, char* msg){
    return _syscall(3, 0, (uint64_t)file, (uint64_t)msg, 0, 0, 0);
}


// -----===== FILE I/O =====-----


/*
 * Open a file
 */
FILE* fopen(const char* filename, const char* mode){
    //Parse mode
    uint64_t m = 0;
    if(strcmp(mode, "r+") == 0)
        m = FS_MODE_READ | FS_MODE_WRITE;
    else if(strcmp(mode, "r") == 0 || strcmp(mode, "rb") == 0)
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
 * Write buffer contents to file
 */
size_t fwrite(const void* ptr, size_t size_of_elements, size_t number_of_elements, FILE* a_file){
    //Make a system call
    sc_state_t state = _fs_write_bytes(a_file, (void*)ptr, size_of_elements * number_of_elements);
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
    volatile char c = 0;
    //Return -1 on error
    if(fread((char*)&c, 1, 1, fp) == 0)
        return -1;
    return (uint8_t)c;
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
    return buf;
}

/*
 * Write character to file
 */
int fputc(volatile int c, FILE* fp){
    //Check character range
    if(c < 0 || c > 255)
        return -1;
    //Write byte and check status
    if(fwrite((int*)&c, 1, 1, fp) != 0)
        return c;
    else
        return -1;
}

/*
 * Write zero-terminated string to file
 */
int fputs(const char* s, FILE* fp){
    //Write bytes and check status
    if(fwrite(s, 1, strlen(s), fp) != 0)
        return 1;
    else
        return -1;
}

/*
 * Seek to absolute file position
 */
int fseek(FILE* fp, uint64_t offs){
    if(_fs_seek(fp, offs) == FS_STATUS_OK)
        return 0;
    else
        return -1;
}

/*
 * Close the file
 */
int fclose(FILE* fp){
    if(_fs_close(fp) == FS_STATUS_OK)
        return 0;
    else
        return -1;
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
    int len1 = strlen(str1) + 1;
    int len2 = strlen(str2) + 1;
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
    return memcpy(dest, src, strlen(src) + 1);
}

/*
 * Find the first occurence of a character in a block of memory
 */
void* memchr(const void* str, int c, size_t n){
    char chr = 0;
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
    while((chr = *(str + cnt))){
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
    while((chr = *(char*)(str1++))){
        //Go through each characther in str2 and compare it against the current character
        while((chr2 = *(char*)(str1 + cnt2++)))
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
 * Prints floating-point value to the string
 */
char* _sprintd(char* str, double val){
    uint8_t str_pos = 0;
    uint64_t bits = *(uint64_t*)&val;
    //Negate the value and print a minus sign if the value is negative
    if(val < 0){
        val = -val;
        str[str_pos++] = '-';
    }
    //Extract the exponent and mantissa
    int16_t exponent = ((bits >> 52) & 0x7FF) - 1023;
    uint64_t mantissa = bits & 0xFFFFFFFFFFFFF;
    _sprintu(str + str_pos, mantissa, 1);
    strcat(str, "e");
    if(exponent < 0){
        exponent = -exponent;
        strcat(str, "-");
    }
    _sprintu(str + strlen(str), exponent, 1);
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
                case 'f': { //floating point number
                    char buf[64] = "\0";
                    _sprintd(buf, va_arg(valist, double));
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
    //terminate the current process
    _task_terminate(_task_get_pid());
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
    _task_terminate(_task_get_pid());
}

/*
 * Assign the function to be called by exit()
 */
int atexit(void (*func)(void)){
    __atexit_func = func;
    return 0;
}

/*
 * Returns the absolute value of x
 */
int abs(int x){
    if(x >= 0)
        return x;
    return -x;
}

/*
 * Returns the minimum of two values
 */
int min(int a, int b){
    if(a < b)
        return a;
    return b;
}

/*
 * Returns the maximum of two values
 */
int max(int a, int b){
    if(a > b)
        return a;
    return b;
}

/*
 * Returns the arc cosine of x (in radians)
 */
double acos(double x){
    return atan2(sqrt((1.0 + x) * (1.0 - x)), x);
}

/*
 * Returns the arc sine of x (in radians)
 */
double asin(double x){
    return atan2(x, sqrt((1.0 + x) * (1.0 - x)));
}

/*
 * Returns the arc tangent of x (in radians)
 */
double atan(double x){
    return atan2(1, x);
}

/*
 * Returns the arc tangent of y/x (in radians)
 */
double atan2(double y, double x){
    asm("fld %1; fld %2; fpatan; fstp %0;" : "=m"(x) : "m"(y), "m"(x));
    return x;
}

/*
 * Returns the cosine of x (x in radians)
 */
double cos(double x){
    return sin(x + (M_PI / 2));
}

/*
 * Returns the sine of x (x in radians)
 */
double sin(double x){
    //black magic
    float xx = x * x;
    return x + (x * xx) * (-0.16612511580269618f + xx * (8.0394356072977748e-3f + xx * -1.49414020045938777495e-4f));
}

/*
 * Returns the exponent of x
 */
double exp(double x){
    //TODO
    /*
    double tmp;
    asm("fld %1;"
        "fldl2e;"
        "fist %2;"
        "fild %2;"
        "fsub;"
        "f2xm1;"
        "fld1;"
        "fadd;"
        "fild %2;"
        "fxch;"
        "fscale;"
        "fstp %0" :
        "=m"(x) :
        "m"(x), "m"(tmp));
        */
    return x;
}

/*
 * Splits the integer and decimal part of a number
 */
double modf(double x, double* integer){
    double tmp;
    asm("fld %1; fist %2; fsub %2; fstp %0" : "=m"(x) : "m"(x), "m"(tmp));
    if(integer != NULL)
        *integer = tmp;
    return x - tmp;
}

/*
 * Returns x raised to the power of y
 */
double pow(double x, double y){
    //TODO
    /*
    double tmp;
    asm("fld %3;"
        "fld %1;"
        "fyl2x;"
        "fist %2;"
        "fild %2;"
        "fsub;"
        "f2xm1;"
        "fld1;"
        "fadd;"
        "fild %2;"
        "fxch;"
        "fscale;"
        "fstp %0" :
        "=m"(x) :
        "m"(x), "m"(tmp), "m"(y));
        */
    return x;
}

/*
 * Returns the square root of x
 */
double sqrt(double x){
    asm("fld %1; fsqrt; fstp %0;" : "=m"(x) : "m"(x));
    return x;
}

/*
 * Returns the ceiling-rounded value of x
 */
double ceil(double x){
    double i;
    double f = modf(x, &i);
    if(f < DBL_EPSILON)
        return i;
    return i + 1;
}

/*
 * Returns the absolute value of x
 */
double fabs(double x){
    if(x < 0)
        return -x;
    return x;
}

/*
 * Returns the remainder of x/y
 */
double fmod(double x, double y){
    return modf(x / y, NULL);
}

/*
 * Returns the floor-rounded value of x
 */
double floor(double x){
    double i;
    modf(x, &i);
    return i;
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

/*
 * Allocates a chunk of memory
 */
void* malloc(uint64_t num){
    if(num == 0)
        return NULL;
    //Round to the page size
    num += 4096 - (num % 4096);
    //Allocate pages
    return _task_palloc(num / 4096);
}

/*
 * Frees a chunk of memory
 */
void free(void* ptr){
    _task_pfree(ptr);
}

/*
 * Read the amount of cycles executed by the CPU
 */
uint64_t rdtsc(void){
    uint32_t h, l;
    __asm__ volatile("mfence; lfence; rdtsc;" : "=d" (h), "=a" (l));
    return (uint64_t)((uint64_t)h << 32) | l;
}

/*
 * Convert big endian doubleword to little endian one and vice versa
 */
void bswap_dw(uint32_t* value){
    __asm__("bswapl %%eax" : "=a" (*value) : "a" (*value));
}

/*
 * Returns the node pointed for that index
 */
ll_node_t* _ll_get_node(ll_t* list, uint64_t idx){
    if(idx == 0){
        return list->first;
    } else if(idx == ll_size(list) - 1){
        return list->last;
    } else {
        ll_node_t* cur_node = list->first;
        uint64_t   cur_idx  = 0;
        while((cur_node) && (cur_node = cur_node->next) && (++cur_idx <= idx));
        cur_node = cur_node->prev;
        return cur_node;
    }
}

/*
 * Creates a linked list
 */
ll_t* ll_create(void){
    //Create and initialize the list
    ll_t* list = (ll_t*)malloc(sizeof(ll_t));
    list->first = NULL;
    list->last = NULL;
    list->size = 0;
    list->cur_iter = NULL;
    list->iter_dir = LL_ITER_DIR_UP;
    return list;
}

/*
 * Destroys the linked lists
 */
void ll_destroy(ll_t* list){
    while(list->size != 0)
        ll_remove(list, 0);
    free(list);
}

/*
 * Insert an item into the list
 */
void ll_insert(ll_t* list, void* item, uint64_t idx){
    //Create and initialize the node
    ll_node_t* node = malloc(sizeof(ll_node_t));
    node->item = item;
    node->next = NULL;
    node->prev = NULL;
    //Get the node that corresponds to that index
    ll_node_t* cur_node = _ll_get_node(list, idx);
    //Link everything together
    node->prev = cur_node->prev;
    node->next = cur_node;
    cur_node->prev = node;
    if(node->prev != NULL)
        node->prev->next = node;
    if(idx == 0)
        list->first = node;
    //Increase the size
    list->size++;
}

/*
 * Sets the element at the specified index
 */
void ll_set(ll_t* list, void* item, uint64_t idx){
    _ll_get_node(list, idx)->item = item;
}

/*
 * Appends a value to the linked list
 */
void ll_append(ll_t* list, void* item){
    //Create and initialize the node
    ll_node_t* node = malloc(sizeof(ll_node_t));
    node->item = item;
    node->next = NULL;
    //Place it in the list and update the link to the previous node
    if(list->first == NULL){
        list->first = list->last = node;
        node->prev = NULL;
    } else {
        node->prev = list->last;
        list->last->next = node;
        list->last = node;
    }
    list->size++;
}

/*
 * Removes the element at the specified index
 */
void ll_remove(ll_t* list, uint64_t idx){
    ll_node_t* node = _ll_get_node(list, idx);
    //Link the previous and last elements
    if(node->prev != NULL)
        node->prev->next = node->next;
    if(node->next != NULL)
        node->next->prev = node->prev;
    //Free the memory used by the node
    free(node);
    //Decrease the list size
    list->size--;
}

/*
 * Swaps two items in a list
 */
void ll_swap(ll_t* list, int64_t idx1, int64_t idx2){
    //Get nodes that correspond to these indicies
    ll_node_t* n1 = _ll_get_node(list, idx1);
    ll_node_t* n2 = _ll_get_node(list, idx2);
    //Swap the items
    void* tmp = n1->item;
    n1->item = n2->item;
    n2->item = tmp;
}

/*
 * Returns the number of items in the list
 */
uint64_t ll_size(ll_t* list){
    return list->size;
}

/*
 * Gets the element at the specified index
 */
void* ll_get(ll_t* list, uint64_t idx){
    return _ll_get_node(list, idx)->item;
}

/*
 * Iterate through the list
 */
void* ll_iter(ll_t* list, uint8_t dir){
    list->iter_dir = dir;
    //Get the next node and item
    void* item;
    if(list->cur_iter == NULL)
        list->cur_iter = (dir == LL_ITER_DIR_UP) ? list->first : list->last;
    else 
        list->cur_iter = (dir == LL_ITER_DIR_UP) ? list->cur_iter->next : list->cur_iter->prev;
    ll_node_t* node = list->cur_iter;
    if(node == NULL){
        item = NULL;
        node = NULL;
    } else {
        item = node->item;
    }
    //Update the current iteration node
    list->cur_iter = node;
    //Return the item pointer
    return item;
}

/*
 * Finds a node with the specified key
 */
dict_node_t* _dict_get_node(dict_t* dict, char* key){
    dict_node_t* node = NULL;
    //Scan through the dictionary
    while((node = (dict_node_t*)ll_iter(dict, LL_ITER_DIR_UP)))
        if(strcmp(node->key, key) == 0)
            break;
    dict->cur_iter = NULL;
    return node;
}

/*
 * Creates a dictionary
 */
dict_t* dict_create(void){
    return ll_create();
}

/*
 * Destroys a dictionary
 */
void dict_destroy(dict_t* dict){
    ll_destroy(dict);
}

/*
 * Sets a value in the dictionary
 */
void dict_set(dict_t* dict, char* key, void* val){
    //Simply set the value if the key already exists
    dict_node_t* node = _dict_get_node(dict, key);
    if(node != NULL){
        node->val = val;
        return;
    }
    //Create a new node else
    node = (dict_node_t*)malloc(sizeof(dict_node_t));
    strcpy(node->key, key);
    node->val = val;
    ll_append(dict, node);
}

/*
 * Gets a value from the dictionary
 */
void* dict_get(dict_t* dict, char* key){
    dict_node_t* node = _dict_get_node(dict, key);
    if(node == NULL)
        return NULL;
    return node->val;
}