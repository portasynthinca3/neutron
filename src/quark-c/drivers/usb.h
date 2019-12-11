#ifndef USB_H
#define USB_H

//EHCI controller structure

typedef struct {
    //Interface version
    char hci_ver[6];
    //Capability Register address in RAM
    void* capreg_addr;
    //PCI bus address
    short pci_addr;
    //Port count
    unsigned char port_cnt;
    //Debug port number
    unsigned char dbg_port;
} ehci_cont_t;

//EHCI capability registers

#define EHCI_CAPREG_CAPLEN                  0x00
#define EHCI_CAPREG_HCIVERSION              0x02
#define EHCI_CAPREG_HCSPARAMS               0x04
#define EHCI_CAPREG_HCCPARAMS               0x08
#define EHCI_CAPREG_HCSP_ROUTE              0x0C

//EHCI operation registers

#define EHCI_OPREG_USBCMD                   0x00
#define EHCI_OPREG_USBSTS                   0x04
#define EHCI_OPREG_USBINTR                  0x08
#define EHCI_OPREG_FRINDEX                  0x0C
#define EHCI_OPREG_CTRLDSSEGMENT            0x10
#define EHCI_OPREG_PERIODICLISTBASE         0x14
#define EHCI_OPREG_ASYNCLISTADDR            0x18
#define EHCI_OPREG_CONFIGFLAG               0x40
#define EHCI_OPREG_PORTSC                   0x40

//Basic operations

unsigned char ehci_add_cont(unsigned char b, unsigned char d);
int ehci_read_capreg(unsigned char ehci_no, unsigned char reg);
int ehci_read_opreg(unsigned char ehci_no, unsigned char reg);
void ehci_write_opreg_dw(unsigned char ehci_no, unsigned char reg, int val);
void ehci_write_opreg_w(unsigned char ehci_no, unsigned char reg, short val);
ehci_cont_t ehci_get_cont(unsigned char ehci_no);

#endif