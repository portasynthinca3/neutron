//THIS vvvv IS RETARDED AND WILL BE REMOVED SOON


#ifndef PCI_H
#define PCI_H

#include "../stdlib.h"

uint16_t pci_read_config_16(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offs);
uint16_t pci_read_config_32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offs);

void pci_enumerate(void);

#endif