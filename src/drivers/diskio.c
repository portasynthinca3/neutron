//Neutron Project
//General disk I/O

#include "./diskio.h"
#include "../stdlib.h"

#include "./initrd.h"

diskio_map_t* mappings;

/*
 * Initializes DISKIO stuff
 */
void diskio_init(void){
    mappings = calloc(DISKIO_MAX_MAPPINGS, sizeof(diskio_map_t));
}

/*
 * Mount a device at path
 */
void diskio_mount(diskio_dev_t device, char* path){
    //Find an unused entry
    for(uint32_t i = 0; i < DISKIO_MAX_MAPPINGS; i++){
        if(!mappings[i].used){
            //Copy the path
            memcpy(mappings[i].mapped_at, path, strlen(path) + 1);
            //Copy the device ID
            mappings[i].device = device;
            //Mark the entry as used
            mappings[i].used = 0;
            //Return
            return;
        }
    }
}

/*
 * Opens a file on the disk
 */
uint8_t diskio_open(char* path, file_handle_t* handle, uint8_t mode){
    //Go through mappings
    for(uint32_t i = 0; i < DISKIO_MAX_MAPPINGS; i++){
        //Find a mapping that the path is relative to
        if(memcmp(path, mappings[i].mapped_at, strlen(mappings[i].mapped_at)) == 0){
            //Get the filename
            char* name = path + strlen(mappings[i].mapped_at);
            //Different device types require different access schemes
            switch(mappings[i].device.bus_type){
                case DISKIO_BUS_INITRD:{
                    //Fetch INITRD file info
                    initrd_file_t file = initrd_read(name);
                    //If there are no files with this name, return an error
                    if(file.location == 0)
                        return DISKIO_STATUS_FILE_NOT_FOUND;
                    //INITRD is read-only
                    if(mode != DISKIO_FILE_ACCESS_READ)
                        return DISKIO_STATUS_WRITE_PROTECTED;
                    //Setup the handle
                    handle->mode = mode;
                    handle->position = 0;
                    handle->info.medium_start = (uint64_t)initrd_contents(name);
                    memcpy(handle->info.name, name, strlen(name) + 1);
                    handle->info.size = file.size;
                    handle->info.device = mappings[i].device;
                    return DISKIO_STATUS_OK;
                } break;
            }
        }
    }
}

/*
 * Reads file contents into buffer
 */
uint8_t diskio_read(file_handle_t* handle, void* buf, uint64_t len){
    //Different device types require different access schemes
    switch(handle->info.device.bus_type){
        case DISKIO_BUS_INITRD:{
            //Copy data into the buffer
            memcpy(buf, (const void*)(handle->info.medium_start + handle->position), len);
            //Shift the position
            handle->position += len;
        } break;
    }
}

/*
 * Closes the file
 */
void diskio_close(file_handle_t* handle){
    //Currently does nothing
}