//Neutron Project
//PS/2 driver

#include "./ps2.h"
#include "./ps2_kbd.h"
#include "./ps2_mouse.h"
#include "../../stdlib.h"
#include "../../gui/gui.h"

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
    ps2_wait_send();
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
    ps2_alloc_buf();
    while(inb(0x64) & 2); //Wait for input acceptability
    outb(0x64, 0xAE); //Issue command 0xAE (enable first PS/2 port)
    while(inb(0x64) & 2); //Wait for input acceptability
    outb(0x64, 0xA8); //Issue command 0xA8 (enable second PS/2 port)
    //Initialize keyboard and mouse
    ps2_mouse_init();
    ps2_kbd_init();
}

/*
 * Allocate buffers that are being used by the PS/2 system
 */
void ps2_alloc_buf(void){
    gfx_verbose_println("Allocating buffers for PS/2");
    //Allocate the keyboard buffer
    kbd_buffer = (unsigned char*)malloc(KEYBOARD_BUFFER_SIZE);
    //Allocate the mouse buffer
    ms_buffer = (unsigned char*)malloc(MOUSE_BUFFER_SIZE);
    //Reset head and tail of keyboard FIFO
    kbd_buffer_head = 0;
    kbd_buffer_tail = 0;
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
    //While data is available for reading
    while((p64d = inb(0x64)) & 1){
        //If bit 5 is set, it's a mouse data byte
        if(p64d & 0x20)
            fifo_pushb(ms_buffer, &ms_buffer_head, inb(0x60));
        else //Else, a keyboard one
            fifo_pushb(kbd_buffer, &kbd_buffer_head, inb(0x60));
    }
    //If at least three bytes are available for reading in the mouse buffer
    if(fifo_av(&ms_buffer_head, &ms_buffer_tail) >= 3){
        ps2_mouse_parse(ms_buffer, &ms_buffer_head, &ms_buffer_tail);
    }
    //If at least some data is available in the keyboard buffer
    if(fifo_av(&kbd_buffer_head, &kbd_buffer_tail)){

    }
}