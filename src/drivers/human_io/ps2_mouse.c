//Neutron Project
//PS/2 mouse driver

#include "./ps2_mouse.h"
#include "./ps2.h"
#include "./mouse.h"
#include "../../stdlib.h"
#include "../gfx.h"

/*
 * Performs an initialization of the PS/2 mouse
 */
void ps2_mouse_init(void){
    gfx_verbose_println("Initializing PS/2 mouse");
    ps2_command(0xD4); //Write to second port
    ps2_send(0xF4);    //Enable mouse data reporting
    ps2_read();        //Read an ACK byte
}

/*
 * Parses data available in the mouse input buffer
 */
void ps2_mouse_parse(uint8_t* buf, uint16_t* head, uint16_t* tail){
    //Read the packet
    uint8_t ms_flags = fifo_popb(buf, head, tail);
    uint8_t ms_x = fifo_popb(buf, head, tail);
    uint8_t ms_y = fifo_popb(buf, head, tail);
    //Bit 3 of flags should always be set
    if(ms_flags & 8){
        //Some local variables
        uint8_t ml, mr = 0;
        int32_t dx, dy = 0;
        //Process it
        if(ms_flags & 0x20) //Bit 5 in flags means delta Y is negative
            dy = -((int32_t)((int32_t)ms_y) | 0xFFFFFFFF00); //We negate the value because PS/2 assumes that the Y axis is looking up, but it's the opposite in our case
        else
            dy = -(int32_t)ms_y;
        if(ms_flags & 0x10) //Bit 4 in flags means delta X is negative
            dx = (int32_t)((int32_t)ms_x) | 0xFFFFFFFF00;
        else
            dx = (int32_t)ms_x;
        //Set mouse button state variables
        ml = ms_flags & 1;
        mr = (ms_flags & 2) >> 1;
        //Call the general driver callback
        mouse_callback(dx, dy, ml, mr);
    }
}