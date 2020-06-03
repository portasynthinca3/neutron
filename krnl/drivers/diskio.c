//Neutron Project
//General disk I/O

#include "./diskio.h"
#include "../stdlib.h"
#include "../mtask/mtask.h"
#include "../krnl.h"
#include "./timr.h"
#include "./gfx.h"

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
    task_t* cur_task = mtask_get_by_pid(mtask_get_pid());
    //Return an error if that process has already opened this file
    if(cur_task != NULL)
        for(int i = 0; i < MTASK_MAX_OPEN_FILES; i++)
            if(strcmp(cur_task->open_files[i]->info.name, path) == 0)
                return DISKIO_STATUS_ALREADY_OPENED;
    //Check if it's a bridge
    if(memcmp(path, "/bridge/", 8) == 0){
        char* pid_str = path + 8;
        //Check if it's a bridge list file
        if(strcmp(pid_str, "pending") == 0){
            //It's read-only
            if(mode & DISKIO_FILE_ACCESS_WRITE)
                return DISKIO_STATUS_WRITE_PROTECTED;
            //Setup the handle
            handle->signature = DISKIO_HANDLE_SIGNATURE;
            handle->pid = mtask_get_pid();
            handle->mode = mode;
            strcpy(handle->info.name, path);
            handle->info.device = (diskio_dev_t){.bus_type = DISKIO_BUS_BRIDGE_LIST};
            mtask_add_open_file(handle);
            return DISKIO_STATUS_OK;
        }
        //Else, it's a bridge
        //Bridges are read-write-only
        if(mode != DISKIO_FILE_ACCESS_READ_WRITE)
            return DISKIO_STATUS_NOT_ALLOWED;
        //Parse PID
        uint64_t pid = atoi(pid_str);
        task_t* task = mtask_get_by_pid(pid);
        //Check if that PID has already opened a bridge to this one
        for(int i = 0; i < MTASK_MAX_OPEN_FILES; i++){
            bridge_t* bridge = &task->open_files[i]->info.device.bridge;
            if(bridge->is_bridge && bridge->to_pid == cur_task->pid){
                //Setup the handle
                handle->signature = DISKIO_HANDLE_SIGNATURE;
                handle->pid = cur_task->pid;
                handle->mode = mode;
                strcpy(handle->info.name, path);
                handle->info.device = (diskio_dev_t){
                    .bus_type = DISKIO_BUS_BRIDGE,
                    .bridge = (bridge_t){
                        .is_bridge = 1,
                        .to_pid = pid,
                        .send_buf = bridge->read_buf,
                        .send_pos = 0,
                        .read_buf = bridge->send_buf,
                        .read_pos = 0,
                        .other = bridge
                    }
                };
                mtask_add_open_file(handle);
                bridge->other = &handle->info.device.bridge;
                return DISKIO_STATUS_OK;
            }
        }
        //If not, create a new one
        handle->signature = DISKIO_HANDLE_SIGNATURE;
        handle->pid = cur_task->pid;
        handle->mode = mode;
        strcpy(handle->info.name, path);
        handle->info.device = (diskio_dev_t){
            .bus_type = DISKIO_BUS_BRIDGE,
            .bridge = (bridge_t){
                .is_bridge = 1,
                .to_pid = pid,
                .send_buf = (uint8_t*)malloc(DISKIO_BRIDGE_BUF_SZ),
                .send_pos = 0,
                .read_buf = (uint8_t*)malloc(DISKIO_BRIDGE_BUF_SZ),
                .read_pos = 0
            }
        };
        mtask_add_open_file(handle);
        return DISKIO_STATUS_OK;
    }
    //Check if it's a system file
    if(memcmp(path, "/sys/", 5) == 0){
        //Check privileges
        if(cur_task->privl & TASK_PRIVL_SYSFILES == 0)
            return DISKIO_STATUS_NOT_ALLOWED;
        char* name = path + 5;
        //Setup the handle
        handle->signature = DISKIO_HANDLE_SIGNATURE;
        handle->pid = cur_task->pid;
        handle->mode = mode;
        handle->position = 0;
        strcpy(handle->info.name, path);
        handle->info.size = 64;
        handle->info.device.bus_type = DISKIO_BUS_SYSTEM;
        handle->info.device.bridge.is_bridge = 0;
        //Determine the actual system file
        if(strcmp(name, "cpufq") == 0){
            if(mode == DISKIO_FILE_ACCESS_READ)
                handle->info.device.device_no = SYS_FILE_CPUFQ;
            else
                return DISKIO_STATUS_WRITE_PROTECTED;
        } else if(strcmp(name, "kvern") == 0){
            if(mode == DISKIO_FILE_ACCESS_READ)
                handle->info.device.device_no = SYS_FILE_KVERN;
            else
                return DISKIO_STATUS_WRITE_PROTECTED;
        } else if(strcmp(name, "kvers") == 0){
            if(mode == DISKIO_FILE_ACCESS_READ)
                handle->info.device.device_no = SYS_FILE_KVERS;
            else
                return DISKIO_STATUS_WRITE_PROTECTED;
        } else if(strcmp(name, "dres") == 0){
            if(mode == DISKIO_FILE_ACCESS_READ)
                handle->info.device.device_no = SYS_FILE_DRES;
            else
                return DISKIO_STATUS_WRITE_PROTECTED;
        }
        return DISKIO_STATUS_OK;
    }
    //Check if it's a device file
    if(memcmp(path, "/dev/", 5) == 0){
        //Check privileges
        if(cur_task->privl & TASK_PRIVL_DEVFILES == 0)
            return DISKIO_STATUS_NOT_ALLOWED;
        char* name = path + 5;
        //Setup the handle
        handle->signature = DISKIO_HANDLE_SIGNATURE;
        handle->pid = cur_task->pid;
        handle->mode = mode;
        handle->position = 0;
        strcpy(handle->info.name, path);
        handle->info.size = 64;
        handle->info.device.bus_type = DISKIO_BUS_DEVICE;
        handle->info.device.bridge.is_bridge = 0;
        //Determine the actual device name
        if(strcmp(name, "ps21") == 0){
            if(mode == DISKIO_FILE_ACCESS_READ_WRITE)
                handle->info.device.device_no = DEV_FILE_PS21;
            else
                return DISKIO_STATUS_NOT_ALLOWED;
        } else if(strcmp(name, "ps22") == 0){
            if(mode == DISKIO_FILE_ACCESS_READ_WRITE)
                handle->info.device.device_no = DEV_FILE_PS22;
            else
                return DISKIO_STATUS_NOT_ALLOWED;
        } else if(strcmp(name, "fb") == 0){
            if(mode == DISKIO_FILE_ACCESS_WRITE)
                handle->info.device.device_no = DEV_FILE_FB;
            else
                return DISKIO_STATUS_READ_PROTECTED;
        }
        return DISKIO_STATUS_OK;
    }
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
                    handle->signature = DISKIO_HANDLE_SIGNATURE;
                    handle->pid = mtask_get_pid();
                    handle->mode = mode;
                    handle->position = 0;
                    handle->info.medium_start = (uint64_t)initrd_contents(name);
                    strcpy(handle->info.name, path);
                    handle->info.size = file.size;
                    handle->info.device = mappings[i].device;
                    handle->info.device.bridge.is_bridge = 0;
                    mtask_add_open_file(handle);
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
    if(handle->signature != DISKIO_HANDLE_SIGNATURE)
        return DISKIO_STATUS_INVL_SIGNATURE;
    //Check the PID of the owner
    if(handle->pid != mtask_get_pid())
        return DISKIO_STATUS_NOT_ALLOWED;
    //Check if read access is allowed
    if(handle->mode & DISKIO_FILE_ACCESS_READ == 0)
        return DISKIO_STATUS_READ_PROTECTED;
    //Limit the length to the remaining file length
    uint64_t act_len = len;
    if(len > handle->info.size - handle->position)
        act_len = handle->info.size - handle->position;
    //Different device/FS types require different access schemes
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
        case DISKIO_BUS_BRIDGE: {
            //Limit the length to the amount of bytes written
            uint64_t max_len = handle->info.device.bridge.other->send_pos - 
                               handle->info.device.bridge.read_pos;
            act_len = len;
            if(len > max_len)
                act_len = max_len;
            //Copy the data into the buffer
            memcpy(buf, handle->info.device.bridge.read_buf +
                        handle->info.device.bridge.read_pos, act_len);
            //Advance the read head
            handle->info.device.bridge.read_pos += act_len;
            if(handle->info.device.bridge.read_pos == handle->info.device.bridge.other->send_pos)
                handle->info.device.bridge.read_pos = handle->info.device.bridge.other->send_pos = 0;
            //Return status: OK if read everything, EOF if the remaining length is smaller than the requested one
            if(act_len != len)
                return DISKIO_STATUS_EOF | (act_len << 32);
            else
                return DISKIO_STATUS_OK;
        } break;
        case DISKIO_BUS_BRIDGE_LIST: {
            //Count the number of bridges
            uint32_t br_cnt = 0;
            for(int i = 0; i < MTASK_TASK_COUNT; i++){
                for(int j = 0; j < MTASK_MAX_OPEN_FILES; j++) {
                    bridge_t* bridge = &mtask_get_task_list()[i].open_files[j]->info.device.bridge;
                    if(bridge->is_bridge && bridge->to_pid == handle->pid)
                        br_cnt++;
                }
            }
            //Construct the structure
            uint8_t br_list[4 + (br_cnt * 16)];
            *(uint32_t*)&br_list[0] = br_cnt;
            int entry = 0;
            //Go through each task
            for(int i = 0; i < MTASK_TASK_COUNT; i++){
                task_t* task = &mtask_get_task_list()[i];
                //And each file that task might've opened
                for(int j = 0; j < MTASK_MAX_OPEN_FILES; j++){
                    bridge_t* bridge = &task->open_files[j]->info.device.bridge;
                    if(bridge->is_bridge){
                        //Try to find this task's handle
                        file_handle_t* this_handle = NULL;
                        task_t* this_task = mtask_get_by_pid(handle->pid);
                        for(int k = 0; k < MTASK_MAX_OPEN_FILES; k++){
                            if(this_task->open_files[k]->info.device.bridge.is_bridge &&
                               this_task->open_files[k]->info.device.bridge.to_pid == task->pid){
                                this_handle = this_task->open_files[k];
                                break;
                            }
                        }
                        //Write information
                        *(uint32_t*)&br_list[4  + (entry * 16)] = task->pid;
                        *(uint32_t*)&br_list[8  + (entry * 16)] = bridge->send_pos - bridge->other->read_pos;
                        *(uint64_t*)&br_list[12 + (entry * 16)] = (uint64_t)this_handle;
                        entry++;
                    }
                }
            }
            //Copy it, limiting the number of bytes
            uint64_t act_len = len;
            if(act_len > 4 + (br_cnt * 16))
                act_len = 4 + (br_cnt * 16);
            memcpy(buf, br_list, act_len);
            //Return status: OK if read everything, EOF if the remaining length is smaller than the requested one
            if(act_len != len)
                return DISKIO_STATUS_EOF | (act_len << 32);
            else
                return DISKIO_STATUS_OK;
        } break;
        case DISKIO_BUS_SYSTEM: {
            char fbuf[64];
            //Constents depend on the specfic file
            switch(handle->info.device.device_no){
                case SYS_FILE_CPUFQ:
                    sprintf(fbuf, "%i", timr_get_cpu_fq());
                    break;
                case SYS_FILE_KVERN:
                    sprintf(fbuf, "%i", KRNL_VERSION_NUM);
                    break;
                case SYS_FILE_KVERS:
                    sprintf(fbuf, "%s", KRNL_VERSION_STR);
                    break;
                case SYS_FILE_DRES:
                    sprintf(fbuf, "%ix%i", gfx_res_x(), gfx_res_y());
                    break;
            }
            //Copy the data and advance the position
            act_len = len;
            if(act_len > strlen(fbuf) - handle->position)
                act_len = strlen(fbuf) - handle->position;
            memcpy(buf, fbuf + handle->position, act_len);
            handle->position += act_len;
            //Return status: OK if read everything, EOF if the remaining length is smaller than the requested one
            if(act_len != len)
                return DISKIO_STATUS_EOF | (act_len << 32);
            else
                return DISKIO_STATUS_OK;
        } break;
        case DISKIO_BUS_DEVICE: {

        } break;
    }
}

/*
 * Write buffer contents to file
 */
uint64_t diskio_write(file_handle_t* handle, void* buf, uint64_t len){
    //Check the signature
    if(handle->signature != DISKIO_HANDLE_SIGNATURE)
        return DISKIO_STATUS_INVL_SIGNATURE;
    //Check the PID of the owner
    if(handle->pid != mtask_get_pid())
        return DISKIO_STATUS_NOT_ALLOWED;
    //Check if write access is allowed
    if(handle->mode & DISKIO_FILE_ACCESS_WRITE == 0)
        return DISKIO_STATUS_WRITE_PROTECTED;
    //Different device/FS types require different access schemes
    switch(handle->info.device.bus_type){
        case DISKIO_BUS_BRIDGE: {
            //Limit the length to the amount of bytes written
            uint64_t max_len = DISKIO_BRIDGE_BUF_SZ - handle->info.device.bridge.send_pos;
            uint64_t act_len = len;
            if(len > max_len)
                act_len = max_len;
            //Copy the data
            memcpy(handle->info.device.bridge.send_buf +
                   handle->info.device.bridge.send_pos, buf, act_len);
            //Advance the write head
            handle->info.device.bridge.send_pos += act_len;
            //Return status: OK if wrote everything, EOF if the remaining buffer space is smaller than the requested data length
            if(act_len != len)
                return DISKIO_STATUS_EOF | (act_len << 32);
            else
                return DISKIO_STATUS_OK;
        } break;
        case DISKIO_BUS_DEVICE: {
            //limit the number of bytes
            uint64_t max_len = (gfx_res_x() * gfx_res_y() * 4) - handle->position;
            uint64_t act_len = len;
            if(len > max_len)
                act_len = max_len;
            //copy the data
            memcpy(gfx_buf_another(), buf, act_len);
            //Return status: OK if wrote everything, EOF if the remaining buffer space is smaller than the requested data length
            if(act_len != len)
                return DISKIO_STATUS_EOF | (act_len << 32);
            else
                return DISKIO_STATUS_OK;
        } break;
    }
}

/*
 * Seek to the specified position in file
 */
uint64_t diskio_seek(file_handle_t* handle, uint64_t pos){
    //Check the signature
    if(handle->signature != DISKIO_HANDLE_SIGNATURE)
        return DISKIO_STATUS_INVL_SIGNATURE;
    //Check the PID of the owner
    if(handle->pid != mtask_get_pid())
        return DISKIO_STATUS_NOT_ALLOWED;
    //Check if seeking is allowed
    if(handle->info.device.bus_type == DISKIO_BUS_BRIDGE ||
       handle->info.device.bus_type == DISKIO_BUS_BRIDGE_LIST)
       return DISKIO_STATUS_SEEKING_ERR;
    if(handle->info.device.bus_type == DISKIO_BUS_DEVICE){
        if(handle->info.device.device_no == DEV_FILE_PS21 ||
           handle->info.device.device_no == DEV_FILE_PS22 ||
           handle->info.device.device_no == DEV_FILE_FB)
           return DISKIO_STATUS_SEEKING_ERR;
    }
    //Check the range
    if(pos >= handle->info.size)
        return DISKIO_STATUS_SEEKING_ERR;
    //Set the position
    handle->position = pos;
    return DISKIO_STATUS_OK;
}

/*
 * Closes the file
 */
void diskio_close(file_handle_t* handle){
    mtask_remove_open_file(handle);
    free(handle);
}