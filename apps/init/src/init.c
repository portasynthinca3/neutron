//Neutron project
//Default initialization application

#include "nlib.h"
#include "app_desc.h"

void main(void* args){
    //Print some info
    _gfx_println_verbose("Neutron standard initializer, version:");
    _gfx_println_verbose(__APP_VERSION);
    _gfx_println_verbose("Loading config file");
    //Open config file for reading
    FILE* fp = fopen("/initrd/init.cfg", "r");
    if(fp == NULL){
        _gfx_println_verbose("Error loading config file");
        while(1);
    }
    //Read data by lines
    char buf[512];
    buf[0] = 1;
    while(buf[0] != 0){
        fgets(buf, 512, fp);
        //Truncate the trailing \n,
        //  becuase _gfx_println_verbose inserts \n by itself
        if(buf[strlen(buf) - 1] == '\n')
            buf[strlen(buf) - 1] = 0;
        _gfx_println_verbose(buf);
    }

    while(1);
}