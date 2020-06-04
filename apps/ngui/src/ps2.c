//Neutron project
//Standard GUI manager - PS/2 interface controller

#include "nlib.h"
#include "app_desc.h"

#include "ps2.h"

//Keyboard and mouse virtual files
FILE* kbd = NULL;
FILE* mouse = NULL;
//Mouse movement event callback
void(*mouse_cb)(mouse_evt_t) = NULL;
//Mouse stream parsing state
uint8_t mouse_byte = 0;
uint8_t mouse_flags = 0;
mouse_evt_t mouse_cur_evt = {0, 0, 0, 0};

/*
 * Sets the mouse event callback
 */
void ps2_set_mouse_cb(void(*cb)(mouse_evt_t)){
    mouse_cb = cb;
}

/*
 * Performs the initialization
 */
void ps2_init(void){
    //Open files
    kbd = fopen("/dev/ps21", "r");
    mouse = fopen("/dev/ps22", "r+");
    if(kbd == NULL){
        _km_write(__APP_SHORT_NAME, "failed to open PS/2 kbd port");
        abort();
    }
    if(mouse == NULL){
        _km_write(__APP_SHORT_NAME, "failed to open PS/2 mouse port");
        abort();
    }
    //Set mouse sampling rate to 60
    fputc(0xF3, mouse);
    fputc(60, mouse);
    //Flush any data
    while(fgetc(kbd) != -1);
    while(fgetc(mouse) != -1);
}

/*
 * Checks PS/2 device buffers and parses them if needed
 */
void ps2_check(void){
    //TODO: keyboard
    //Check mouse
    uint64_t mouse_data = 0;
    while((mouse_data = fgetc(mouse)) != -1){
        //First byte - flags
        if(mouse_byte == 0){
            mouse_flags = mouse_data;
            if(mouse_flags & 1)
                mouse_cur_evt.buttons |= MOUSE_BTN_LEFT;
            if(mouse_flags & 2)
                mouse_cur_evt.buttons |= MOUSE_BTN_RIGHT;
            if(mouse_flags & 4)
                mouse_cur_evt.buttons |= MOUSE_BTN_MIDDLE;
        }
        //Second byte - delta x
        else if(mouse_byte == 1) {
            mouse_cur_evt.rel_x = (uint32_t)mouse_data - (((uint64_t)mouse_flags << 4) & 0x100);
        }
        //Second byte - delta y
        else if(mouse_byte == 2){
            mouse_cur_evt.rel_y = (uint32_t)mouse_data - (((uint64_t)mouse_flags << 3) & 0x100);
            mouse_cur_evt.rel_y = -mouse_cur_evt.rel_y;
            //Call the event handler (last byte in the sequence)
            if(mouse_cb != NULL)
                mouse_cb(mouse_cur_evt);
        }

        mouse_byte = (mouse_byte + 1) % 3;
    }
}