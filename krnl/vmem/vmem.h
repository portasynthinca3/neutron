#include "../stdlib.h"

//Page Attribute Table MSR
#define MSR_IA32_PAT                0x277

typedef void* virt_addr_t;
typedef void* phys_addr_t;

void vmem_enable_physwin(void);
uint64_t vmem_physwin_set(phys_addr_t addr);
void vmem_physwin_write32(phys_addr_t addr, uint32_t val);
uint32_t vmem_physwin_read32(phys_addr_t addr);
void vmem_physwin_write64(phys_addr_t addr, uint64_t val);
uint64_t vmem_physwin_read64(phys_addr_t addr);

uint64_t vmem_get_cr3(void);
void vmem_set_cr3(uint64_t cr3);
uint8_t vmem_pcid_supported(void);

void vmem_init(void);
void vmem_enable_trans(void);

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
void vmem_create_page_user(uint64_t cr3, virt_addr_t at, phys_addr_t from);
uint8_t vmem_present_page(uint64_t cr3, virt_addr_t at);
phys_addr_t vmem_virt_to_phys(uint64_t cr3, virt_addr_t at);

void vmem_map(uint64_t cr3, phys_addr_t p_st, phys_addr_t p_end, virt_addr_t v_st);
void vmem_map_user(uint64_t cr3, phys_addr_t p_st, phys_addr_t p_end, virt_addr_t v_st);
void vmem_unmap(uint64_t cr3, virt_addr_t v_st, virt_addr_t v_end);
void vmem_map_defaults(uint64_t cr3);

void vmem_invlpg(phys_addr_t addr);
void vmem_pat_print(void);
void vmem_pat_set(uint8_t idx, uint8_t mem_type);
void vmem_pat_set_range(uint64_t cr3, virt_addr_t st, virt_addr_t end, uint8_t mem_type);

uint16_t vmem_create_pcid(void);