//Neutron Project
//PCI driver

#include "./pci.h"
#include "./usb.h"
#include "../stdlib.h"
#include "./gfx.h"

/*
 * Read config word from a PCI device
 */
uint16_t pci_read_config_16(unsigned char bus, unsigned char slot, unsigned char func, unsigned char offs){
    //Construct the total address
    uint32_t addr = (uint32_t)((uint32_t)0x80000000 | (offs & 0xFC) | ((uint32_t)func << 8)
        | ((uint32_t)slot << 11) | ((uint32_t)bus << 16));
    //Write address to I/O port
    outl(0xCF8, addr);
    //Read the response
    uint32_t data = inl(0xCFC);
    //Format it
    uint16_t val = (short)((data >> ((offs & 2) * 8)) & 0xffff);
    return val;
}

/*
 * Read config doubleword from a PCI device
 */
uint16_t pci_read_config_32(unsigned char bus, unsigned char slot, unsigned char func, unsigned char offs){
    //Construct the total address
    uint32_t addr = (uint32_t)((uint32_t)0x80000000 | (offs & 0xFC) | ((uint32_t)func << 8)
        | ((uint32_t)slot << 11) | ((uint32_t)bus << 16));
    //Write address to I/O port
    outl(0xCF8, addr);
    //Read the response
    return inl(0xCFC);
}

/*
 * Enumerates and initializes all known PCI devices
 */
void pci_enumerate(void){
    gfx_verbose_println("Enumerating PCI devices");
    for(int b = 0; b < 256; b++){
        for(int d = 0; d < 256; d++){
            //Read device VID
            short vendor = pci_read_config_16(b, d, 0, 0);
            if(vendor != 0xFFFFFFFF){ //VID=FFFF means there is no device
                uint16_t product = pci_read_config_16(b, d, 0, 2); //Read PID
                uint16_t class_sub = pci_read_config_16(b, d, 0, 10); //Read class and subclass
                uint16_t if_rev = pci_read_config_16(b, d, 0, 8); //Read interface and revision
                char temp[100] = "Found PCI device VID=";
                char temp2[15];
                strcat(temp, sprintu(temp2, vendor, 1));
                strcat(temp, " PID=");
                strcat(temp, sprintu(temp2, product, 1));
                gfx_verbose_println(temp);
                //Try to detect a known device
                //Firstly, USB controllers (C=0C, S=03)
                if(class_sub == 0x00000C03){
                    //EHCI, IF=0x20
                    if((if_rev & 0xFF00) == 0x2000){
                        gfx_verbose_println("  (EHCI controller)");
                        ehci_add_cont(b, d);
                    }
                }
            }
        }
    }
}