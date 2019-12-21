#include <efi.h>
#include <efilib.h>

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable){
    SystemTable->ConOut->OutputString(SystemTable->ConOut, L"Hello, UEFI!\n\r");
    while(1);
}