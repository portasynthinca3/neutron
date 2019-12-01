//Neutron Project
//PCI driver

#include "../h/pci.h"
#include "../h/usb.h"
#include "../h/stdlib.h"
#include "../h/gfx.h"

/*
 * Read config word from a PCI device
 */
short pci_read_config_16(unsigned char bus, unsigned char slot, unsigned char func, unsigned char offs){
    //Construct the total address
    unsigned int addr = (unsigned int)((unsigned int)0x80000000 | (offs & 0xFC) | ((unsigned int)func << 8)
        | ((unsigned int)slot << 11) | ((unsigned int)bus << 16));
    //Write address to I/O port
    outl(0xCF8, addr);
    //Read the response
    unsigned int data = inl(0xCFC);
    //Format it
    short val = (short)((data >> ((offs & 2) * 8)) & 0xffff);
    return val;
}

/*
 * Read config doubleword from a PCI device
 */
short pci_read_config_32(unsigned char bus, unsigned char slot, unsigned char func, unsigned char offs){
    //Construct the total address
    unsigned int addr = (unsigned int)((unsigned int)0x80000000 | (offs & 0xFC) | ((unsigned int)func << 8)
        | ((unsigned int)slot << 11) | ((unsigned int)bus << 16));
    //Write address to I/O port
    outl(0xCF8, addr);
    //Read the response
    return inl(0xCFC);
}

/*
 * Enumerates and initializes all known PCI devices
 */
void pci_enumerate(void){
    for(int b = 0; b < 256; b++){
        for(int d = 0; d < 256; d++){
            //Read device VID
            short vendor = pci_read_config_16(b, d, 0, 0);
            if(vendor != 0xFFFFFFFF){ //VID=FFFF means there is no device
                short product = pci_read_config_16(b, d, 0, 2); //Read PID
                short class_sub = pci_read_config_16(b, d, 0, 10); //Read class and subclass
                short if_rev = pci_read_config_16(b, d, 0, 8); //Read interface and revision
                //Try to detect a known device
                //Firstly, USB controllers (C=0C, S=03)
                if(class_sub == 0x00000C03){
                    //UHCI, IF=0x00
                    if((if_rev & 0xFF00) == 0x0000)
                        gfx_vterm_println("UHCI USB controller found [unsupported]", 0x28);
                    //OHCI, IF=0x10
                    if((if_rev & 0xFF00) == 0x1000)
                        gfx_vterm_println("OHCI USB controller found [unsupported]", 0x28);
                    //EHCI, IF=0x20
                    if((if_rev & 0xFF00) == 0x2000){
                        gfx_vterm_println("EHCI USB controller found", 0x0F);
                        ehci_add_cont(b, d);
                    }
                    //xHCI, IF=0x30
                    if((if_rev & 0xFF00) == 0x3000)
                        gfx_vterm_println("xHCI USB controller found [unsupported]", 0x28);
                }
            }
        }
    }
}