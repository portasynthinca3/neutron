#ifndef AHCI_H
#define AHCI_H

#include "../stdlib.h"

//Settings

#define MAX_CONTROLLERS             8

//Structure definitions

typedef volatile struct {
    uint64_t clb, //command list base
             fb;  //FIS base

    uint32_t is,    //interrupt status
             ie,    //interrupt enable
             cmd,   //command and status
             rsvd0,
             tfd,   //task file data
             sig,   //signature
             ssts,  //SATA status
             sctl,  //SATA control
             serr,  //SATA error
             sact,  //SATA active
             ci,    //command issue
             sntf,  //SATA notification
             fbs;   //FIS-based switch control
    
    uint32_t rsvd1[11];
    uint32_t vend[4];
} __attribute__((packed)) ahci_hba_port_t;

typedef volatile struct {
    uint32_t cap,     //host capability
             ghc,     //general host control
             is,      //interrupt status
             pi,      //port implemented
             vs,      //version
             ccc_ctl, //command completion coalescing control
             ccc_pts, //command completion coalescing ports
             em_loc,  //enclosure management location
             em_ctl,  //enclosure management control
             cap2,    //host capabilities extended
             bohc;    //BIOS/OS handoff
    
    uint8_t rsvd[0x74];
    uint8_t vend[0x60];

    ahci_hba_port_t ports[1];
} __attribute__((packed)) ahci_hba_mem_t;

//Enumerator definitions

typedef enum {
    AHCI_NONE = 0, AHCI_SATAPI, AHCI_SEMB, AHCI_PM, AHCI_SATA
} ahci_dev_type_t;

//Function prototypes

void ahci_init(ahci_hba_mem_t* base);
ahci_dev_type_t ahci_dev_type(ahci_hba_port_t* port);

#endif