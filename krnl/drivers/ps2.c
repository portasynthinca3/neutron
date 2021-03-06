//Neutron project
//PS/2 driver

#include "./ps2.h"
#include "../krnl.h"
#include "../stdlib.h"
#include "./acpi.h"
#include "./apic.h"

//Buffers for received data
uint8_t ps21_buf[PS2_BUF_SIZE];
uint64_t ps21_buf_wr = 0;
uint64_t ps21_buf_rd = 0;
uint8_t ps22_buf[PS2_BUF_SIZE];
uint64_t ps22_buf_wr = 0;
uint64_t ps22_buf_rd = 0;

/*
 * Initializes the PS/2 controller
 */
void ps2_init(void){
    //Disable both PS/2 devices
    outb(PS2_COMMAND_REG, 0xAD);
    outb(PS2_COMMAND_REG, 0xA7);
    //Flush the output buffer
    for(int i = 0; i < 256; i++)
        inb(PS2_DATA_PORT);
    //Disable IRQs and translation
    outb(PS2_COMMAND_REG, 0x20);
    uint8_t cfg = ps2_read_byte();
    cfg &= 3;
    cfg &= 1 << 6;
    outb(PS2_COMMAND_REG, 0x60);
    ps2_write_byte(cfg);
    //Perform controller self-test
    outb(PS2_COMMAND_REG, 0xAA);
    uint8_t test_res = ps2_read_byte();
    if(test_res != 0x55){
        krnl_write_msg(__FILE__, __LINE__, "cotroller self-test failed");
        return;
    }
    //Restore the configuration byte in case it was reset by the self-test
    outb(PS2_COMMAND_REG, 0x60);
    ps2_write_byte(cfg);
    //Test PS/2 ports
    outb(PS2_COMMAND_REG, 0xAB);
    test_res = ps2_read_byte();
    if(test_res != 0)
        krnl_write_msgf(__FILE__, __LINE__, "interface 1 test failed (code %i)", test_res);
    outb(PS2_COMMAND_REG, 0xA9);
    test_res = ps2_read_byte();
    if(test_res != 0)
        krnl_write_msgf(__FILE__, __LINE__, "interface 2 test failed (code %i)", test_res);
    //Enable device interfaces
    outb(PS2_COMMAND_REG, 0xAE);
    outb(PS2_COMMAND_REG, 0xA8);
    //Map IRQs to CPU vectors
    ioapic_map_irq(0, 1,  33); //IRQ1  -> vector #33
    ioapic_map_irq(0, 12, 34); //IRQ12 -> vector #34
    //Reset and enable devices
    ps21_write(0xFF);
    ps21_write(0xF4);
    ps22_write(0xFF);
    ps22_write(0xF4);
    //It might me a good idea to flush all buffers once again
    for(int i = 0; i < 256; i++)
        inb(PS2_DATA_PORT);
    ps21_flush();
    ps22_flush();
    //Enable IRQs
    cfg |= 3;
    outb(PS2_COMMAND_REG, 0x60);
    ps2_write_byte(cfg);

    krnl_write_msgf(__FILE__, __LINE__, "initialized", test_res);
}

/*
 * Reads a byte from the PS/2 controller
 */
uint8_t ps2_read_byte(void){
    //Wait for bit 0 of the status register to be set
    while((inb(PS2_STATUS_REG) & 1) == 0);
    //Read data
    return inb(PS2_DATA_PORT);
}

/*
 * Writes a byte to the PS/2 controller
 */
void ps2_write_byte(uint8_t val){
    //Wait for bit 1 of the status register to be cleared
    while(inb(PS2_STATUS_REG) & 2);
    //Write data
    outb(PS2_DATA_PORT, val);
}

/*
 * Handles PS/2 port 1 interrupts
 */
void ps21_intr(void){
    //Read data byte
    uint8_t data = ps2_read_byte();
    //Write it to the ring buffer
    ps21_buf[ps21_buf_wr] = data;
    ps21_buf_wr = (ps21_buf_wr + 1) % PS2_BUF_SIZE;
    //Check for overflows
    if(ps21_buf_wr == ps21_buf_rd)
        krnl_write_msgf(__FILE__, __LINE__, "device 1 buffer overflow");
}

/*
 * Handles PS/2 port 2 interrupts
 */
void ps22_intr(void){
    //Read data byte
    uint8_t data = ps2_read_byte();
    //Write it to the ring buffer
    ps22_buf[ps22_buf_wr] = data;
    ps22_buf_wr = (ps22_buf_wr + 1) % PS2_BUF_SIZE;
    //Check for overflows
    if(ps22_buf_wr == ps22_buf_rd)
        krnl_write_msgf(__FILE__, __LINE__, "device 2 buffer overflow");
}

/*
 * Reads a byte from the PS/2 device 1 buffer
 */
int ps21_read(void){
    //Check if there are bytes waiting
    if(ps21_buf_wr == ps21_buf_rd)
        return -1;
    uint8_t data = ps21_buf[ps21_buf_rd];
    ps21_buf_rd = (ps21_buf_rd + 1) % PS2_BUF_SIZE;
    return data;
}

/*
 * Reads a byte from the PS/2 device 2 buffer
 */
int ps22_read(void){
    //Check if there are bytes waiting
    if(ps22_buf_wr == ps22_buf_rd)
        return -1;
    uint8_t data = ps22_buf[ps22_buf_rd];
    ps22_buf_rd = (ps22_buf_rd + 1) % PS2_BUF_SIZE;
    return data;
}

/*
 * Writes a byte to the PS/2 device 1
 */
void ps21_write(uint8_t val){
    ps2_write_byte(val);
}

/*
 * Writes a byte to the PS/2 device 2
 */
void ps22_write(uint8_t val){
    outb(PS2_COMMAND_REG, 0xD4);
    ps2_write_byte(val);
}

/*
 * Discard all data in the first PS/2 device's buffer
 */
void ps21_flush(void){
    ps21_buf_rd = ps21_buf_wr = 0;
}

/*
 * Discard all data in the second PS/2 device's buffer
 */
void ps22_flush(void){
    ps22_buf_rd = ps22_buf_wr = 0;
}