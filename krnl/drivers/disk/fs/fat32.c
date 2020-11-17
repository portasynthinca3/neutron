//Neutron Project
//FAT32 filesystem driver

#include "./fat32.h"
#include "../diskio.h"
#include "../part.h"
#include "../../../krnl.h"

/*
 * Seeks a partition file so that it points to the cluster number
 */
void _fat32_seek(fat32_handle_t* handle, uint64_t clust){
    uint64_t lba = handle->rsvd_sect_cnt + (handle->fat_cnt * handle->sect_per_fat) +
                   ((clust - 2) * handle->sect_per_clust);
    diskio_seek(handle->part, lba * 512);
}

/*
 * Reads a cluster into the buffer
 */
void _fat32_read(fat32_handle_t* handle, uint64_t clust, void* buf){
    _fat32_seek(handle, clust);
    diskio_read(handle->part, buf, handle->sect_per_clust * 512);
}

/*
 * Initializes the FAT32 filesystem on a partition
 */
void fat32_init(uint32_t no){
    krnl_write_msg(__FILE__, __LINE__, "loading a FAT32 filesystem");
    //Create the FS partition file handles
    part_get(no)->fs_handle = malloc(sizeof(fat32_handle_t));
    fat32_handle_t* handle = (fat32_handle_t*)part_get(no)->fs_handle;
    file_handle_t*  part   =                  part_get(no)->fs_file;
    handle->part = part;

    //Read and parse the BIOS Parameter Block
    uint8_t bpb[512];
    diskio_seek(part, 0);
    diskio_read(part, bpb, 512);
    if(bpb[66] != 0x28 && bpb[66] != 0x29){
        krnl_write_msgf(__FILE__, __LINE__, "invalid EBPB signature (expected 0x28 or 0x29, got 0x%x)", bpb[38]);
        return;
    }
    handle->sect_per_clust =              bpb[13];
    handle->rsvd_sect_cnt  = *(uint16_t*)&bpb[14];
    handle->fat_cnt        =              bpb[16];
    handle->dir_entry_cnt  = *(uint16_t*)&bpb[17];
    handle->sect_per_fat   = *(uint32_t*)&bpb[36];
    handle->root_dir_clust = *(uint32_t*)&bpb[44];
    handle->fsinfo_sect    = *(uint16_t*)&bpb[48];

    //Read and parse the FSInfo structure
    uint8_t fsinfo[512];
    diskio_seek(part, handle->fsinfo_sect * 512);
    diskio_read(part, fsinfo, 512);
    uint32_t sign1 = *(uint32_t*)&fsinfo[0];
    if(sign1 != 0x41615252){
        krnl_write_msgf(__FILE__, __LINE__, "invalid FSInfo signature 1 (expected 0x41615252, got 0x%x)", sign1);
        return;
    }
    uint32_t sign2 = *(uint32_t*)&fsinfo[484];
    if(sign2 != 0x61417272){
        krnl_write_msgf(__FILE__, __LINE__, "invalid FSInfo signature 2 (expected 0x61417272, got 0x%x)", sign2);
        return;
    }
    handle->free_cluster_cnt   = *(uint32_t*)&fsinfo[488];
    handle->first_free_cluster = *(uint32_t*)&fsinfo[492];
    krnl_write_msgf(__FILE__, __LINE__, "sectors per cluster: %i (cluster size %i)", handle->sect_per_clust, handle->sect_per_clust * 512);
    krnl_write_msgf(__FILE__, __LINE__, "free cluster count: 0x%x", handle->free_cluster_cnt);
    krnl_write_msgf(__FILE__, __LINE__, "first free cluster: 0x%x", handle->first_free_cluster);
}

/*
 * Gets the directory in a FAT32 filesystem
 */
void fat32_get_dir(char* path, dir_handle_t* handle){
    //<ake a local copy of the path string, we're going to modify it
    char path2[DISKIO_MAX_PATH_LEN];
    strcpy(path2, path);
    path = path2;
    //Remove the trailing slash
    if(path[0] == '/')
        path++;
    //
}