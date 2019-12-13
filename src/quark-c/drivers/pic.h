#ifndef PIC_H
#define PIC_H

#include "../stdlib.h"

//Define I/O ports for both PICs

#define PIC1_BASE           0x20
#define PIC1_COMM           (PIC1_BASE + 0)
#define PIC1_DATA           (PIC1_BASE + 1)
#define PIC2_BASE           0xA0
#define PIC2_COMM           (PIC2_BASE + 0)
#define PIC2_DATA           (PIC2_BASE + 1)

void pic_init(uint8_t offs1, uint8_t offs2);
void pic_send_eoi(uint8_t irq);

#endif