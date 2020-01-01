#ifndef DISKIO_H
#define DISKIO_H

#include "./ata.h"

//Size of the buffer dedicated to disk I/O
#define DISK_IO_BUFFER_SIZE                 4096

//The device type
typedef enum {DISK_FLOPPY,
              DISK_PATA, DISK_PATAPI, DISK_SATA, DISK_SATAPI,
              DISK_UHCI, DISK_OHCI, DISK_EHCI, DISK_XHCI} device_type_t;

typedef enum {DISKIO_STATUS_OK = 0, DISKIO_STATUS_INVALID_PART = 1, 
              DISKIO_STATUS_UNSUPPORTED_FS = 2, DISKIO_STATUS_FILE_NOT_FOUND = 3, DISKIO_STATUS_OOB = 4} diskio_status_t;

//Structure describing a partition on a drive
typedef struct {
    //Device type the partition is stored on
    device_type_t device_type;
    //Device number (numbering depends on the device's type)
    uint32_t device_no;
    //GPT entry number
    uint8_t mbr_entry_no;
    //LBA of the partition's start
    uint64_t lba_start;
    //Size of the partition in sectors
    uint64_t size;
    //Partition type
    uint8_t type;
    //Flag indicating whether the entry is valid
    uint8_t valid;
} disk_part_t;

void diskio_init(void);

void diskio_read_sect(uint16_t part_no, uint32_t sect, uint8_t count, uint8_t from_part);

uint8_t diskio_fs_read_file(uint8_t part_no, char* name, uint8_t* dest_buffer);

#endif