//Neutron project
//ATA driver

#include "./ata.h"
#include "../stdlib.h"

/*
 * Get the I/O base for an ATA bus
 */
uint16_t ata_iobas(uint8_t bus){
    return bus ? ATA_SECO_IOBAS : ATA_PRIM_IOBAS;
}

/*
 * Get the Control base for an ATA bus
 */
uint16_t ata_devct(uint8_t bus){
    return bus ? ATA_SECO_DEVCT : ATA_PRIM_DEVCT;
}

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
    outb(ata_devct(bus), 1 << 2);
    //Wait 100us
    ata_wait_100us(1);
    //Reset SRST bit in Device Control Register
    outb(ata_devct(bus), 0);
}

/*
 * Read ATA device type
 */
uint8_t ata_get_type(uint8_t bus, uint8_t device){
    //Reset the bus
    ata_soft_reset(bus);
    //Select the device
    outb(ata_iobas(bus) + ATA_REG_DEVSEL, 0xA0 | (device << 4));
    //Wait 400us
    ata_wait_100us(4);
    //Read the signature bytes
    uint8_t ch = inb(ata_iobas(bus) + ATA_REG_CYL_HI);
    uint8_t cl = inb(ata_iobas(bus) + ATA_REG_CYL_LO);
    //Recognize the device type
    if (cl == 0x14 && ch == 0xEB) return ATA_DEV_PATAPI;
	if (cl == 0x69 && ch == 0x96) return ATA_DEV_SATAPI;
	if (cl == 0x00 && ch == 0x00) return ATA_DEV_PATA;
	if (cl == 0x3c && ch == 0xc3) return ATA_DEV_SATA;
	else return ATA_DEV_UNKNOWN;
}

/*
 * Read ATA device sectors
 */
void ata_read_sect(uint8_t bus, uint8_t device, uint32_t lba, uint8_t count, uint8_t* buffer){
    //Reset the bus
    ata_soft_reset(bus);
    //Select the device
    outb(ata_iobas(bus) + ATA_REG_DEVSEL, 0xA0 | (device << 4));
    //Wait 400us
    ata_wait_100us(4);
    //Output: sector count
    outb(ata_iobas(bus) + ATA_REG_SECTCOUNT, count);
    //Output: LBA[7:0]
    outb(ata_iobas(bus) + ATA_REG_LBA_LO, lba & 0xFF);
    //Output: LBA[15:8]
    outb(ata_iobas(bus) + ATA_REG_LBA_MID, (lba >> 8) & 0xFF);
    //Output: LBA[23:16]
    outb(ata_iobas(bus) + ATA_REG_LBA_MID, (lba >> 16) & 0xFF);
    //Output: LBA mode flag, LBA[27:24], drive number
    outb(ata_iobas(bus) + ATA_REG_DEVSEL, 0b11100000 | ((lba & 0x0F) >> 24) | ((device & 1) << 4));
    //Output: command: Read With Retry
    outb(ata_iobas(bus) + ATA_REG_COMMAND, 0x20);
    //Wait for the data to be ready
    while(!(inb(ata_iobas(bus) + ATA_REG_STATUS) & (1 << 3)));
    //Read the data
    rep_insw(0x1F0, (uint32_t)count * 256, (uint16_t*)buffer);
}