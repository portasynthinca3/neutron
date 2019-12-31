#ifndef PIT_H
#define PIT_H

#include "../stdlib.h"

void timr_init(void);
void timr_tick(void);

uint64_t timr_ms(void);

#endif