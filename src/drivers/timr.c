//Neutron Project
//Local APIC timer driver

#include "./timr.h"
#include "./apic.h"
#include "../stdlib.h"

//The variable for keeping track of milliseconds since timer initialization
uint64_t timr_millis = 0;

/*
 * Initializes the timer
 */
void timr_init(void){
    apic_reg_wr(LAPIC_REG_TPR, 0);
    //Set the 16x divider
    apic_reg_wr(LAPIC_REG_TIMR_DIVCONF, 0x3);
    //In order to use the APIC timer, we need to get
    //  an initial clock source
    //And we will count 100M CPU cycles for that
    uint64_t tsc_start = rdtsc();
    apic_reg_wr(LAPIC_REG_TIMR_INITCNT, 0xFFFFFFFF); //Set an initial count of uint32_t_max-1
    //Wait for 10M cycles to pass
    while(rdtsc() - tsc_start < 100000);
    //Stop the APIC timer
    apic_reg_wr(LAPIC_REG_LVT_TIM, 0x10000);
    //Get the counter value
    uint32_t cnt_val = apic_reg_rd(LAPIC_REG_TIMR_CURCNT);
    uint32_t ticks_in_cycles = 0xFFFFFFFF - cnt_val;
    apic_reg_wr(LAPIC_REG_LVT_TIM, 0x20000 | 32); //Enable the timer with interrupt vector #32
    apic_reg_wr(LAPIC_REG_TIMR_DIVCONF, 0); //Set the divider to 2
    apic_reg_wr(LAPIC_REG_TIMR_INITCNT, ticks_in_cycles); //Set the initial counter value
}

/*
 * Stops the timer
 */
void timr_stop(void){
    apic_reg_wr(LAPIC_REG_LVT_TIM, apic_reg_rd(LAPIC_REG_LVT_TIM) & ~0x20000);
}

/*
 * Increment the millisecond counter
 */
void timr_tick(void){
    timr_millis++;
}

/*
 * Returns the amount of milliseconds that have passed since timer initialization
 */
uint64_t timr_ms(void){
    return timr_millis / 2;
}