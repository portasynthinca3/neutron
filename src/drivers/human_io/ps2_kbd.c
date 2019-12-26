//Neutron Project
//PS/2 keyboard driver

#include "./ps2_kbd.h"
#include "./ps2.h"
#include "../../stdlib.h"
#include "../gfx.h"

/*
 * Performs an initialization of the PS/2 keyboard
 */
void ps2_kbd_init(void){
    gfx_verbose_println("Initializing PS/2 keyboard");
    ps2_send(0xF6); //Set default parameters
    ps2_send(0xF4); //Enable scanning
    ps2_send(0xF0); //Set scancode set 2
    ps2_send(2);
}

/*
 * Set keyboard "lock" LEDs
 */
void ps2_kbd_leds(uint8_t scroll, uint8_t num, uint8_t caps){
    ps2_send(0xED); //Command: set LEDs
    ps2_send((scroll & 1) | ((num & 1) << 1) | ((caps & 1) << 2));
}

/*
 * Parses data available in the keyboard input buffer
 */
void ps2_kbd_parse(uint8_t* buf, uint16_t* head, uint16_t* tail){

}