#ifndef ATA_H
#define ATA_H

#include "../h/stdlib.h"

//ATA I/O ports

#define ATA_PRIM_IOBAS                      0x1F0
#define ATA_PRIM_DEVCT                      0x3F6
#define ATA_SECO_IOBAS                      0x170
#define ATA_SECO_DEVCT                      0x376

#define ATA_REG_CYL_LO                      4
#define ATA_REG_CYL_HI                      5
#define ATA_REG_DEVSEL                      6

//ATA device types

#define ATA_DEV_UNKNOWN                     0
#define ATA_DEV_PATAPI                      1
#define ATA_DEV_SATAPI                      2
#define ATA_DEV_PATA                        3
#define ATA_DEV_SATA                        4

//ATA bus
enum ata_bus {ATA_BUS_PRIM = 0, ATA_BUS_SEC = 1};
//ATA bus device
enum ata_dev {ATA_DEV_MASTER = 0, ATA_DEV_SLAVE = 1};

void ata_wait_100us(uint32_t periods);
void ata_soft_reset(uint8_t bus);
uint8_t ata_get_type(uint8_t bus, uint8_t device);

#endif