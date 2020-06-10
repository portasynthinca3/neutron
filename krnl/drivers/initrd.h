#ifndef INITRD_H
#define INITRD_H

#include "../stdlib.h"

typedef struct {
    uint32_t location;
    uint32_t size;
    char name[56];
} __attribute__((packed)) initrd_file_t;

//Function prototypes

uint8_t       initrd_init     (void);
initrd_file_t initrd_read     (char* name);
uint8_t*      initrd_contents (char* name);

#endif