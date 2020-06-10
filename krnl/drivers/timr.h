#ifndef PIT_H
#define PIT_H

#include "../stdlib.h"

//Function prototypes

//CPU frequency
uint64_t timr_get_cpu_fq     (void);
void     timr_measure_cpu_fq (void);
//LAPIC timer control
void timr_init (void);
void timr_stop (void);

#endif