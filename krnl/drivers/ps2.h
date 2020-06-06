#ifndef PS2_H
#define PS2_H

#include "../stdlib.h"

//Settings

#define PS2_BUF_SIZE            2048

//PS/2 controller I/O ports

#define PS2_DATA_PORT           0x60
#define PS2_STATUS_REG          0x64
#define PS2_COMMAND_REG         0x64

//Function prototypes

//Initializationn
void ps2_init (void);
//Controller I/O
uint8_t ps2_read_byte  (void);
void    ps2_write_byte (uint8_t val);
//Interrupt handlers
void ps21_intr (void);
void ps22_intr (void);
//FIFO buffers
int  ps21_read  (void);
int  ps22_read  (void);
void ps21_write (uint8_t val);
void ps22_write (uint8_t val);
void ps21_flush (void);
void ps22_flush (void);

#endif