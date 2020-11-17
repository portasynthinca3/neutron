#ifndef FAT32_H
#define FAT32_H

#include "../../../stdlib.h"
#include "../diskio.h"

//Structure definitions

typedef struct {
    //partition file
    file_handle_t* part;
    //contained in the BPB
    uint16_t rsvd_sect_cnt;
    uint8_t  sect_per_clust;
    uint8_t  fat_cnt;
    uint32_t sect_per_fat;
    uint16_t dir_entry_cnt;
    uint32_t root_dir_clust;
    uint16_t fsinfo_sect;
    //contained in the FSInfo structure
    uint32_t free_cluster_cnt;
    uint32_t first_free_cluster;
} fat32_handle_t;

//Function prototypes

void _fat32_seek (fat32_handle_t* handle, uint64_t clust);
void _fat32_read (fat32_handle_t* handle, uint64_t clust, void* buf);

void fat32_init (uint32_t no);

void fat32_get_dir (char* path, dir_handle_t* handle);

#endif