//Neutron Project
//VMem - Virtual memory manager and paging controller

#include "../krnl.h"
#include <efi.h>
#include <efilib.h>
#include "./vmem.h"
#include "../stdlib.h"
#include "../cpuid.h"
#include "../drivers/gfx.h"

//A flag that indicates whether PCIDs are supported or not
uint8_t pcid_supported = 0;
//The next available PCID
uint16_t pcid_next = 0;

uint8_t vmem_pcid_supported(void){
    return pcid_supported;
}

/*
 * Initializes the virtual memory manager: configures the CPU, etc.
 */
void vmem_init(void){
    //Clear lowest 12 bits of CR3 before enabling PCIDs
    uint64_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r" (cr3));
    cr3 &= ~0xFFFULL; //full pls :>
    __asm__ volatile("mov %0, %%cr3" : : "r" (cr3));

    //Enable Process Context Identifiers (PCIDs) and 4-level paging, disable SMAP and PKE
    uint64_t cr4;
    __asm__ volatile("mov %%cr4, %0" : "=r" (cr4));
    cr4 |= (1 << 5); //4-level paging
    //Detect if PCIDs are supported by the CPU
    uint32_t ecx;
    cpuid_get_feat(NULL, &ecx);
    pcid_supported = (ecx & CPUID_FEAT_ECX_PCID) > 0;
    if(pcid_supported)
        cr4 |= (1 << 17); //Then enable it
    cr4 &= ~((1ULL << 21) | (1ULL << 22)); //Disable SMAP and PKE
    __asm__ volatile("mov %0, %%cr4" : : "r" (cr4));

    //Disable write protection for supervisor access
    uint64_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r" (cr0));
    cr0 &= ~(1 << 16);
    __asm__ volatile("mov %0, %%cr0" : : "r" (cr0));
}

/*
 * Creates a PML4 structure, returns a value that can be entered into CR3
 */
uint64_t vmem_create_pml4(uint16_t pcid){
    uint64_t cr3 = 0;
    phys_addr_t pml4 = vmem_addr_page(cr3, calloc(8192, 1)); //allocate 8 kB even though we only need 4
    pml4 = (uint8_t*)pml4 + 4096 - ((uint64_t)pml4 % 4096); //align by 4 kB
    //Set the PML4 pointer
    cr3 = (uint64_t)pml4;
    //Set the PCID
    if(pcid_supported)
        cr3 |= pcid & 0x0FFF; //only let the lower 12 bits through
    return cr3;
}




/*
 * Creates a PDPT (page directory pointer table) structure that
 *   can be accessed using the specific "at" mask and CR3 value
 */
void vmem_create_pdpt(uint64_t cr3, virt_addr_t at){
    //Extract PML4 address from CR3
    phys_addr_t pml4_addr = (phys_addr_t)(cr3 & 0xFFFFFFFFFFFFF000);
    //Extract entry index from "at"
    uint64_t pml4e_idx = ((uint64_t)at >> 39) & 0x1FF;
    //Calculate the address of the entry
    phys_addr_t pml4e_addr = (uint8_t*)pml4_addr + (pml4e_idx * 8);
    //Allocate space for the PDPT
    phys_addr_t pdpt = vmem_addr_page(cr3, calloc(8192, 1)); //allocate 8 kB even though we only need 4
    pdpt = (uint8_t*)pdpt + 4096 - ((uint64_t)pdpt % 4096); //align by 4 kB
    //Generate the entry
    uint64_t pml4e = 0;
    pml4e |= (1 << 0); //it's present
    pml4e |= (1 << 1); //writes are allowed
    //pml4e |= (1 << 2); //user access is allowed
    pml4e &= ~((1 << 3) | (1 << 4)); //enable caching on access to this PDPT
    pml4e &= ~(1 << 5); //clear the "accessed" bit
    pml4e |= (uint64_t)pdpt & 0xFFFFFFFFFFFFF000; //set the address
    pml4e &= ~(1ULL << 63); //allow instruction fetching
    //Set the entry
    *(uint64_t*)pml4e_addr = pml4e;
}

/*
 * Checks if PDPT is present
 */
uint8_t vmem_present_pdpt(uint64_t cr3, virt_addr_t at){
    //Extract PML4 address from CR3
    phys_addr_t pml4_addr = (phys_addr_t)(cr3 & 0xFFFFFFFFFFFFF000);
    //Extract PML4 entry index from "at"
    uint64_t pml4e_idx = ((uint64_t)at >> 39) & 0x1FF;
    //Check its "present" bit
    return *(uint64_t*)((uint8_t*)pml4_addr + (pml4e_idx * 8)) & 1;
}

/*
 * Returns the physical address of the PDPT that maps a specific address
 */
phys_addr_t vmem_addr_pdpt(uint64_t cr3, virt_addr_t at){
    //Extract PML4 address from CR3
    phys_addr_t pml4_addr = (phys_addr_t)(cr3 & 0xFFFFFFFFFFFFF000);
    //Extract PML4 entry index from "at"
    uint64_t pml4e_idx = ((uint64_t)at >> 39) & 0x1FF;
    //Extract PDPT entry address
    return (phys_addr_t)(*(uint64_t*)((uint8_t*)pml4_addr + (pml4e_idx * 8)) & 0x7FFFFFFFFFFFF000);
}




/*
 * Creates a PD (page directory) structure that
 *   can be accessed using the specific "at" mask and CR3 value
 */
void vmem_create_pd(uint64_t cr3, virt_addr_t at){
    //Check if that entry is present
    if(!vmem_present_pdpt(cr3, at))
        vmem_create_pdpt(cr3, at); //Create it if not
    
    //Allocate space for the PD
    phys_addr_t pd = vmem_addr_page(cr3, calloc(8192, 1)); //allocate 8 kB even though we only need 4
    pd = (uint8_t*)pd + 4096 - ((uint64_t)pd % 4096); //align by 4 kB
    //Extract entry index from "at"
    uint64_t pdpte_idx = ((uint64_t)at >> 30) & 0x1FF;
    //Calculate the address of the entry
    phys_addr_t pdpte_addr = (uint8_t*)vmem_addr_pdpt(cr3, at) + (pdpte_idx * 8);
    //Generate the entry
    uint64_t pdpte = 0;
    pdpte |= (1 << 0); //it's present
    pdpte |= (1 << 1); //writes are allowed
    //pdpte |= (1 << 2); //user access is allowed
    pdpte &= ~((1 << 3) | (1 << 4)); //enable caching on access to this PDPT
    pdpte &= ~(1 << 5); //clear the "accessed" bit
    pdpte |= (uint64_t)pd & 0xFFFFFFFFFFFFF000; //set the address
    pdpte &= ~(1ULL << 63); //allow instruction fetching
    //Set the entry
    *(uint64_t*)(pdpte_addr) = pdpte;
}

/*
 * Checks if PD is present
 */
uint8_t vmem_present_pd(uint64_t cr3, virt_addr_t at){
    //Extract PDPT address
    phys_addr_t pdpt = vmem_addr_pdpt(cr3, at);
    //Calculate the entry index
    uint64_t pdpte_idx = ((uint64_t)at >> 30) & 0x1FF;
    //Check its "present" bit
    return *(uint64_t*)((uint8_t*)pdpt + (pdpte_idx * 8)) & 1;
}

/*
 * Returns the physical address of the PD that maps a specific address
 */
phys_addr_t vmem_addr_pd(uint64_t cr3, virt_addr_t at){
    //Extract PDPT address
    phys_addr_t pdpt = vmem_addr_pdpt(cr3, at);
    //Calculate the entry index
    uint64_t pdpte_idx = ((uint64_t)at >> 30) & 0x1FF;
    //Extract its address
    return (phys_addr_t)(*(uint64_t*)((uint8_t*)pdpt + (pdpte_idx * 8)) & 0x7FFFFFFFFFFFF000);
}




/*
 * Creates a PT (page table) structure that
 *   can be accessed using the specific "at" mask and CR3 value
 */
void vmem_create_pt(uint64_t cr3, virt_addr_t at){
    //Check if that entry is present
    if(!vmem_present_pd(cr3, at))
        vmem_create_pd(cr3, at); //Create it if not
    
    //Allocate space for the PT
    phys_addr_t pt = vmem_addr_page(cr3, calloc(8192, 1)); //allocate 8 kB even though we only need 4
    pt = (uint8_t*)pt + 4096 - ((uint64_t)pt % 4096); //align by 4 kB
    //Extract entry index from "at"
    uint64_t pde_idx = ((uint64_t)at >> 21) & 0x1FF;
    //Calculate the address of the entry
    phys_addr_t pde_addr = (uint8_t*)vmem_addr_pd(cr3, at) + (pde_idx * 8);
    //Generate the entry
    uint64_t pde = 0;
    pde |= (1 << 0); //it's present
    pde |= (1 << 1); //writes are allowed
    //pde |= (1 << 2); //user access is allowed
    pde &= ~((1 << 3) | (1 << 4)); //enable caching on access to this PDPT
    pde &= ~(1 << 5); //clear the "accessed" bit
    pde |= (uint64_t)pt & 0xFFFFFFFFFFFFF000; //set the address
    pde &= ~(1ULL << 63); //allow instruction fetching
    //Set the entry
    *(uint64_t*)(pde_addr) = pde;
}

/*
 * Checks if PT is present
 */
uint8_t vmem_present_pt(uint64_t cr3, virt_addr_t at){
    //Extract PD address
    phys_addr_t pd = vmem_addr_pd(cr3, at);
    //Calculate the entry index
    uint64_t pde_idx = ((uint64_t)at >> 21) & 0x1FF;
    //Check its "present" bit
    return *(uint64_t*)((uint8_t*)pd + (pde_idx * 8)) & 1;
}

/*
 * Returns the physical address of the PT that maps a specific address
 */
phys_addr_t vmem_addr_pt(uint64_t cr3, virt_addr_t at){
    //Extract PD address
    phys_addr_t pd = vmem_addr_pd(cr3, at);
    //Calculate the entry index
    uint64_t pde_idx = ((uint64_t)at >> 21) & 0x1FF;
    //Extract its address
    return (phys_addr_t)(*(uint64_t*)((uint8_t*)pd + (pde_idx * 8)) & 0x7FFFFFFFFFFFF000);
}




/*
 * Creates a page mapped to a specific physical address that
 *   can be accessed using the specific "at" mask and CR3 value
 */
void vmem_create_page(uint64_t cr3, virt_addr_t at, phys_addr_t from){
    //Check if that entry is present
    if(!vmem_present_pt(cr3, at))
        vmem_create_pt(cr3, at); //Create it if not
    
    //Extract entry index from "at"
    uint64_t pte_idx = ((uint64_t)at >> 12) & 0x1FF;
    //Calculate the address of the entry
    phys_addr_t pte_addr = (uint8_t*)vmem_addr_pt(cr3, at) + (pte_idx * 8);
    //Generate the entry
    uint64_t pte = 0;
    pte |= (1 << 0); //it's present
    pte |= (1 << 1); //writes are allowed
    //pte |= (1 << 2); //user access is allowed
    pte &= ~((1 << 3) | (1 << 4)); //enable caching on access to this PDPT
    pte &= ~(1 << 5); //clear the "accessed" bit
    pte |= (uint64_t)from & 0xFFFFFFFFFFFFF000; //set the address
    pte &= ~(1ULL << 63); //allow instruction fetching
    //Set the entry
    *(uint64_t*)(pte_addr) = pte;
}

/*
 * Checks if page is present
 */
uint8_t vmem_present_page(uint64_t cr3, virt_addr_t at){
    //Extract PT address
    phys_addr_t pt = vmem_addr_pt(cr3, at);
    //Calculate the entry index
    uint64_t pte_idx = ((uint64_t)at >> 12) & 0x1FF;
    //Check its "present" bit
    return *(uint64_t*)((uint8_t*)pt + (pte_idx * 8)) & 1;
}

/*
 * Returns the physical address of the page that maps a specific address
 */
phys_addr_t vmem_addr_page(uint64_t cr3, virt_addr_t at){
    //Extract PT address
    phys_addr_t pt = vmem_addr_pt(cr3, at);
    //Calculate the entry index
    uint64_t pte_idx = ((uint64_t)at >> 12) & 0x1FF;
    //Extract its base address and add the offset
    return (phys_addr_t*)((uint64_t)(*(uint64_t*)((uint8_t*)pt + (pte_idx * 8)) & 0x7FFFFFFFFFFFF000)
                        + (uint64_t)((uint64_t)at & 0xFFF));
}




/*
 * Maps a virtual address range to a physical address range
 */
void vmem_map(uint64_t cr3, phys_addr_t p_st, phys_addr_t p_end, virt_addr_t v_st){
    //Loop through the range
    for(uint64_t offs = 0; offs < p_end - p_st; offs += 4096){
        //Map one page
        vmem_create_page(cr3, (uint8_t*)v_st + offs, (uint8_t*)p_st + offs);
    }
}




/*
 * Prints Page Attribute Table for debugging
 */
void vmem_pat_print(void){
    gfx_verbose_println("PAT debug");
    //Read PAT MSR
    uint64_t pat = rdmsr(MSR_IA32_PAT);
    //For each entry
    for(uint8_t i = 0; i < 8; i++){
        //Fetch an entry
        uint8_t entry = *(((uint8_t*)&pat) + i);
        //Construct the string
        char temp[50];
        temp[0] = 0;
        char temp2[5];
        strcat(temp, "  entry ");
        strcat(temp, sprintu(temp2, i, 1));
        strcat(temp, ": ");
        switch(entry){
            case 0:
                strcat(temp, "UC");
                break;
            case 1:
                strcat(temp, "WC");
                break;
            case 2:
                strcat(temp, "RSVD (2)");
                break;
            case 3:
                strcat(temp, "RSVD (3)");
                break;
            case 4:
                strcat(temp, "WT");
                break;
            case 5:
                strcat(temp, "WP");
                break;
            case 6:
                strcat(temp, "WB");
                break;
            case 7:
                strcat(temp, "UC-");
                break;
            default:
                strcat(temp, "RSVD (8-255)");
                break;
        }
        //Print it
        gfx_verbose_println(temp);
    }
}

/*
 * Sets an entry in Page Attribute Table
 */
void vmem_pat_set(uint8_t idx, uint8_t mem_type){
    //Fetch PAT
    uint64_t pat = rdmsr(MSR_IA32_PAT);
    //Clear the entry
    pat &= ~(0xFFULL << (idx * 8));
    //Set the entry
    pat |= ((uint64_t)mem_type << (idx * 8));
    //Write PAT
    wrmsr(MSR_IA32_PAT, pat);
}

/*
 * For a specific descriptor (CR3), sets memory type in a virtual memory address range
 */
void vmem_pat_set_range(uint64_t cr3, virt_addr_t st, virt_addr_t end, uint8_t mem_type){
    //Fetch PAT
    uint64_t pat = rdmsr(MSR_IA32_PAT);
    //Go through it to see if the memory type we need exists
    uint8_t pat_idx = 7;
    for(uint8_t idx = 0; idx < 8; idx++)
        if(((pat >> (idx * 8)) & 0xFF) == mem_type)
            pat_idx = idx;
    //pat_idx is now set to the entry number in PAT with
    //  the memory type we need. If no such entries
    //  exist, overwrite the 7th (counting from 0) entry
    if(pat_idx == 7)
        vmem_pat_set(pat_idx, mem_type);
    //Go through addresses
    for(uint64_t offs = 0; offs <= end - st; offs += 4 * 1024){
        //From CR3, get the PT entry describing a page
        uint64_t* pt = vmem_addr_pt(cr3, st);
        //Calculate the entry offset
        uint64_t* pte = pt + (((uint64_t)((uint64_t)st + offs) >> 12) & 0x1FF);
        //Clear PWT, PCD and PAT bits of the entry
        *pte &= ~((1 << 3) | (1 << 4) | (1 << 7));
        //Set bits according to the PAT index
        *pte |= (pat_idx & 1) << 3;
        *pte |= ((pat_idx & 2) >> 1) << 4;
        *pte |= ((pat_idx & 4) >> 2) << 7;
    }
}

/*
 * Reads the current CR3 value
 */
uint64_t vmem_get_cr3(void){
    uint64_t cr3 = 0;
    __asm__ volatile("mov %%cr3, %0" : "=r" (cr3));
    return cr3;
}

/*
 * Creates a new PCID
 */
uint16_t vmem_create_pcid(void){
    return pcid_next++;
}