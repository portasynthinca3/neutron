//Neutron project
//ATA driver

#include "../h/ata.h"
#include "../h/stdlib.h"

/*
 * Wait some time, the unit for time is 100us
 */
void ata_wait_100us(uint32_t periods){
    //Reading the same I/O port several times in a row yields a higher delay of 100us
    //Yes, I know this is not the right way to do it, but that's kind of a replacement
    //  while i didn't implement the APIC timer driver
    while(periods--)
        inb(ATA_PRIM_DEVCT);
}

/*
 * Reset devices on the ATA bus
 */
void ata_soft_reset(uint8_t bus){
    //Set SRST bit in Device Control Register
    outb(bus ? ATA_SECO_DEVCT : ATA_PRIM_DEVCT, 1 << 2);
    //Wait 100us
    ata_wait_100us(1);
    //Reset SRST bit in Device Control Register
    outb(bus ? ATA_SECO_DEVCT : ATA_PRIM_DEVCT, 0);
}

/*
 * Read ATA device type
 */
uint8_t ata_get_type(uint8_t bus, uint8_t device){
    //Reset the bus
    ata_soft_reset(bus);
    //Select the device
    outb((bus ? ATA_SECO_IOBAS: ATA_PRIM_IOBAS) + ATA_REG_DEVSEL, 0xA0 | (device << 4));
    //Wait 400us
    ata_wait_100us(4);
    //Read the signature bytes
    uint8_t ch = inb((bus ? ATA_SECO_IOBAS : ATA_PRIM_IOBAS) + ATA_REG_CYL_HI);
    uint8_t cl = inb((bus ? ATA_SECO_IOBAS : ATA_PRIM_IOBAS) + ATA_REG_CYL_LO);
    //Recognize the device type
    if (cl == 0x14 && ch == 0xEB) return ATA_DEV_PATAPI;
	if (cl == 0x69 && ch == 0x96) return ATA_DEV_SATAPI;
	if (cl == 0x00 && ch == 0x00) return ATA_DEV_PATA;
	if (cl == 0x3c && ch == 0xc3) return ATA_DEV_SATA;
	else return ATA_DEV_UNKNOWN;
}