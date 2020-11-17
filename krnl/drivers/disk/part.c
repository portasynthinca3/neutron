//Neutron Project
//Partition driver

#include "./part.h"
#include "../../stdlib.h"
#include "../../krnl.h"
#include "./diskio.h"

#include "./fs/fat32.h"

part_t parts[MAX_PARTS];
uint32_t part_cnt;

/*
 * Load recognizable partitions from the disk virtual file
 */
void parts_load(char* path){
    krnl_write_msgf(__FILE__, __LINE__, "loading partitions on %s", path);
    //Open the disk file
    file_handle_t* disk = (file_handle_t*)malloc(sizeof(file_handle_t));
    diskio_open(path, disk, DISKIO_FILE_ACCESS_READ);
    //Read the boot sector
    uint8_t bootsect[512];
    diskio_seek(disk, 0);
    diskio_read(disk, bootsect, 512);
    //Check the signature
    if(bootsect[510] != 0x55 || bootsect[511] != 0xAA){
        krnl_write_msgf(__FILE__, __LINE__, "bootsect signature is invalid (0x%x, 0x%x)", bootsect[510], bootsect[511]);
        return;
    }
    //Go through the partition descriptors
    for(int p = 0; p < 4; p++){
        uint16_t base  = 0x1BE + (p * 16);
        uint32_t start = *(uint32_t*)&bootsect[base +  8];
        uint32_t size  = *(uint32_t*)&bootsect[base + 12];
        uint8_t  type  =              bootsect[base +  4];
        if(type == 0 || size == 0)
            continue; //invalid or free entry

        //Parse GPT if it's a EFI partition
        if(type == 0xEE || type == 0xEF){
            parts_load_gpt(disk);
            continue;
        }
        //Else, it's a typical MBR partition
        krnl_write_msgf(__FILE__, __LINE__, "found MBR partition type 0x%x at 0x%x of size 0x%x", type, start, size);
        if(type == 0x0C){
            parts[part_cnt++] = (part_t){.is_gpt = 0, .lba_start = start, .lba_end = start + size,
                                         .type = PART_TYPE_FAT32, .part_no = p, .drive_file = disk};
            part_load(part_cnt - 1);
        } else {
            parts[part_cnt++] = (part_t){.is_gpt = 0, .lba_start = start, .lba_end = start + size,
                                         .type = PART_TYPE_UNKNOWN, .part_no = p, .drive_file = disk};
            part_load(part_cnt - 1);
        }
        strcpy(parts[part_cnt - 1].drive, path);
    }
}

/*
 * Load recognizable partitions from the disk virtual file assuming it is using GPT
 */
void parts_load_gpt(file_handle_t* disk){
    //Read the primary and secondary GPT headers
    uint8_t hdr1[512], hdr2[512];
    uint8_t* hdr;
    diskio_seek(disk, 512);
    diskio_read(disk, hdr1, 512);
    diskio_seek(disk, disk->info.size - 512);
    diskio_read(disk, hdr2, 512);
    //TODO: the correct header should be determined and used
    hdr = hdr1;

    //Check the signature
    if(memcmp(hdr, "EFI PART", 8) != 0){
        char* actual[9];
        memcpy(actual, hdr, 8);
        actual[8] = 0;
        krnl_write_msgf(__FILE__, __LINE__, "GPT signature is invalid (expected \"EFI PART\", got \"%s\")", actual);
        return;
    }
    //Check revision
    uint32_t rev = *(uint32_t*)&hdr[8];
    krnl_write_msgf(__FILE__, __LINE__, "GPT revision %i %s", rev, (rev == 65536) ? "" : "(unsupported)");
    if(rev != 65536)
        return;
    //Get some parameters
    uint64_t pe_st  = *(uint64_t*)&hdr[72];
    uint32_t pe_cnt = *(uint32_t*)&hdr[80];
    uint32_t pe_sz  = *(uint32_t*)&hdr[84];
    //Go through partition table sectors
    for(int s = 0; s < (pe_cnt * pe_sz) / 512; s++){
        uint8_t sect[512];
        diskio_seek(disk, (pe_st + s) * 512);
        diskio_read(disk, sect, 512);

        //Go through partition descriptors
        for(int p = 0; p < 512 / pe_sz; p++){
            uint8_t* desc      = &sect[p * pe_sz];
            guid_t*  type_guid = (guid_t*)&desc[0];
            uint64_t lba_start = *(uint64_t*)&desc[32];
            uint64_t lba_end   = *(uint64_t*)&desc[40];
            //uint64_t attr      = *(uint64_t*)&desc[48];

            //Determine the partition type
            if(memcmp(type_guid, PART_GUID_NONE, sizeof(guid_t)) == 0)
                continue;
            if(memcmp(type_guid, PART_GUID_MBDP, sizeof(guid_t)) == 0){
                krnl_write_msgf(__FILE__, __LINE__, "found Microsoft Basic Data partition between 0x%x and %x", lba_start, lba_end);
                parts[part_cnt++] = (part_t){.is_gpt = 1, .lba_start = lba_start, .lba_end = lba_end,
                                             .type = PART_TYPE_FAT32, .part_no = p + (s * (512 / pe_sz)), .drive_file = disk};
                part_load(part_cnt - 1);
            } else {
                krnl_write_msgf(__FILE__, __LINE__, "partition of unknown type between 0x%x and 0x%x", lba_start, lba_end);
                parts[part_cnt++] = (part_t){.is_gpt = 1, .lba_start = lba_start, .lba_end = lba_end,
                                             .type = PART_TYPE_UNKNOWN, .part_no = p + (s * (512 / pe_sz)), .drive_file = disk};
                part_load(part_cnt - 1);
            }
            strcpy(parts[part_cnt - 1].drive, disk->info.name);
        }
    }
}

/*
 * Returns the partition by its number
 */
part_t* part_get(uint32_t num){
    return &parts[num];
}

/*
 * Parses the filesystem on a partition
 */
void part_load(uint32_t no){
    //Open the partition file
    free(parts[no].fs_file);
    parts[no].fs_file = (file_handle_t*)malloc(sizeof(file_handle_t));
    char part_path[32];
    sprintf(part_path, "/part/%i", no);
    diskio_open(part_path, parts[no].fs_file, DISKIO_FILE_ACCESS_READ_WRITE);
    //Call the specific FS initializer
    switch(parts[no].type){
        case PART_TYPE_FAT32:
            fat32_init(no);
            break;
        case PART_TYPE_UNKNOWN:
            return;
    }
}