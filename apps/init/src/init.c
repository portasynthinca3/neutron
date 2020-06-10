//Neutron project
//Default initialization application

#include "nlib.h"
#include "app_desc.h"

void main(void* args){
    //A temporary buffer for messages
    char buf[256];
    //Print some info
    sprintf(buf, "Neutron standard initializer version %s compiled on %s %s",
                 __APP_VERSION, __DATE__, __TIME__);
    _km_write("init", buf);
    FILE* a = fopen("/sys/kvers", "r");
    char buf2[64];
    fgets(buf2, 64, a);
    sprintf(buf, "Running on Neutron kernel version %s", buf2);
    _km_write("init", buf);
    //Open config file for reading
    _km_write("init", "loading config file");
    FILE* fp = fopen("/initrd/init.cfg", "r");
    if(fp == NULL){
        _km_write("init", "error loading config file (/initrd/init.cfg)");
        while(1);
    }
    //Read data by lines
    char line[512];
    uint64_t line_no = 1;
    while(true){
        //Read one line
        fgets(line, 512, fp);
        //If it's empty, EOF has been reached
        if(strlen(line) == 0)
            break;
        //Skip the line if it's empty or starts with a hash
        if(line[0] == '\n' || line[0] == '#'){
            line_no++;
            continue;
        }
        //Truncate the trailing \n
        if(line[strlen(line) - 1] == '\n')
            line[strlen(line) - 1] = 0;
        //Get key and value
        char key[256];
        char val[256];
        memset(key, 0, sizeof(key));
        memset(val, 0, sizeof(val));
        //Find occurence of the equality sign
        char* eq_occur = strchr(line, '=');
        memcpy(key, line, eq_occur - line);
        strcpy(val, eq_occur + 1);

        memset(buf, 0, 256);

        //Parse the commands
        if(strcmp(key, "start") == 0){ //Start an application
            //Load the specified application
            uint64_t status = _task_load(val, TASK_PRIVL_INHERIT);
            //Print an error or a success message
            if(status == ELF_STATUS_OK){
                sprintf(buf, "[%i]: started application %s", line_no, val);
                _km_write("init", buf);
            } else {
                sprintf(buf, "[%i]: error starting application %s", line_no, val);
                _km_write("init", buf);
            }
        } else if(strcmp(key, "print") == 0){ //Print a string
            sprintf(buf, "[%i]: print: %s", line_no, val);
            _km_write("init", buf);
        } else { //Unknown command
            sprintf(buf, "[%i]: unknown command %s", line_no, key);
            _km_write("init", buf);
        }

        //Increment the line number
        line_no++;
    }

    //At this point, init's job is done.
    fclose(fp);

    while(1);
}