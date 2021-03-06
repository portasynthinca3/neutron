#ifndef DISKIO_H
#define DISKIO_H

#include "../../stdlib.h"

//Definitions

//File access modes
#define DISKIO_FILE_ACCESS_READ                     1
#define DISKIO_FILE_ACCESS_WRITE                    2
#define DISKIO_FILE_ACCESS_READ_WRITE               (DISKIO_FILE_ACCESS_READ | DISKIO_FILE_ACCESS_WRITE)

//Operation statuses
#define DISKIO_STATUS_OK                            0
#define DISKIO_STATUS_FILE_NOT_FOUND                1
#define DISKIO_STATUS_READ_PROTECTED                2
#define DISKIO_STATUS_WRITE_PROTECTED               3
#define DISKIO_STATUS_NOT_ALLOWED                   4
#define DISKIO_STATUS_EOF                           6
#define DISKIO_STATUS_ALREADY_OPENED                7
#define DISKIO_STATUS_SEEKING_ERR                   8

//Devices/filesytems (buses)
#define DISKIO_BUS_INITRD                           0
#define DISKIO_BUS_BRIDGE                           1
#define DISKIO_BUS_BRIDGE_LIST                      2
#define DISKIO_BUS_SYSTEM                           3
#define DISKIO_BUS_DEVICE                           4
#define DISKIO_BUS_SATA                             5
#define DISKIO_BUS_PART                             6
#define DISKIO_BUS_FILESYSTEM                       7

//Virtual files in the /sys/ directory
#define SYS_FILE_CPUFQ                              0
#define SYS_FILE_KVERN                              1
#define SYS_FILE_KVERS                              2
#define SYS_FILE_DRES                               3
#define SYS_FILE_TIME                               4

//Virtual files in the /dev/ directory
#define DEV_FILE_PS21                               0
#define DEV_FILE_PS22                               1
#define DEV_FILE_FB                                 2

//Settings
#define DISKIO_MAX_MAPPINGS                         64
#define DISKIO_BRIDGE_BUF_SZ                        4096
#define DISKIO_MAX_PATH_LEN                         256
#define DISKIO_MAX_FILES_IN_DIR                     256

//Structures

typedef struct _bridge_s{
    uint8_t  is_bridge;
    uint64_t to_pid;

    uint8_t* send_buf;
    uint64_t send_pos;

    uint8_t* read_buf;
    uint64_t read_pos;

    struct _bridge_s* other;
} bridge_t;

typedef struct {
    uint64_t bus_type;
    uint64_t device_no;
    void*    file;

    //only applicable if the file is a bridge
    bridge_t bridge;
} diskio_dev_t;

typedef struct {
    uint8_t      used;
    diskio_dev_t device;
    char         mapped_at[DISKIO_MAX_PATH_LEN];
} diskio_map_t;

typedef struct {
    diskio_dev_t device;
    char         name[DISKIO_MAX_PATH_LEN];
    uint64_t     size;
    uint64_t     medium_start;
} file_info_t;

typedef struct {
    file_info_t info;
    uint64_t    pid;
    uint8_t     mode;
    uint64_t    position;
} file_handle_t;

typedef struct {
    diskio_dev_t device;
    char         path[DISKIO_MAX_PATH_LEN];
    file_info_t  files[DISKIO_MAX_FILES_IN_DIR];
} dir_info_t;

typedef struct {
    dir_info_t info;
} dir_handle_t;

//Function prototypes

void     diskio_init  (void);
void     diskio_mount (diskio_dev_t device, char* path);
uint8_t  diskio_open  (char* path, file_handle_t* handle, uint8_t mode);
uint64_t diskio_read  (file_handle_t* handle, void* buf, uint64_t len);
uint64_t diskio_write (file_handle_t* handle, void* buf, uint64_t len);
uint64_t diskio_seek  (file_handle_t* handle, uint64_t pos);
void     diskio_close (file_handle_t* handle);

#endif