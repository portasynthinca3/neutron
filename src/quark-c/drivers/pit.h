#ifndef PIT_H
#define PIT_H

#include "../stdlib.h"

//PIT input frequency
#define PIT_FQ 1193182

//PIT ports

#define PIT_C0 0x40
#define PIT_C1 0x41
#define PIT_C2 0x42
#define PIT_MC 0x43

void pit_irq0(void);

void pit_configure_irq0_ticks(uint16_t div);
uint64_t pit_ticks(void);

#endif