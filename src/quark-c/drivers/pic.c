//Neutron Project
//Programmable Interrupt Controller (PIC) driver

#include "./pic.h"
#include "../stdlib.h"

/*
 * Initialize both of the PICs
 */
void pic_init(uint8_t offs1, uint8_t offs2){
    //Save the interrupt masks
    uint8_t m1 = inb(PIC1_DATA);
    uint8_t m2 = inb(PIC2_DATA);
    //Perform a long and complicated initialization sequence
    outb(PIC1_COMM, 0x11);
    outb(PIC2_COMM, 0x11);
    outb(PIC1_DATA, offs1);
    outb(PIC2_DATA, offs2);
    outb(PIC1_DATA, 4);
    outb(PIC2_DATA, 2);
    outb(PIC1_DATA, 1);
    outb(PIC2_DATA, 1);
    //Restore the interrupt masks
    outb(PIC1_DATA, m1);
    outb(PIC2_DATA, m2);
}

/*
 * Send an EOI signal to the PIC
 *   that corresponds to a certain IRQ
 * 
 * [EOI is not to be confused with yaoi]
 */
void pic_send_eoi(uint8_t irq){
    //If IRQ >= 8, PIC2 is involved
    if(irq >= 8)
        outb(PIC2_COMM, 0x20);
    //PIC1 is always involved as PIC2's
    //  IRQ line is routed through PIC1
    outb(PIC1_COMM, 0x20);
}

/*
 * Mask an IRQ
 */
void pic_mask_irq(uint8_t no){
    //Read interrupt mask
    uint8_t mask = inb(PIC1_DATA);
    //Set the bit that curresponds to the IRQ
    mask |= (1 << no);
    //Write the mask
    outb(PIC1_DATA, mask);
}

/*
 * Enable an IRQ
 */
void pic_en_irq(uint8_t no){
    //Read interrupt mask
    uint8_t mask = inb(PIC1_DATA);
    //Clear the bit that curresponds to the IRQ
    mask &= ~(1 << no);
    //Write the mask
    outb(PIC1_DATA, mask);
}