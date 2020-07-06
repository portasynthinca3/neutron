#ifndef PART_H
#define PART_H

#include "../../stdlib.h"
#include "./diskio.h"

//Settings

#define MAX_PARTS       256

//Partition GUIDs

#define PART_GUID_NONE      (&(guid_t){.data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}})
#define PART_GUID_MBDP      (&(guid_t){.data = {0xA2, 0xA0, 0xD0, 0xEB, 0xE5, 0xB9, 0x33, 0x44, 0x87, 0xC0, 0x68, 0xB6, 0xB7, 0x26, 0x99, 0xC7}})

//Enumerator definitions

typedef enum {
    PART_TYPE_UNKNOWN,
    PART_TYPE_FAT32
} part_type_t;

//Structure definitions

typedef struct {
    uint64_t lba_start;
    uint64_t lba_end;
    char     name[128];

    char           drive[128];
    uint32_t       part_no;
    uint8_t        is_gpt;
    file_handle_t* drive_file;

    part_type_t    type;
} part_t;

typedef struct {
    uint8_t data[16];
} guid_t;

//Function prototypes

void parts_load(char* path);
void parts_load_gpt(file_handle_t* disk);

part_t* part_get(uint32_t num);

#endif