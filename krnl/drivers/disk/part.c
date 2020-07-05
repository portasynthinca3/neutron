//Neutron Project
//Partition driver

#include "./part.h"
#include "../../stdlib.h"
#include "./diskio.h"

/*
 * Load, register and mount recognizable partitions from the disk virtual file
 */
void parts_load(char* path){
    //Open the disk file
    file_handle_t disk;
    diskio_open(path, &disk, DISKIO_FILE_ACCESS_READ);
    diskio_seek(&disk, 0);
}