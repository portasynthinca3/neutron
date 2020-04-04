//Neutron Project
//ELF support

#include "./elf.h"
#include "../../stdlib.h"
#include "../../drivers/diskio.h"
#include "../../drivers/gfx.h"
#include "../../vmem/vmem.h"
#include "../../mtask/mtask.h"

/*
 * Loads and executes an ELF file
 */
uint8_t elf_load(char* path, uint8_t debug){
    if(debug) gfx_verbose_println("ELF: opening file");
    //Open the file
    file_handle_t file;
    if(diskio_open(path, &file, DISKIO_FILE_ACCESS_READ) != DISKIO_STATUS_OK)
        return ELF_STATUS_FILE_INACCESSIBLE;
    //Read the ELF header
    union {
        uint8_t raw[sizeof(elf_hdr_t)];
        elf_hdr_t hdr;
    } elf_hdr;
    diskio_read(&file, elf_hdr.raw, sizeof(elf_hdr_t));
    if(debug){
        if(elf_hdr.hdr.bits == 2)
            gfx_verbose_println("ELF bits: 64");
        if(elf_hdr.hdr.bits == 1)
            gfx_verbose_println("ELF bits: 32");
        char temp[50] = "ELF arch: 0x";
        char temp2[20];
        gfx_verbose_println(strcat(temp, sprintub16(temp2, elf_hdr.hdr.instruction_set, 2)));
    }
    //Check the format (ELF32/ELF64)
    if(elf_hdr.hdr.bits != 2)
        return ELF_STATUS_INCOMPATIBLE;
    //Check the architecture (should be x86-64)
    if(elf_hdr.hdr.instruction_set != 0x3E)
        return ELF_STATUS_INCOMPATIBLE;
    //Get the .shrtrtab section offset
    file.position = elf_hdr.hdr.sec_hdr_table_pos + (elf_hdr.hdr.sect_names_idx * elf_hdr.hdr.sect_hdr_entry_sz) + 24;
    uint64_t shrtrtab_offs = 0;
    diskio_read(&file, (void*)&shrtrtab_offs, 8);
    if(debug){
        char temp[50] = "ELF shrtrtab_offs: 0x";
        char temp2[20];
        gfx_verbose_println(strcat(temp, sprintub16(temp2, shrtrtab_offs, 1)));
    }
    //Create a virtual memory space
    uint64_t cr3 = vmem_create_pml4(vmem_create_pcid());
    //Go through the sections
    for(uint32_t i = 0; i < elf_hdr.hdr.sect_hdr_entry_cnt; i++){
        uint32_t offs = elf_hdr.hdr.sec_hdr_table_pos + (i * elf_hdr.hdr.sect_hdr_entry_sz);
        //Read the data
        union {
            uint8_t raw[64];
            struct {
                uint32_t name_offs;
                uint32_t type;
                uint64_t flags;
                uint64_t addr;
                uint64_t offs;
                uint64_t size;
             } __attribute__((packed)) hdr;
        } sect_hdr;
        file.position = offs;
        diskio_read(&file, sect_hdr.raw, elf_hdr.hdr.sect_hdr_entry_sz);
        //Display debug info
        if(debug){
            gfx_verbose_println("");
            char buf[128] = "ELF section: ";
            char buf2[32] = "";
            file.position = shrtrtab_offs + sect_hdr.hdr.name_offs;
            diskio_read(&file, buf2, 32);
            gfx_verbose_println(strcat(buf, buf2));
            memcpy(buf, "ELF section offset: 0x", strlen("ELF section offset: 0x") + 1);
            gfx_verbose_println(strcat(buf, sprintub16(buf2, sect_hdr.hdr.offs, 1)));
            memcpy(buf, "ELF section size: 0x", strlen("ELF section size: 0x") + 1);
            gfx_verbose_println(strcat(buf, sprintub16(buf2, sect_hdr.hdr.size, 1)));
            memcpy(buf, "ELF section type: 0x", strlen("ELF section type: 0x") + 1);
            gfx_verbose_println(strcat(buf, sprintub16(buf2, sect_hdr.hdr.type, 1)));
            memcpy(buf, "ELF section flags: 0x", strlen("ELF section flags: 0x") + 1);
            gfx_verbose_println(strcat(buf, sprintub16(buf2, sect_hdr.hdr.flags, 1)));
        }
        //Load the section in memory if it's executable
        if(sect_hdr.hdr.flags & 0x4){
            //Allocate the memory
            uint8_t* app_text = (uint8_t*)malloc(sect_hdr.hdr.size);
            //Copy the data
            file.position = sect_hdr.hdr.offs;
            diskio_read(&file, app_text, sect_hdr.hdr.size);
            //Map the data as required by the ELF file
            vmem_map(cr3, app_text, app_text + sect_hdr.hdr.size, (void*)sect_hdr.hdr.addr);
            //Display debug info
            if(debug){
                char buf[128] = "ELF: this section is loaded at physaddr 0x";
                char buf2[32] = "";
                gfx_verbose_println(strcat(buf, sprintub16(buf2, (uint64_t)app_text, 1)));
            }
        }
    }
}