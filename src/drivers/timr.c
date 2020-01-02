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
    //And we will use PIT channel 2 for that
    outb(0x61, (inb(0x61) & 0xFD) | 1);
    outb(0x42, 0x9B); //Wait 0x2E9B PIT clock cycles
    outb(0x42, 0x2E); //(10 ms)
    outb(0x61, inb(0x61) & 0xFE); //Reset one-shot mode
    outb(0x61, inb(0x61) | 1);
    apic_reg_wr(LAPIC_REG_TIMR_INITCNT, 0xFFFFFFFF); //Set an initial count of uint32_t_max-1
    //Wait for the PIT to reach zero
    while(!(inb(0x61) & 0x20));
    //Stop the APIC timer
    apic_reg_wr(LAPIC_REG_LVT_TIM, 0x10000);
    //Get the counter value
    uint32_t cnt_val = apic_reg_rd(LAPIC_REG_TIMR_CURCNT);
    uint32_t ticks_in_10ms = 0xFFFFFFFF - cnt_val;
    apic_reg_wr(LAPIC_REG_LVT_TIM, 0x20000 | 32); //Enable the timer with interrupt vector #32
    apic_reg_wr(LAPIC_REG_TIMR_DIVCONF, 3); //Set the divider to 16
    apic_reg_wr(LAPIC_REG_TIMR_INITCNT, ticks_in_10ms * 20); //Set the initial counter value
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