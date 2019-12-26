#ifndef PS2_MOUSE_H
#define PS2_MOUSE_H

#include "../../stdlib.h"

void ps2_mouse_init(void);
void ps2_mouse_parse(uint8_t* buf, uint16_t* head, uint16_t* tail);

#endif