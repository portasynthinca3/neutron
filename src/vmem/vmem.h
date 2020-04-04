#include "../stdlib.h"

//Page Attribute Table MSR
#define MSR_IA32_PAT                0x277

typedef void* virt_addr_t;
typedef void* phys_addr_t;

uint64_t vmem_get_cr3(void);
uint8_t vmem_pcid_supported(void);

void vmem_init(void);

uint64_t vmem_create_pml4(uint16_t pcid);

void vmem_create_pdpt(uint64_t cr3, virt_addr_t at);
uint8_t vmem_present_pdpt(uint64_t cr3, virt_addr_t at);
phys_addr_t vmem_addr_pdpt(uint64_t cr3, virt_addr_t at);

void vmem_create_pd(uint64_t cr3, virt_addr_t at);
uint8_t vmem_present_pd(uint64_t cr3, virt_addr_t at);
phys_addr_t vmem_addr_pd(uint64_t cr3, virt_addr_t at);

void vmem_create_pt(uint64_t cr3, virt_addr_t at);
uint8_t vmem_present_pt(uint64_t cr3, virt_addr_t at);
phys_addr_t vmem_addr_pt(uint64_t cr3, virt_addr_t at);

void vmem_create_page(uint64_t cr3, virt_addr_t at, phys_addr_t from);
uint8_t vmem_present_page(uint64_t cr3, virt_addr_t at);
phys_addr_t vmem_addr_page(uint64_t cr3, virt_addr_t at);

void vmem_map(uint64_t cr3, phys_addr_t p_st, phys_addr_t p_end, virt_addr_t v_st);

void vmem_pat_print(void);
void vmem_pat_set(uint8_t idx, uint8_t mem_type);
void vmem_pat_set_range(uint64_t cr3, virt_addr_t st, virt_addr_t end, uint8_t mem_type);

uint16_t vmem_create_pcid(void);