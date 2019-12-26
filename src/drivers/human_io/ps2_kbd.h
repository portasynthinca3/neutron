#ifndef PS2_KBD_H
#define PS2_KBD_H

#include "../../stdlib.h"

void ps2_kbd_init(void);
void ps2_kbd_leds(uint8_t scroll, uint8_t num, uint8_t caps);

#endif