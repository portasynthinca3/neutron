#ifndef MOUSE_H
#define MOUSE_H

#include "../../stdlib.h"

void mouse_abs(int32_t* x, int32_t* y);
void mouse_delta(int32_t* x, int32_t* y);
void mouse_buttons(uint8_t* left, uint8_t* right);

void mouse_callback(int32_t dx, int32_t dy, uint8_t l, uint8_t r);
void mouse_frame_end(void);

#endif