//Neutron Project
//General disk I/O

#include "./diskio.h"
#include "../stdlib.h"
#include "../mtask/mtask.h"
#include "../krnl.h"

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
            krnl_write_msgf(__FILE__, "mounted dev 0x%i.0x%i to %s", device.bus_type, device.device_no, path);
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
                    handle->signature = 0xFFFF0000AAAA5555;
                    handle->pid = mtask_get_uid();
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
    //If no mappings were found, return an error
    return DISKIO_STATUS_FILE_NOT_FOUND;
}

/*
 * Reads file contents into buffer
 */
uint64_t diskio_read(file_handle_t* handle, void* buf, uint64_t len){
    //Check the signature
    if(handle->signature != 0xFFFF0000AAAA5555)
        return DISKIO_STATUS_INVL_SIGNATURE;
    //Check the PID of the owner
    if(handle->pid != mtask_get_uid())
        return DISKIO_STATUS_NOT_ALLOWED;
    //Limit the length to the remaining file length
    uint64_t act_len = len;
    if(len > handle->info.size - handle->position)
        act_len = handle->info.size - handle->position;
    //Different device types require different access schemes
    switch(handle->info.device.bus_type){
        case DISKIO_BUS_INITRD: {
            //Copy data into the buffer
            memcpy(buf, (const void*)(handle->info.medium_start + handle->position), act_len);
            //Shift the position
            handle->position += act_len;
            //Return status: OK if read everything, EOF if the remaining length is smaller than the requested one
            if(act_len != len)
                return DISKIO_STATUS_EOF | (act_len << 32);
            else
                return DISKIO_STATUS_OK;
        } break;
    }
}

/*
 * Closes the file
 */
void diskio_close(file_handle_t* handle){
    //Currently does nothing
}