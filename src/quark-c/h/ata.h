#ifndef ATA_H
#define ATA_H

#include "../h/stdlib.h"

//ATA I/O ports

#define ATA_PRIM_IOBAS                      0x1F0
#define ATA_PRIM_DEVCT                      0x3F6
#define ATA_SECO_IOBAS                      0x170
#define ATA_SECO_DEVCT                      0x376

<<<<<<< HEAD
#define ATA_REG_CYL_LO                      4
#define ATA_REG_CYL_HI                      5
#define ATA_REG_DEVSEL                      6
=======
#define ATA_REG_DATA                        0
#define ATA_REG_ERROR                       1
#define ATA_REG_FEATURES                    1
#define ATA_REG_SECTCOUNT                   2
#define ATA_REG_SECTNUM                     3
#define ATA_REG_LBA_LO ATA_REG_SECTNUM
#define ATA_REG_CYL_LO                      4
#define ATA_REG_LBA_MID ATA_REG_CYL_LO
#define ATA_REG_CYL_HI                      5
#define ATA_REG_LBA_HI ATA_REG_CYL_HI
#define ATA_REG_DEVSEL                      6
#define ATA_REG_STATUS                      7
#define ATA_REG_COMMAND                     7
>>>>>>> ATA reading done

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

<<<<<<< HEAD
=======
uint16_t ata_iobas(uint8_t bus);
uint16_t ata_devct(uint8_t bus);

>>>>>>> ATA reading done
void ata_wait_100us(uint32_t periods);
void ata_soft_reset(uint8_t bus);
uint8_t ata_get_type(uint8_t bus, uint8_t device);

<<<<<<< HEAD
=======
void ata_read_sect(uint8_t bus, uint8_t device, uint32_t lba, uint8_t count, uint8_t* buffer);

>>>>>>> ATA reading done
#endif