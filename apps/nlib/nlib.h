//Neutron project
//Standard library for userland projects

#pragma once

#include <stdarg.h>

//Settings
#define ALLOC_STEP  (1024 * 1024)
#define ALLOC_ALIGN 4096

//Definitions
#define NLIB_VERSION "1.0.0"

#define NULL        ((void*)0)
#define true        1
#define false       0

#define CHAR_BIT    8
#define SCHAR_MIN   -128
#define SCHAR_MAX   127
#define UCHAR_MAX   255
#define CHAR_MIN    SCHAR_MIN
#define CHAR_MAX    SCHAR_MAX
#define MB_LEN_MAX  16
#define SHRT_MIN    -32768
#define SHRT_MAX    32767
#define USHRT_MAX   65536
#define INT_MIN     -2147483648
#define INT_MAX     2147483648
#define UINT_MAX    4294967295
#define LONG_MIN    -9223372036854775808
#define LONG_MAX    9223372036854775807
#define ULONG_MAX   18446744073709551615
#define DBL_EPSILON 1E-9
#define M_PI        3.14159265358979323846

#define EOF          -1
#define FOPEN_MAX    256
#define FILENAME_MAX 256
//#define stdout       (FILE*)1
//#define stdin        (FILE*)2
//#define stderr       (FILE*)3

#define LL_ITER_DIR_UP   0
#define LL_ITER_DIR_DOWN 1

//Macros
//Assertion macro
#define assert(expr) do { if(!(expr)) abort(); } while(0);

//Standrard type definitions

typedef unsigned char uint8_t;
typedef signed char int8_t;
typedef unsigned short uint16_t;
typedef signed short int16_t;
typedef unsigned int uint32_t;
typedef signed int int32_t;
typedef long long unsigned int uint64_t;
typedef long long signed int int64_t;
typedef uint64_t size_t;
typedef uint64_t sc_state_t;
typedef void FILE;

//Structures

//An allocable or allocated block of memory
typedef struct _alloc_block_s {
    uint8_t                used;
    struct _alloc_block_s* prev;
    struct _alloc_block_s* next;
    size_t                 size;
} alloc_block_t;

//Linked list node
typedef struct _ll_node_s {
    void*              item;
    struct _ll_node_s* prev;
    struct _ll_node_s* next;
} ll_node_t;

//Linked list
typedef struct {
    ll_node_t* first;
    ll_node_t* last;

    uint64_t   size;

    ll_node_t* cur_iter;
    uint8_t    iter_dir;
} ll_t;

//Dictionary key-value pair
typedef struct {
    char  key[128];
    void* val;
} dict_node_t;

//Dictionary
typedef ll_t dict_t;

//Function prototypes

//System calls
//General syscall function
uint64_t _syscall(uint32_t func, uint32_t subfunc,
                  uint64_t p0, uint64_t p1, uint64_t p2, uint64_t p3, uint64_t p4);
//Syscalls: Task management
uint64_t   _task_get_pid   (void);
sc_state_t _task_terminate (uint64_t pid);
uint64_t   _task_load      (char* path, uint64_t privl);
void*      _task_palloc    (uint64_t num);
sc_state_t _task_pfree     (void* start);
#define    ELF_STATUS_OK                    0
#define    ELF_STATUS_FILE_INACCESSIBLE     1
#define    ELF_STATUS_INCOMPATIBLE          2
#define    TASK_PRIVL_EVERYTHING            (0xFFFFFFFFFFFFFFFFULL & ~TASK_PRIVL_INHERIT & ~TASK_PRIVL_SUDO_MODE)
#define    TASK_PRIVL_INHERIT               (1ULL << 63)
#define    TASK_PRIVL_KMESG                 (1ULL << 0)
#define    TASK_PRIVL_SUDO_MODE             (1ULL << 1)
#define    TASK_PRIVL_SYSFILES              (1ULL << 2)
#define    TASK_PRIVL_DEVFILES              (1ULL << 3)
//Syscalls: Filesystem
sc_state_t _fs_open       (char* path, uint64_t mode);
sc_state_t _fs_read_bytes (FILE* file, void* buf, size_t len);
#define    FS_MODE_READ                     1
#define    FS_MODE_WRITE                    2
#define    FS_MODE_APPEND                   4
#define    FS_STATUS_OK                     0
#define    FS_STATUS_FILE_DOESNT_EXIST      1
#define    FS_STATUS_MODE_NOT_APPLICABLE    2
#define    FS_STATUS_FILE_BUSY              3
#define    FS_RD_STATUS_INVL_PTR            4
#define    FS_RD_STATUS_ERROR               5
#define    FS_RD_STATUS_EOF                 6
//Syscalls: Kernel messages
sc_state_t _km_write (char* file, char* msg);
//File I/O
FILE*  fopen  (const char* filename, const char* mode);
int    fgetc  (FILE* fp);
char*  fgets  (char* buf, int n, FILE* fp);
int    fputc  (int c, FILE* fp);
int    fputs  (const char* s, FILE* fp);
int    fseek  (FILE* fp, uint64_t offs);
int    fclose (FILE* fp);
size_t fread  (void* ptr, size_t size_of_elements, size_t number_of_elements, FILE* a_file);
size_t fwrite (const void *ptr, size_t size_of_elements, size_t number_of_elements, FILE *a_file);
//String/memory operations
char*  strcat      (char* dest, char* src);
int    memcmp      (const void* lhs, const void* rhs, size_t cnt);
int    strcmp      (const char* str1, const char* str2);
void*  memset      (void* dst, int ch, size_t size);
void*  memcpy      (void* destination, const void* source, size_t num);
char*  strcpy      (char* dest, char* src);
void*  memchr      (const void* str, int c, size_t n);
char*  strchr      (const char* str, int c);
char*  strpbrk     (const char *str1, const char *str2);
char*  strstr      (const char* haystack, const char* needle);
size_t strlen      (const char* str);
int    atoi        (const char* str);
long   atol        (const char* str);
char*  _sprintu    (char* str, uint64_t i, uint8_t min);
char*  _sprintub16 (char* str, uint64_t i, uint8_t min);
char*  _sprintd    (char* str, double val);
int    sprintf     (char* str, const char* format, ...);
//Flow control
void   abort  (void);
void   exit   (void);
int    atexit (void (*func)(void));
//Maths
int    abs   (int x);
int    min   (int a, int b);
int    max   (int a, int b);
double acos  (double x);
double asin  (double x);
double atan  (double x);
double atan2 (double y, double x);
double cos   (double x);
double sin   (double x);
double exp   (double x);
double modf  (double x, double* integer);
double pow   (double x, double y);
double sqrt  (double x);
double ceil  (double x);
double fabs  (double x);
double floor (double x);
double fmod  (double x, double y);
int    rand  (void);
void   srand (unsigned int seed);
//Memory control
void* malloc (size_t sz);
void  free   (void* ptr);
//Raw CPU instructions
uint64_t rdtsc    (void);
void     bswap_dw (uint32_t* value);
//Linked list operations
ll_t*    ll_create  (void);
void     ll_destroy (ll_t* list);
void     ll_insert  (ll_t* list, void* item, uint64_t idx);
void     ll_set     (ll_t* list, void* item, uint64_t idx);
void     ll_append  (ll_t* list, void* item);
void     ll_remove  (ll_t* list, uint64_t idx);
void     ll_swap    (ll_t* list, int64_t idx1, int64_t idx2);
uint64_t ll_size    (ll_t* list);
void*    ll_get     (ll_t* list, uint64_t idx);
void*    ll_iter    (ll_t* list, uint8_t dir);
//Dictionary operations
dict_t* dict_create  (void);
void    dict_destroy (dict_t* dict);
void    dict_set     (dict_t* dict, char* key, void* val);
void*   dict_get     (dict_t* dict, char* key);