#ifndef PS2_H
#define PS2_h

#include "../stdlib.h"

//PS/2 controller data port
#define PS2_CONT_DATA           0x60
//PS/2 controller status port
#define PS2_CONT_STATUS         0x64
//PS/2 controller command port
#define PS2_CONT_COMM           0x64

//Keyboard buffer size in bytes
#define KEYBOARD_BUFFER_SIZE 128
//Mouse buffer size in bytes
#define MOUSE_BUFFER_SIZE 128

uint8_t ps2_cont_status(void);
void ps2_wait_send(void);
void ps2_wait_recv(void);
void ps2_command(uint8_t c);
uint8_t ps2_read(void);
void ps2_send(uint8_t d);
void ps2_init(void);
void ps2_alloc_buf(void);
void ps2_poll(int32_t* mx, int32_t* my, uint8_t* ml, uint8_t* mr);

#endif