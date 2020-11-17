#ifndef AHCI_H
#define AHCI_H

#include "../../stdlib.h"

//Settings

#define AHCI_MAX_CONTROLLERS     8
#define AHCI_MAX_PORTS           (AHCI_MAX_CONTROLLERS * 32)

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

typedef volatile struct {
    uint8_t  cfl:5, //command FIS length in dwords
               a:1, //is ATAPI
               w:1, //read/write
               p:1, //prefetchable

               r:1, //reset
               b:1, //BIST
               c:1, //clear busy if OK
              
           rsvd0:1,
             pmp:4; //port multiplier port

    uint16_t prdtl; //PRD table entry count
    uint32_t prdbc; //PRD bytes transferred
    uint64_t  ctba; //command table base address

    uint32_t rsvd1[4];
} __attribute__((packed)) ahci_cmd_hdr_t;

typedef volatile struct {
    uint64_t     dba; //data base address
    uint32_t   rsvd0;

    uint32_t  dbc:22; //byte count
    uint32_t rsvd1:9; //reserved
    uint32_t     i:1; //interrupt on completion
} __attribute__((packed)) ahci_prdt_entry_t;

typedef volatile struct {
    uint8_t cfis[64], //command FIS
            acmd[16], //ATAPI command
            rsvd[48];

    ahci_prdt_entry_t prdt_entries[1];
} __attribute__((packed)) ahci_cmd_tbl_t;

typedef volatile struct {
    uint8_t fis_type; //type (FIS_TYPE_REG_H2D)

    uint8_t   pmp:4, //port multiplier port
            rsvd0:3,
                c:1, //command/control
              cmd:8, //command register
            featl:8; //feature register (low)
    
    uint8_t lba0, //LBA byte 0
            lba1, //LBA byte 1
            lba2, //LBA byte 2
             dev; //device register
    
    uint8_t  lba3, //LBA byte 3
             lba4, //LBA byte 4
             lba5, //LBA byte 5
            feath; //feature register (high)
    
    uint8_t cntl, //count (low)
            cnth, //count (high)
             icc, //isochronous command completion
             ctl; //control register

    uint8_t rsvd1[4];
} __attribute__((packed)) ahci_fis_reg_h2d_t;

typedef struct {
    uint8_t host;
    uint8_t port;

    ahci_hba_port_t* ptr;
    ahci_cmd_hdr_t*  cmd_hdr;
    ahci_cmd_tbl_t*  cmd_tbl;
    uint8_t*         info;

    uint64_t max_lba;
} sata_dev_t;

//Enumerator definitions

typedef enum {
    AHCI_NONE, AHCI_SATAPI, AHCI_SEMB, AHCI_PM, AHCI_SATA
} ahci_dev_type_t;

typedef enum {
    AHCI_FIS_TYPE_REG_H2D   = 0x27,
    AHCI_FIS_TYPE_REG_D2H   = 0x34,
    AHCI_FIS_TYPE_DMA_ACT   = 0x39,
    AHCI_FIS_TYPE_DMA_SETUP = 0x41,
    AHCI_FIS_TYPE_DATA      = 0x46,
    AHCI_FIS_TYPE_BIST      = 0x58,
    AHCI_FIS_TYPE_PIO_SETUP = 0x5F,
    AHCI_FIS_TYPE_DEV_BITS  = 0xA1
} ahci_fis_type_t;

//Function prototypes

void ahci_init                (ahci_hba_mem_t* base);
ahci_dev_type_t ahci_dev_type (ahci_hba_port_t* port);

void ahci_start_cmd (ahci_hba_port_t* port);
void ahci_stop_cmd  (ahci_hba_port_t* port);

void ahci_read     (uint32_t dev, void* buf, size_t cnt, uint64_t lba);
void ahci_write    (uint32_t dev, void* buf, size_t cnt, uint64_t lba);
void ahci_identify (uint32_t dev, uint8_t info[512]);

sata_dev_t* ahci_get_drive (uint32_t dev);

#endif