//Neutron Project
//PS/2 driver

#include "./ps2.h"
#include "./ps2_kbd.h"
#include "./ps2_mouse.h"
#include "../../stdlib.h"
#include "../../gui/gui.h"
#include "../ata.h" //For delay only

//Keyboard buffer
uint8_t* kbd_buffer;
uint16_t kbd_buffer_head, kbd_buffer_tail;
//Mouse buffer
uint8_t* ms_buffer;
uint16_t ms_buffer_head, ms_buffer_tail;

/*
 * Read PS/2 controller's status register
 */
uint8_t ps2_cont_status(void){
    return inb(PS2_CONT_STATUS);
}

/*
 * Waits until the PS/2 controller is ready to accept data
 */
void ps2_wait_send(void){
    while(ps2_cont_status() & 2);
}

/*
 * Waits until the PS/2 controller is ready to send data
 */
void ps2_wait_recv(void){
    while(ps2_cont_status() & 1 == 0);
}

/*
 * Sends a command to the PS/2 controller
 */
void ps2_command(uint8_t c){
    ata_wait_100us(10);
    outb(PS2_CONT_COMM, c);
}

/*
 * Reads a byte from the PS/2 data port
 */
uint8_t ps2_read(void){
    ps2_wait_recv();
    return inb(PS2_CONT_DATA);
}

/*
 * Sends a byte to the PS/2 data port
 */
void ps2_send(uint8_t d){
    ps2_wait_send();
    return outb(PS2_CONT_DATA, d);
}

/*
 * Initializes the PS/2 system
 */
void ps2_init(void){
    //Flush the input buffer
    for(uint32_t i = 0; i < 1000; i++)
        inb(PS2_CONT_DATA);
    //Disable PS/2 ports
    ps2_command(0xAD);
    ps2_command(0xA7);
    //Flush the output buffer
    while(ps2_cont_status() & 1)
        ps2_read();
    //Read configuration byte
    ps2_command(0x20);
    uint8_t config_byte = ps2_read();
    //Disable interrupts and translation
    config_byte &= 0b10111100;
    //Write the modified configuration byte back
    ps2_command(0x60);
    ps2_send(config_byte);
    //Enable PS/2 ports
    ps2_command(0xAE);
    ps2_command(0xA8);
    //Initialize keyboard and mouse
    ps2_mouse_init();
    ps2_kbd_init();
    ps2_alloc_buf();
}

/*
 * Allocate buffers that are being used by the PS/2 system
 */
void ps2_alloc_buf(void){
    gfx_verbose_println("Allocating buffers for PS/2");
    //Allocate the keyboard buffer
    kbd_buffer = (uint8_t*)malloc(KEYBOARD_BUFFER_SIZE);
    //Reset head and tail of keyboard FIFO
    kbd_buffer_head = 0;
    kbd_buffer_tail = 0;
    //Allocate the mouse buffer
    ms_buffer = (uint8_t*)malloc(MOUSE_BUFFER_SIZE);
    //Reset head and tail of mouse FIFO
    ms_buffer_head = 0;
    ms_buffer_tail = 0;
}

/*
 * Polls the PS/2 controller
 */
void ps2_poll(void){
    //Variable holding the I/O port 64h data
    uint8_t p64d;
    uint32_t timeout = 100;
    //While the time didn't out
    while(timeout--){
        //While there's data ready to be read
        if((p64d = inb(0x64)) & 1){
            //If bit 5 is set, it's a mouse data byte
            if(p64d & 0x20)
                fifo_pushb(ms_buffer, &ms_buffer_head, inb(PS2_CONT_DATA));
            else //Else, a keyboard one
                fifo_pushb(kbd_buffer, &kbd_buffer_head, inb(PS2_CONT_DATA));
        }
        //Delay for 1ms
        ata_wait_100us(10);
    }
    //While at least three bytes are available for reading in the mouse buffer
    while(fifo_av(&ms_buffer_head, &ms_buffer_tail) >= 3){
        ps2_mouse_parse(ms_buffer, &ms_buffer_head, &ms_buffer_tail);
    }
    //While at least some data is available in the keyboard buffer
    while(fifo_av(&kbd_buffer_head, &kbd_buffer_tail)){
        ps2_kbd_parse(kbd_buffer, &kbd_buffer_head, &kbd_buffer_tail);
    }
}