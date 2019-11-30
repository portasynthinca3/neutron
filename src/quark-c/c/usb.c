//Neutron project
//USB driver

#include "../h/usb.h"
#include "../h/pci.h"

//Array of EHCI controller parameters
ehci_cont_t ehci_conts[4];
//EHCI controller count
unsigned char ehci_cont_count;

/*
 * Perform some value initialization
 */
void ehci_init(){
    //The starting EHCI controller count is 0
    ehci_cont_count = 0;
}

/*
 * Add an EHCI controller to the list
 */
unsigned char ehci_add_cont(unsigned char b, unsigned char d){
    //Add the controller to the list
    ehci_conts[ehci_cont_count].pci_addr = (b << 8) | d;
    //Read PCI BAR0 register of the controller
    short bar0h = pci_read_config_16(b, d, 0, 0x10);
    short bar0l = pci_read_config_16(b, d, 0, 0x12);
    int bar0 = (bar0h << 16) | bar0l;
    ehci_conts[ehci_cont_count].capreg_addr = (void*)bar0;
    //Decode some register values
    int hcsparams = ehci_read_capreg(ehci_cont_count, EHCI_CAPREG_HCSPARAMS);
    ehci_conts[ehci_cont_count].port_cnt = hcsparams & 0x0000000F;
    ehci_conts[ehci_cont_count].dbg_port = (hcsparams & ((1 << 23) | (1 << 22) | (1 << 21) | (1 << 20))) >> 20;
    short hci_ver_bcd = ehci_read_capreg(ehci_cont_count, EHCI_CAPREG_HCIVERSION) & 0x0000FFFF;
    ehci_conts[ehci_cont_count].hci_ver[0] = ((hci_ver_bcd & 0xF000) >> 24) + '0';
    ehci_conts[ehci_cont_count].hci_ver[1] = ((hci_ver_bcd & 0x0F00) >> 16) + '0';
    ehci_conts[ehci_cont_count].hci_ver[2] = '.';
    ehci_conts[ehci_cont_count].hci_ver[3] = ((hci_ver_bcd & 0x00F0) >> 8) + '0';
    ehci_conts[ehci_cont_count].hci_ver[4] = (hci_ver_bcd & 0x000F) + '0';
    ehci_conts[ehci_cont_count].hci_ver[5] = 0;
    //Increment the controller count and return the current one
    return ehci_cont_count++;
}

/*
 * Read EHCI controller capability register
 */
int ehci_read_capreg(unsigned char ehci_no, unsigned char reg){
    //Get the address
    int* ptr = (int*)(ehci_conts[ehci_no].capreg_addr + reg);
    //Read and return it
    return *ptr;
}

/*
 * Read EHCI controller operation register
 */
int ehci_read_opreg(unsigned char ehci_no, unsigned char reg){
    //Read capability register length
    int caplen = ehci_read_capreg(ehci_no, EHCI_CAPREG_CAPLEN);
    //Get the address
    int* ptr = (int*)(ehci_conts[ehci_no].capreg_addr + caplen + reg);
    //Read and return it
    return *ptr;
}

/*
 * Write a doubleword to EHCI controller operation register
 */
void ehci_write_opreg_dw(unsigned char ehci_no, unsigned char reg, int val){
    //Read capability register length
    int caplen = ehci_read_capreg(ehci_no, EHCI_CAPREG_CAPLEN);
    //Get the address
    int* ptr = (int*)(ehci_conts[ehci_no].capreg_addr + caplen + reg);
    //Write the value
    *ptr = val;
}

/*
 * Write a word to EHCI controller operation register
 */
void ehci_write_opreg_w(unsigned char ehci_no, unsigned char reg, short val){
    //Read capability register length
    int caplen = ehci_read_capreg(ehci_no, EHCI_CAPREG_CAPLEN);
    //Get the address
    short* ptr = (short*)(ehci_conts[ehci_no].capreg_addr + caplen + reg);
    //Write the value
    *ptr = val;
}

/*
 * Get information about an EHCI controller
 */
ehci_cont_t ehci_get_cont(unsigned char ehci_no){
    return ehci_conts[ehci_no];
}
