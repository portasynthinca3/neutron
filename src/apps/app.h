#ifndef APP_H
#define APP_H

#include "../stdlib.h"

typedef struct {
    //The entry point for the application
    void(*entry_point)(void);
    //The icon of the application
    uint8_t* icon;
    //The name of the application
    char* name;
    //The version of the application
    uint16_t ver_major;
    uint16_t ver_minor;
    uint16_t ver_patch;
} app_t;

#endif