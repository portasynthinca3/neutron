//Neutron project
//ACPI driver

#include <efi.h>
#include <efilib.h>

#include "./acpi.h"
#include "./pit.h"
#include "../stdlib.h"
#include "./gfx.h"

EFI_SYSTEM_TABLE* quark_get_efi_systable(void);

uint32_t acpi_smi_cmd;
uint8_t acpi_en;
uint8_t acpi_dis;
uint32_t acpi_pm1a_ctl;
uint32_t acpi_pm1b_ctl;
uint16_t acpi_slp_typ_a;
uint16_t acpi_slp_typ_b;
uint16_t acpi_slp_en;
uint16_t acpi_sci_en;
uint8_t acpi_pm1_ctl_len;

/*
 * Initializes ACPI
 */
uint32_t acpi_init(void){
    //Find the RSDP
    acpi_rsdp_t* rsdp = acpi_find_rsdp();
    //If no RSDP was found, return
    if(rsdp == NULL){
        gfx_verbose_println("Error: RSDP not found");
        return 0;
    }

    //Fetch RSDT from RSDP
    acpi_rsdt_t* rsdt = (acpi_rsdt_t*)(uint64_t)rsdp->rsdt_ptr;
    //Check if it's valid
    if(!acpi_sdt_checksum(&rsdt->hdr)){
        gfx_verbose_println("Error: RSDP is not valid");
        return 0;
    }

    //Find FADT
    acpi_fadt_t* fadt = rsdt_find(rsdt, "FACP");
    if(fadt == NULL){
        gfx_verbose_println("Error: FADT not found");
        return 0;
    }

    //Check the DSDT
    if(!acpi_sdt_checksum((acpi_sdt_hdr_t*)(uint64_t)fadt->dsdt)){
        gfx_verbose_println("Error: DSDT is not valid");
        return 0;
    }
    //Search for the \_S5 package in DSDT
    char* s5_addr = (char*)(uint64_t)fadt->dsdt + sizeof(acpi_sdt_hdr_t);
    uint32_t dsdt_len = *((uint32_t*)(uint64_t)fadt->dsdt + 1) - sizeof(acpi_sdt_hdr_t);
    while(dsdt_len-- > 0){
        if(memcmp(s5_addr, "_S5_", 4) == 0)
            break;
        s5_addr++;
    }
    //If DSDT was found
    if(dsdt_len > 0){
        //Check if AML structure is valid
        if ((*(s5_addr - 1) == 0x08 || (*(s5_addr - 2) == 0x08 && *(s5_addr - 1) == '\\')) && *(s5_addr + 4) == 0x12){
            s5_addr += 5;
            //Calculate PkgLen size
            s5_addr += ((*s5_addr & 0xC0) >> 6) + 2;

            //True black magic
            if(*s5_addr == 0x0A)
                s5_addr++;
            acpi_slp_typ_a = *(s5_addr) << 10;
            s5_addr++;

            if(*s5_addr == 0x0A)
                s5_addr++;
            acpi_slp_typ_b = *(s5_addr) << 10;

            acpi_smi_cmd = fadt->smi_comm_port;
            acpi_en = fadt->acpi_en;
            acpi_dis = fadt->acpi_dis;

            acpi_pm1a_ctl = fadt->pm1a_ctl_blk;
            acpi_pm1b_ctl = fadt->pm1b_ctl_blk;

            acpi_pm1_ctl_len = fadt->pm1_ctl_len;

            acpi_slp_en = 1 << 13;
            acpi_sci_en = 1;
        }
    }

    gfx_verbose_println("Sending enable commands (this might take a while)");
    //Enable ACPI
    outb(acpi_smi_cmd, acpi_en);
    /*
    uint32_t start = pit_ticks();
    while((pit_ticks() - start <= 1500) &&
          ((inw(acpi_pm1a_ctl) & acpi_sci_en) == 0));
    if(fadt->pm1b_ctl_blk != 0){
        start = pit_ticks();
        while((pit_ticks() - start <= 1500) &&
              ((inw(acpi_pm1b_ctl) & acpi_sci_en) == 0));
    }
    */

    gfx_verbose_println("ACPI successfully initialized");
    
    return 1;
}

/*
 * Send ACPI shutdown signal
 */
void acpi_shutdown(void){
    //Issue the shutdown command
    outw(acpi_pm1a_ctl, acpi_slp_typ_a | acpi_slp_en);
    if(acpi_pm1b_ctl != 0)
        outw(acpi_pm1b_ctl, acpi_slp_typ_a | acpi_slp_en);
}

/*
 * Send ACPI reboot signal
 */
void acpi_reboot(void){
    //Well, it's not so ACPI-y, but it works
    //Actually this method uses the keybord controller to reset
    uint8_t good = 2;
    while(good & 2)
        good = inb(0x64);
    outb(0x64, 0xFE);
}

/*
 * Do checksumming of the ACPI RSDT table
 */
uint8_t acpi_sdt_checksum(acpi_sdt_hdr_t* rsdt){
    //If the sum of all the bytes in the ACPI RSDT
    //  table mod 256 is zero, the table is valid
    uint8_t sum = 0;
    for(uint32_t i = 0; i < rsdt->len; i++){
        uint8_t byte = *(uint8_t*)((uint64_t)rsdt + i);
        sum += byte;
    }
    return sum == 0;
}

/*
 * Finds ACPI RSDT pointer (RSDP) in memory
 */
acpi_rsdp_t* acpi_find_rsdp(void){
    //Search for the pointer in the UEFI config table
    EFI_CONFIGURATION_TABLE* config_table =  quark_get_efi_systable()->ConfigurationTable;
    for(uint32_t i = 0; i < quark_get_efi_systable()->NumberOfTableEntries; i++){
        //If the GUID is ACPI 1.0 RSDP pointer GUID, return it
        if(memcmp(&(config_table[i].VendorGuid), &(EFI_GUID)ACPI_TABLE_GUID, sizeof(EFI_GUID)) == 0)
            return config_table[i].VendorTable;
    }
    return NULL;
}

/*
 * Finds an ACPI table in ACPI RSDT
 */
void* rsdt_find(acpi_rsdt_t* rsdt, char* table){
    //Calculate the number of entries RSDT has
    uint32_t rsdt_entries = (rsdt->hdr.len - sizeof(rsdt->hdr)) / 4;
    //Cycle through each entry
    for(uint32_t e = 0; e < rsdt_entries; e++){
        //Get the SDT header
        acpi_sdt_hdr_t* hdr = (acpi_sdt_hdr_t*)(uint64_t)(rsdt->ptrs + e);
        //Compare its signature with the desired one
        if(*(uint32_t*)(hdr) == *(uint32_t*)(table))
            return (void*)hdr;
    }
    //No tables were found - return null
    return NULL;
}