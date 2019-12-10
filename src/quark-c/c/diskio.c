//Neutron Project
//General disk I/O

#include "../h/diskio.h"
#include "../h/stdlib.h"
#include "../h/ata.h"

disk_part_t* partitions;
uint8_t* buffer;

/*
 * Initialize the disk I/O subsystem by initializing all known storage devices and loading all the partitions from them
 */
void diskio_init(void){
    //Allocate a chunk of memory for the list
    partitions = (disk_part_t*)malloc(sizeof(disk_part_t) * 64);
    uint8_t cur_part = 0;
    //Allocal a chunk of memory as a read/write buffer
    buffer = (uint8_t*)malloc(sizeof(uint8_t) * DISK_IO_BUFFER_SIZE);
    //Firstly, go through the ATA devices
    for(uint8_t i = 0; i < 4; i++){
        //Get the device type
        uint8_t ata_type = ata_get_type(i >> 1, i & 1);
        //We only support PATA for now
        if(ata_type == ATA_DEV_PATA){
            //Read the device's first sector that contains MBR
            ata_read_sect(i >> 1, i & 1, 0, 1, buffer);
            //Go through all the partition entries
            for(uint8_t p = 0; p < 4; p++){
                uint32_t p_base = (uint32_t)buffer + 0x1BE + (p * 16);
                //Write the partition disk type and number
                partitions[cur_part].device_type = DISK_PATA;
                partitions[cur_part].device_no = i;
                //Fetch the partition type
                partitions[cur_part].type = *(uint8_t*)(p_base + 4);
                //Fetch the partition start sector
                partitions[cur_part].lba_start = *(uint32_t*)(p_base + 8);
                //Fetch the partition length
                partitions[cur_part].size = *(uint32_t*)(p_base + 0xC);
                //Fetch the partition status
                partitions[cur_part].valid = ((*(uint8_t*)p_base) == 0x80);
                //Increment the partition pointer
                cur_part++;
            }
        }
    }
}

/*
 * Read the byte describing floppy drives from CMOS
 */
unsigned char diskio_get_floppy_drives(void){
    unsigned char val;
    //BLACK MAGIC
    __asm__ volatile("movb $0x10, %%al; outb %%al, $0x70; inb $0x71, %%al" : "=a" (val));
    return val;
}

/*
 * Read sectors to the buffer from a partition
 */
void diskio_read_sect(uint8_t part_no, uint32_t sect, uint8_t count, uint32_t from_part){
    disk_part_t* part = &partitions[part_no];
    //Immediately return if the partition is invalid
    if(!part->valid)
        return;
    //Depending on the disk type, read sectors
    switch(part->device_type){
        case DISK_PATA:
            //from_part=1 means sector number is with respect to partition start
            ata_read_sect(part->device_no >> 1, part->device_no & 1,
                          sect + (from_part ? part->lba_start : 0), count, buffer);
            break;
    }
}

/*
 * Read a file from disk
 */
uint8_t diskio_fs_read_file(uint8_t part_no, char* name, uint8_t* dest_buffer, uint32_t sect_start, uint32_t len){
    disk_part_t* part = &partitions[part_no];
    //Immediately return if the partition is invalid
    if(!part->valid)
        return DISKIO_STATUS_INVALID_PART;
    //Immediately return if the length requested won't fit in the buffer
    if(len >= DISK_IO_BUFFER_SIZE)
        return DISKIO_STATUS_OOB;
    //Immediately return if the length requested exceeds the 255 sector limit
    if(len >= 512 * 255)
        return DISKIO_STATUS_OOB;
    //Firstly, everything depends on the filesystem
    //Neutron currently only supports nFS
    if(part->type == 0x58){
        //Read nFS master sector and master filetable
        diskio_read_sect(part_no, 0, 2, 1);
        //Check nFS signature
        if(*(uint32_t*)(buffer) != 0xDEADF500)
            return DISKIO_STATUS_INVALID_PART;
        //Go through all the file entries
        for(uint16_t i = 512; i < 1024; i += 32){
            //Fetch the filename
            char* filename = (char*)(buffer + i);
            //Compare with with the desired name
            if(strcmp(filename, name) == 0){
                //Fetch file's length and starting sector
                uint32_t fsize = *(uint32_t*)(buffer + i + 24);
                uint32_t fslba = *(uint32_t*)(buffer + i + 28) - part->lba_start;
                //If the desired starting LBA or length is out of bounds, return
                if((sect_start * 512) + len >= (fslba * 512) + fsize)
                    return DISKIO_STATUS_OOB;
                //Read the sectors
                diskio_read_sect(part_no, fslba + sect_start, len / 512, 1);
                //Copy them over to the destination buffer
                memcpy(dest_buffer, buffer, len);
                //Return
                return DISKIO_STATUS_OK;
            }
        }
        //If no file with such a name was found, return
        return DISKIO_STATUS_FILE_NOT_FOUND;
    } else {
        return DISKIO_STATUS_UNSUPPORTED_FS;
    }
}