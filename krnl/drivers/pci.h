#ifndef PCI_H
#define PCI_H

#include "../stdlib.h"
#include "./acpi.h"

//Structure definitions

typedef struct {
    uint64_t base;
    uint16_t segment;
    uint8_t  start_bus;
    uint8_t  end_bus;
    uint32_t rsvd;
} __attribute__((packed)) pci_cfg_desc_t;

typedef struct {
    acpi_sdt_hdr_t sdt_hdr;

    uint64_t rsvd;
    pci_cfg_desc_t descs[1];
} __attribute__((packed)) acpi_mcfg_t;

//Function prototypes

void      pci_init      (void);
void      pci_enumerate (void);
uint32_t* pci_cfg_space (uint16_t bus, uint16_t dev, uint16_t func);

#endif