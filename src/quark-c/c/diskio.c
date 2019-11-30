//Neutron Project
//Quark Disk I/O

#include "../h/diskio.h"

/*
 * Read the byte describing floppy drives from CMOS
 */
unsigned char diskio_get_floppy_drives(void){
    unsigned char val;
    //BLACK MAGIC
    __asm__ volatile("movb $0x10, %%al; outb %%al, $0x70; inb $0x71, %%al" : "=a" (val));
    return val;
}