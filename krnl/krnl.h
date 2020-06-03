#ifndef KRNL_H
#define KRNL_H

#include "./stdlib.h"

#include <efi.h>
#include <efilib.h>

//Definitions

#define MSR_IA32_EFER                       0xC0000080
#define MSR_IA32_STAR                       0xC0000081
#define MSR_IA32_LSTAR                      0xC0000082
#define MSR_IA32_SFMASK                     0xC0000084

#define MAX_KRNL_MSG_SZ                     256

//Kernel version
#define KRNL_VERSION_STR "v0.6.1"
#define KRNL_VERSION_NUM 2

//Structures

//The struct that shows where in memory the kernel is loaded
typedef struct {
    uint64_t offset;
    uint64_t size;
} krnl_pos_t;

//Kernel message struct
typedef struct _krnl_msg_s {
    uint64_t tsc;
    char file[64];
    char msg[MAX_KRNL_MSG_SZ];
    struct _krnl_msg_s* next;
} krnl_msg_t;

//Function prototypes

void krnl_write_msg(char* file, char* msg);
void krnl_write_msgf(char* file, char* msg, ...);

EFI_SYSTEM_TABLE* krnl_get_efi_systable(void);
krnl_pos_t krnl_get_pos(void);

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable);

#endif