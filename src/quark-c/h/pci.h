#ifndef PCI_H
#define PCI_H

short pci_read_config_16(unsigned char bus, unsigned char slot, unsigned char func, unsigned char offs);

#endif