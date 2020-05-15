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
 * Copy a block of memory
 */
void* memcpy(void* destination, const void* source, size_t num){
    if(num == 0)
        return destination;
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
 * Get the length of a zero-terminated string
 */
size_t strlen(const char* str){
    size_t i = 0;
    while(str[i++] != 0);
    return i - 1;
}