//Neutron Project
//Local APIC timer driver

#include "./timr.h"
#include "./apic.h"
#include "../stdlib.h"
#include "../krnl.h"

//CPU clock frequency in Hz
uint64_t cpu_fq_hz;

/*
 * Returns CPU frequency in Hz
 */
uint64_t timr_get_cpu_fq(void){
    return cpu_fq_hz;
}

/*
 * Measures the CPU frequency and stores it in cpu_fq_hz
 */
void timr_measure_cpu_fq(void){
    uint64_t sum = 0;
    for(int i = 0; i < 50; i++){
        //Disable ch2 counting and PC speaker (clear bits 0 and 1)
        outb(0x61, inb(0x61) & ~3);
        //Accept a new value in chan 2 latch reg
        outb(0x43, 0b10110000);
        //Write the initial counter value
        uint16_t cnt_val = 1193182 / 20;
        outb(0x42, cnt_val >> 8);
        outb(0x42, cnt_val & 0xFF);
        //Enable ch2 counting and read TSC
        outb(0x61, inb(0x61) | 1);
        uint64_t tsc_start = rdtsc();
        //Wait until bit 5 goes high (this means that the time needed passed)
        while((inb(0x61) & (1 << 5)) == 0);
        uint64_t tsc_diff = rdtsc() - tsc_start;
        //Calculate CPU speed
        sum += tsc_diff * 394;
    }
    cpu_fq_hz = sum / 50;
    krnl_write_msgf(__FILE__, "CPU frequency detected: %i MHz", cpu_fq_hz / 1000 / 1000);

    krnl_writec_f("Measured the CPU frequency (%i MHz)\r\n", cpu_fq_hz / 1000 / 1000);
}

/*
 * Initializes the timer
 */
void timr_init(void){
    lapic_reg_wr(LAPIC_REG_TPR, 0);
    //Set the 16x divider
    lapic_reg_wr(LAPIC_REG_TIMR_DIVCONF, 0x3);
    //In order to use the APIC timer, we need to get an initial clock source
    //And we will wait for 1 millisecond for that
    uint64_t tsc_start = rdtsc();
    lapic_reg_wr(LAPIC_REG_TIMR_INITCNT, 0xFFFFFFFF); //Set an initial count of uint32_t_max-1
    //Wait for 1 ms to pass
    while(rdtsc() - tsc_start < cpu_fq_hz / 1000);
    //Stop the APIC timer
    lapic_reg_wr(LAPIC_REG_LVT_TIM, 0x10000);
    //Get the counter value
    uint32_t cnt_val = lapic_reg_rd(LAPIC_REG_TIMR_CURCNT);
    uint32_t ticks_in_cycles = 0xFFFFFFFF - cnt_val;
    krnl_write_msgf(__FILE__, "LAPIC timer ticks in 1 ms (%i CPU cycles): %i", cpu_fq_hz / 1000, ticks_in_cycles);
    lapic_reg_wr(LAPIC_REG_TIMR_DIVCONF, 0); //Set the divider to 2
    lapic_reg_wr(LAPIC_REG_TIMR_INITCNT, ticks_in_cycles); //Set the initial counter value
    lapic_reg_wr(LAPIC_REG_LVT_TIM, 0x20000 | 32); //Enable the timer with interrupt vector #32
    krnl_write_msgf(__FILE__, "LAPIC timer initialized");
}

/*
 * Stops the timer
 */
void timr_stop(void){
    lapic_reg_wr(LAPIC_REG_LVT_TIM, lapic_reg_rd(LAPIC_REG_LVT_TIM) & ~0x20000);
}


