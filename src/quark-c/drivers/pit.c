//Neutron Project
//Programmable Interval Timer (PIT) driver

#include "./pit.h"
#include "../stdlib.h"

uint64_t ticks = 0;

/*
 * This is called by Quark whenever an IRQ0 occurs
 */
void pit_irq0(void){
    //Increment the tick count
    ticks++;
}

/*
 * Configures PIT to generate IRQ0 at its input frequency divided by some value
 */
void pit_configure_irq0_ticks(uint16_t div){
    //Configure channel 0 as lo-hi access, rate generation, 16-bit binary mode
    outb(PIT_MC, 0b00110100);
    //Save the flags and disable interrupts
    __asm__ volatile("pushfl; cli");
    //Output lower bits of the divider
    outb(PIT_C0, div);
    //Output higher bits of the divider
    outb(PIT_C0, div >> 8);
    //Restore the flags, enabling interrupts if they were enabled before
    __asm__ volatile("popfl");
}

/*
 * Returns the amount of ticks since PIT initialization
 */
uint64_t pit_ticks(void){
    return ticks;
}