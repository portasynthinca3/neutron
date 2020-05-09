#ifndef KRNL_H
#define KRNL_H

#include "./stdlib.h"

#include <efi.h>
#include <efilib.h>

//Definitions

//Kernel version
#define KRNL_VERSION_STR "v0.6.0"

//Structures

//The struct that shows where in memory the kernel is loaded
typedef struct {
    uint64_t offset;
    uint64_t size;
} krnl_pos_t;

//Function prototypes

EFI_SYSTEM_TABLE* krnl_get_efi_systable(void);
krnl_pos_t krnl_get_pos(void);

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable);

#endif