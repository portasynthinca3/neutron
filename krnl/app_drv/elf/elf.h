#include "../../stdlib.h"
#include "../../mtask/mtask.h"

//Definitions

#define ELF_STATUS_OK                       0
#define ELF_STATUS_FILE_INACCESSIBLE        1
#define ELF_STATUS_INCOMPATIBLE             2
#define ELF_STATUS_ESCALATION_ERROR         3

//Structures

typedef struct {
    uint32_t signature;
    uint8_t bits;
    uint8_t endianness;
    uint8_t hdr_version;
    uint8_t os_abi;
    uint8_t padding[8];
    uint16_t type;
    uint16_t instruction_set;
    uint32_t elf_version;
    uint64_t entry_pos;
    uint64_t pgm_hdr_table_pos;
    uint64_t sec_hdr_table_pos;
    uint32_t flags;
    uint16_t hdr_size;
    uint16_t pgm_hdr_entry_sz;
    uint16_t pgm_hdr_entry_cnt;
    uint16_t sect_hdr_entry_sz;
    uint16_t sect_hdr_entry_cnt;
    uint16_t sect_names_idx;
} __attribute__((packed)) elf_hdr_t;

typedef struct {
    uint32_t name;
    uint8_t  info;
    uint8_t  other;
    uint16_t shndx;
    uint64_t val;
    uint64_t size;
} __attribute__((packed)) elf_sym_t;

//Function prototypes

uint8_t elf_load(char* path, uint64_t privl, uint8_t prio);
void elf_get_sym(task_t* task, uint64_t addr, char* result);