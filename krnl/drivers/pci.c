//Neutron Project
//PCI driver

#include "./pci.h"
#include "../stdlib.h"
#include "../krnl.h"
#include "./acpi.h"

#include "./ahci.h"

//All PCI address allocation descriptors
uint16_t pci_cfg_sect_cnt;
pci_cfg_desc_t* cfg_descs;

/*
 * Initializes and enumerates the PCI bus
 */
void pci_init(void){
    //Find the MCFG table
    acpi_mcfg_t* mcfg = rsdt_find("MCFG");
    if(mcfg == NULL){
        krnl_write_msg(__FILE__, "MCFG not found");
        return;
    }
    krnl_write_msgf(__FILE__, "Found MCFG at 0x%x", mcfg);
    pci_cfg_sect_cnt = (mcfg->sdt_hdr.len - sizeof(acpi_sdt_hdr_t)) / sizeof(pci_cfg_desc_t);
    cfg_descs = mcfg->descs;
    //Enumerate PCI devices
    pci_enumerate();
    while(1);
}

/*
 * Enumerates PCI devices
 */
void pci_enumerate(void){
    for(uint16_t bus = 0; bus < 256; bus++){
        //Check if this bus exists
        if(pci_cfg_space(bus, 0, 0) == NULL)
            continue;
        //Go through devices on this bus
        for(uint16_t dev = 0; dev < 256; dev++){
            //Get the config space pointer
            uint32_t* cfg_space = pci_cfg_space(bus, dev, 0);
            //Get VID and PID
            uint32_t vid_pid = cfg_space[0];
            if(vid_pid == 0xFFFFFFFF)
                continue;
            uint16_t vid   = vid_pid & 0xFFFF;
            uint16_t pid   = vid_pid >> 16;
            uint16_t c_sub = cfg_space[2] >> 16;
            krnl_write_msgf(__FILE__, "found dev VID=0x%x PID=0x%x C_SUB=0x%x", vid, pid, c_sub);

            //Initialize devices based on their type
            switch(c_sub){
                case 0x0106:
                    ahci_init((ahci_hba_mem_t*)(uint64_t)cfg_space[9]);
                    break;
            }
        }
    }
}

/*
 * Returns the pointer to PCI device configration space
 */
uint32_t* pci_cfg_space(uint16_t bus, uint16_t dev, uint16_t func){
    for(int i = 0; i < pci_cfg_sect_cnt; i++){
        pci_cfg_desc_t* cfg = &cfg_descs[i];
        if(bus >= cfg->start_bus && bus < cfg->end_bus)
            return (uint32_t*)(cfg->base + ((bus - cfg->start_bus) << 20 | dev << 15 | func << 12));
    }
    return NULL;
}