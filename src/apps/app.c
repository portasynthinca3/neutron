//Neutron Project
//Application database

#include "./app.h"
#include "../drivers/gfx.h"

//The list of loaded apps
app_t loaded_apps[32];
//Where to load the next app
uint32_t next_app = 0;

/*
 * Registers an application
 */
void app_register(app_t app){
    //Print the information about the application we're loading
    char temp[100];
    char temp2[10];
    temp[0] = 0;
    strcat(temp, "Registering app \"");
    strcat(temp, app.name);
    strcat(temp, "\" ver. ");
    strcat(temp, sprintu(temp2, app.ver_major, 1));
    strcat(temp, ".");
    strcat(temp, sprintu(temp2, app.ver_minor, 1));
    strcat(temp, ".");
    strcat(temp, sprintu(temp2, app.ver_patch, 1));
    strcat(temp, " as ID ");
    strcat(temp, sprintu(temp2, next_app, 1));
    gfx_verbose_println(temp);
    //Actually store the application data
    loaded_apps[next_app++] = app;
}

/*
 * Returns the count of registered apps
 */
uint32_t app_count(void){
    return next_app;
}

/*
 * Returns an application based on its ID
 */
app_t* app_get_id(uint32_t id){
    return &loaded_apps[id];
}

/*
 * Returns an application based on its name
 */
app_t* app_get_name(char* name){
    for(uint32_t i = 0; i < app_count(); i++)
        if(strcmp(app_get_id(i)->name, name) == 0)
            return app_get_id(i);
    return NULL;
}