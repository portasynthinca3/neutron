//Neutron Project
//ELF support

#include "./elf.h"
#include "../../krnl.h"
#include "../../stdlib.h"
#include "../../drivers/diskio.h"
#include "../../drivers/gfx.h"
#include "../../vmem/vmem.h"
#include "../../mtask/mtask.h"

/*
 * Loads and executes an ELF file
 */
uint8_t elf_load(char* path, uint64_t privl, uint8_t prio){
    //Check requested privileges
    if(mtask_get_pid() > 0)
        if(popcnt(privl ^ mtask_get_by_pid(mtask_get_pid())->privl) > 0)
            return ELF_STATUS_ESCALATION_ERROR;
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
    //Check the format (ELF32/ELF64)
    if(elf_hdr.hdr.bits != 2)
        return ELF_STATUS_INCOMPATIBLE;
    //Check the architecture (should be x86-64)
    if(elf_hdr.hdr.instruction_set != 0x3E)
        return ELF_STATUS_INCOMPATIBLE;
    //Check the type (should be EXECUTABLE)
    if(elf_hdr.hdr.type != 2)
        return ELF_STATUS_INCOMPATIBLE;
    //Create a virtual memory space
    uint64_t cr3 = vmem_create_pml4(vmem_create_pcid());
    vmem_map_defaults(cr3);
    //Get the .shrtrtab section offset
    file.position = elf_hdr.hdr.sec_hdr_table_pos + (elf_hdr.hdr.sect_names_idx * elf_hdr.hdr.sect_hdr_entry_sz) + 24;
    uint64_t shrtrtab_offs = 0;
    diskio_read(&file, (void*)&shrtrtab_offs, 8);
    //Some variables to help with loading
    uint64_t next_addr = 0;
    uint8_t* symtab = NULL;
    uint8_t* strtab = NULL;
    uint32_t symtab_link = 0;
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
                uint64_t link;
             } __attribute__((packed)) hdr;
        } sect_hdr;
        file.position = offs;
        diskio_read(&file, sect_hdr.raw, elf_hdr.hdr.sect_hdr_entry_sz);
        //Read section name
        char s_name[32];
        file.position = shrtrtab_offs + sect_hdr.hdr.name_offs;
        diskio_read(&file, s_name, 32);
        //Load the section if needed
        if(sect_hdr.hdr.type == 1 //SHT_PROGBITS
            && sect_hdr.hdr.size > 0){
            //Allocate memory
            uint8_t* addr = (uint8_t*)amalloc(sect_hdr.hdr.size, 4096);
            //Copy the data
            file.position = sect_hdr.hdr.offs;
            diskio_read(&file, addr, sect_hdr.hdr.size);
            //Map the section
            uint64_t target_addr = 0;
            if(sect_hdr.hdr.addr == 0)
                target_addr = next_addr;
            else
                target_addr = sect_hdr.hdr.addr;
            //If this page is already mapped, don't do anything
            //Otherwise, map
            if(!vmem_present_page(cr3, (virt_addr_t)target_addr)){
                vmem_map_user(cr3, (phys_addr_t)vmem_virt_to_phys(vmem_get_cr3(), addr),
                                   (phys_addr_t)((uint64_t)vmem_virt_to_phys(vmem_get_cr3(), addr) + sect_hdr.hdr.size),
                                   (virt_addr_t)target_addr);
            }
            //Advance next address
            if(sect_hdr.hdr.addr == 0) {
                next_addr += sect_hdr.hdr.size;
                next_addr += 4096 - (next_addr % 4096);
            }
        }
        //Load the symbol table
        if(sect_hdr.hdr.type == 2 //SHT_SYMTAB
            && sect_hdr.hdr.size > 0){
            //Copy the data
            symtab = (uint8_t*)malloc(sect_hdr.hdr.size + sizeof(uint32_t));
            file.position = sect_hdr.hdr.offs;
            diskio_read(&file, symtab + sizeof(uint32_t), sect_hdr.hdr.size);
            //Write the section size
            *(uint32_t*)symtab = sect_hdr.hdr.size / sizeof(elf_sym_t);
            //Set the linked section
            symtab_link = sect_hdr.hdr.link;
        }
        //Load the symbol string table
        if(i == symtab_link){
            strtab = (uint8_t*)malloc(sect_hdr.hdr.size);
            file.position = sect_hdr.hdr.offs;
            diskio_read(&file, strtab, sect_hdr.hdr.size);
        }
    }
    //Allocate some memory for the stack and map it
    void* stack = vmem_virt_to_phys(vmem_get_cr3(), calloc(32768, 1));
    vmem_map_user(cr3, stack, (void*)((uint8_t*)stack + 32768), (void*)(1ULL << 46));

    //Create a new task
    mtask_create_task(32768, path, prio, 0, cr3,
        (void*)(1ULL << 46), 1, (void(*)(void*))elf_hdr.hdr.entry_pos, NULL, privl, symtab, strtab);
    return ELF_STATUS_OK;
}

/*
 * Returns the symbol name and offset into that symbol of an address for a task
 */
void elf_get_sym(task_t* task, uint64_t addr, char* result){
    //Check if that task has a symbol and string table in the first place
    if(task->symtab == NULL || task->strtab == NULL){
        strcpy(result, "unknown");
        return;
    }
    //Find the closest symbol
    char symbol_name[256] = "\0";
    uint32_t sym_cnt = *(uint32_t*)task->symtab;
    elf_sym_t* sym_arr = (elf_sym_t*)(task->symtab + sizeof(uint32_t));
    uint64_t closest_diff = 0xFFFFFFFFFFFFFFFF;
    for(int i = 0; i < sym_cnt; i++){
        elf_sym_t* sym = &sym_arr[i];
        if(sym->val <= addr){
            uint64_t diff = addr - sym->val;
            if(diff < closest_diff){
                closest_diff = diff;
                strcpy(symbol_name, (char*)(task->strtab + sym->name));
            }
        }
    }
    //Construct the result string
    sprintf(result, "%s+0x%x", symbol_name, closest_diff);
}