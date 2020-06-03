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
    //Check if there's a PS/2 controller at all
    //Find the FADT table
    acpi_fadt_t* fadt = rsdt_find("FACP");
    //Check its presence and boot architecture flag 1
    if(fadt != NULL && fadt->boot_arch_flags & 2 == 0){
        krnl_write_msg(__FILE__, "no controller found");
        return;
    }
    krnl_write_msg(__FILE__, "controller found");
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
        krnl_write_msg(__FILE__, "cotroller self-test failed");
        return;
    }
    //Restore the configuration byte in case it was reset by the self-test
    outb(PS2_COMMAND_REG, 0x60);
    ps2_write_byte(cfg);
    //Test PS/2 ports
    outb(PS2_COMMAND_REG, 0xAB);
    test_res = ps2_read_byte();
    if(test_res != 0)
        krnl_write_msgf(__FILE__, "interface 1 test failed (code %i)", test_res);
    outb(PS2_COMMAND_REG, 0xA9);
    test_res = ps2_read_byte();
    if(test_res != 0)
        krnl_write_msgf(__FILE__, "interface 2 test failed (code %i)", test_res);
    //Enable device interfaces
    outb(PS2_COMMAND_REG, 0xAE);
    outb(PS2_COMMAND_REG, 0xA8);
    //Map IRQs to CPU vectors
    ioapic_map_irq(0, 1,  33); //IRQ1  -> vector #33
    ioapic_map_irq(0, 12, 34); //IRQ12 -> vector #34
    //Enable IRQs
    cfg |= 3;
    outb(PS2_COMMAND_REG, 0x60);
    ps2_write_byte(cfg);
    //Reset and enable devices
    ps2_write_byte(0xFF);
    ps2_write_byte(0xF4);
    outb(PS2_COMMAND_REG, 0xD4);
    ps2_write_byte(0xFF);
    outb(PS2_COMMAND_REG, 0xD4);
    ps2_write_byte(0xF4);

    krnl_write_msgf(__FILE__, "initialized", test_res);
}

/*
 * Reads a byte from the PS/2 controller
 */
uint8_t ps2_read_byte(void){
    //Wait for bit 0 of the status register to be set
    while(inb(PS2_STATUS_REG) & 1 == 0);
    //Read data
    return inb(PS2_DATA_PORT);
}

/*
 * Writes a byte to the PS/2 controller
 */
void ps2_write_byte(uint8_t val){
    //Wait for bit 1 of the status register to be cleared
    while(inb(PS2_STATUS_REG) & 2 == 0);
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
        krnl_write_msgf(__FILE__, "oops, device 1 buffer overflow");
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
        krnl_write_msgf(__FILE__, "oops, device 2 buffer overflow");
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