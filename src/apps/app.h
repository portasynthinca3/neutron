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

void app_register(app_t app);
uint32_t app_count(void);
app_t* app_get_id(uint32_t id);
app_t* app_get_name(char* name);

#endif