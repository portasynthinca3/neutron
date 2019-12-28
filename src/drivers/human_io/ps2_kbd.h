#ifndef PS2_KBD_H
#define PS2_KBD_H

#include "../../stdlib.h"
#include "./kbd.h"

//Structure that defines a
//    PS/2 scancode <-> Neutron key number
//  relationship
typedef struct {
    uint16_t ps2_scan;
    kbd_scan_code_t num;
} ps2_kbd_scan_map_t;

void ps2_kbd_init(void);
void ps2_kbd_leds(uint8_t scroll, uint8_t num, uint8_t caps);
kbd_scan_code_t ps2_search_key_map(uint16_t scan);
void ps2_kbd_scan_done(void);
void ps2_kbd_parse(uint8_t* buf, uint16_t* head, uint16_t* tail);

#endif