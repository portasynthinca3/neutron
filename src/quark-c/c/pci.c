//Neutron Project
//PCI driver

#include "../h/pci.h"

/*
 * Read config word from a PCI device
 */
short pci_read_config_16(unsigned char bus, unsigned char slot, unsigned char func, unsigned char offs){
    //Construct the total address
    unsigned int addr = (unsigned int)(0x80000000 | (offs & 0xFC) | ((unsigned int)func << 8)
        | ((unsigned int)slot << 11) | ((unsigned int)bus << 16));
    //Write address to I/O port
    __asm__ volatile("outl %%eax, %%dx;" : : "a" (addr), "d" (0xCF8));
    //Read the response
    unsigned int data;
    __asm__ volatile("inl %%dx, %%eax;" : "=a" (data) : "d" (0xCFC));
    //Format it (BLACK MAGIC)
    short val = (short)((data >> ((offs & 2) * 8)) & 0xffff);
    return val;
}