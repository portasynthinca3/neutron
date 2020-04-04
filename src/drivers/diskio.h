#ifndef DISKIO_H
#define DISKIO_H

#include "../stdlib.h"

//Definitions

#define DISKIO_FILE_ACCESS_READ                     1
#define DISKIO_FILE_ACCESS_WRITE                    2
#define DISKIO_FILE_ACCESS_READ_WRITE               (DISKIO_FILE_ACCESS_READ | DISKIO_FILE_ACCESS_WRITE)

#define DISKIO_STATUS_OK                            0
#define DISKIO_STATUS_FILE_NOT_FOUND                1
#define DISKIO_STATUS_WRITE_PROTECTED               2

#define DISKIO_BUS_INITRD                           0

//Settings

#define DISKIO_MAX_MAPPINGS                         64

//Structures

typedef struct {
    uint16_t bus_type;
    uint64_t device_no;
} diskio_dev_t;

typedef struct {
    uint8_t used;
    diskio_dev_t device;
    char mapped_at[256];
} diskio_map_t;

typedef struct {
    char name[256];
    uint64_t size;
    uint64_t medium_start;
    diskio_dev_t device;
} file_info_t;

typedef struct {
    file_info_t info;
    uint8_t mode;
    uint64_t position;
} file_handle_t;

//Function prototypes

void diskio_init(void);
void diskio_mount(diskio_dev_t device, char* path);
uint8_t diskio_open(char* path, file_handle_t* handle, uint8_t mode);
uint8_t diskio_read(file_handle_t* handle, void* buf, uint64_t len);
void diskio_close(file_handle_t* handle);

#endif