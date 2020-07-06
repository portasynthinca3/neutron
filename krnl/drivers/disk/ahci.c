//Neutron Project
//AHCI driver

#include "./ahci.h"
#include "../../stdlib.h"
#include "../../krnl.h"
#include "./diskio.h"
#include "./part.h"
#include "../../vmem/vmem.h"

//The list of AHCI MMIO bases
ahci_hba_mem_t* ahci_base[AHCI_MAX_CONTROLLERS];
uint8_t ahci_cnt = 0;
//The list of SATA devices
sata_dev_t sata_devs[AHCI_MAX_PORTS];
uint16_t sata_cnt = 0;

/*
 * An AHCI controller was detected
 */
void ahci_init(ahci_hba_mem_t* base){
    krnl_write_msg(__FILE__, __LINE__, "a controller was detected");
    ahci_base[ahci_cnt++] = base;
    //Make the controller range uncacheable
    vmem_pat_set_range(vmem_get_cr3(), (void*)base, (void*)(base + 4096), 0);

    uint32_t pi = base->pi;
    for(int i = 0; i < 32; i++){
        //Check if there is a drive at this port
        if(((pi >> i) & 1) == 0)
            continue;
        //Determine and its type, register it and mount its partitions if supported
        ahci_hba_port_t* port = &base->ports[i];
        ahci_dev_type_t type = ahci_dev_type(port);
        if(type == AHCI_NONE)
            continue;
        switch(type){
            case AHCI_SATA: {
                krnl_write_msgf(__FILE__, __LINE__, "device in port %i: SATA drive", i);
                //Stop command engine
                ahci_stop_cmd(port);

                //Allocate memory for drive control structures
                void* cl = amalloc(32, 1024);
                memset(cl, 0, 32);
                port->clb = (uint64_t)vmem_virt_to_phys(vmem_get_cr3(), cl);

                ahci_cmd_hdr_t* cmd_hdr = (ahci_cmd_hdr_t*)cl;
                cmd_hdr->prdtl = 1;
                void* ctb = amalloc(256, 256);
                memset(ctb, 0, 256);
                cmd_hdr->ctba = (uint64_t)vmem_virt_to_phys(vmem_get_cr3(), ctb);

                void* fis = amalloc(256, 256);
                memset(fis, 0, 256);
                port->fb = (uint64_t)vmem_virt_to_phys(vmem_get_cr3(), fis);

                //Make the port range uncacheable
                vmem_pat_set_range(vmem_get_cr3(), (void*)port, (void*)(port + 4096), 0);

                //Start command engine
                ahci_start_cmd(port);

                //Register the device
                sata_devs[sata_cnt++] = (sata_dev_t){
                    .host = ahci_cnt - 1,
                    .port = i,
                    .ptr = port,
                    .cmd_hdr = cmd_hdr,
                    .cmd_tbl = ctb,
                    .info = (uint8_t*)amalloc(512, 2)
                };

                //Read drive info
                sata_dev_t* dev = &sata_devs[sata_cnt - 1];
                ahci_identify(sata_cnt - 1, dev->info);
                //Calculate max LBA
                if(dev->info[138] & 4)
                    dev->max_lba = *(uint64_t*)&dev->info[460];
                else
                    dev->max_lba = *(uint32_t*)&dev->info[120];
                krnl_write_msgf(__FILE__, __LINE__, "max LBA: 0x%x", dev->max_lba);

                //Load partitions on this drive
                char disk_path[32];
                sprintf(disk_path, "/disk/sata%i", sata_cnt - 1);
                parts_load(disk_path);

                diskio_mount((diskio_dev_t){.bus_type = DISKIO_BUS_PART, .device_path = "/part/0"}, "/");
            } break;
            case AHCI_PM:
                krnl_write_msgf(__FILE__, __LINE__, "device in port %i: port multiplier (unsupported)", i);
                break;
            case AHCI_SATAPI:
                krnl_write_msgf(__FILE__, __LINE__, "device in port %i: SATAPI drive (unsupported)", i);
                break;
            case AHCI_SEMB:
                krnl_write_msgf(__FILE__, __LINE__, "device in port %i: enclosure manager (unsupported)", i);
                break;
            default:
                krnl_write_msgf(__FILE__, __LINE__, "device in port %i: unknown (unsupported)", i);
                break;
        }
    }
}

/*
 * Returns the type of the device connected to a port
 */
ahci_dev_type_t ahci_dev_type(ahci_hba_port_t* port){
    uint32_t status = port->ssts;

    uint8_t ipm = (status >> 8) & 0xF;
    uint8_t det =  status       & 0xF;

    //Check if the drive is present
    if(det != 3)
        return AHCI_NONE;
    if(ipm != 1)
        return AHCI_NONE;

    //Check the type
    switch(port->sig){
        case 0x00000101:
            return AHCI_SATA;
        case 0xEB140101:
            return AHCI_SATAPI;
        case 0x96690101:
            return AHCI_PM;
        case 0xC33C0101:
            return AHCI_SEMB;
        default:
            return AHCI_NONE;
    }
}

/*
 * Starts command engine for a port
 */
void ahci_start_cmd(ahci_hba_port_t* port){
    while((port->cmd & (1 << 15)));

    port->cmd |= (1 << 0) | (1 << 4);
}

/*
 * Stops command engine for a port
 */
void ahci_stop_cmd(ahci_hba_port_t* port){
    port->cmd &= ~((1 << 0) | (1 << 4));

    while((port->cmd & (1 << 14)) ||
          (port->cmd & (1 << 15)));
}

/*
 * Reads data from a drive
 */
void ahci_read(uint32_t dev, void* buf, size_t cnt, uint64_t lba){
    //Get port pointer
    ahci_hba_port_t* port = sata_devs[dev].ptr;

    //Clear interrupt flags
    port->is = 0xFFFF;

    //Setup the command header
    ahci_cmd_hdr_t* cmd_hdr = sata_devs[dev].cmd_hdr;
    cmd_hdr->cfl   = sizeof(ahci_fis_reg_h2d_t) / sizeof(uint32_t);
    cmd_hdr->w     = 0;
    cmd_hdr->c     = 1;
    cmd_hdr->p     = 1;
    cmd_hdr->prdtl = 1; //only one PRDT entry

    //Setup the command table
    ahci_cmd_tbl_t* cmd_tbl = sata_devs[dev].cmd_tbl;
    memset((void*)cmd_tbl, 0, sizeof(*cmd_tbl));

    //Setup the PRDT
    memset((void*)&cmd_tbl->prdt_entries[0], 0, sizeof(cmd_tbl->prdt_entries[0]));
    cmd_tbl->prdt_entries[0].dba = (uint64_t)vmem_virt_to_phys(vmem_get_cr3(), buf);
    cmd_tbl->prdt_entries[0].dbc = (cnt * 512) - 1;
    cmd_tbl->prdt_entries[0].i   = 0;

    //Setup the command FIS
    ahci_fis_reg_h2d_t* cmd_fis = (ahci_fis_reg_h2d_t*)&cmd_tbl->cfis;
    memset((void*)cmd_fis, 0, sizeof(*cmd_fis));
    cmd_fis->fis_type = AHCI_FIS_TYPE_REG_H2D;
    cmd_fis->c        = 1;
    cmd_fis->cmd      = 0x25; //DMA read

    cmd_fis->lba0 = (uint8_t)(lba >>  0);
    cmd_fis->lba1 = (uint8_t)(lba >>  8);
    cmd_fis->lba2 = (uint8_t)(lba >> 16);
    cmd_fis->dev  = 1 << 6;
    cmd_fis->lba3 = (uint8_t)(lba >> 24);
    cmd_fis->lba4 = (uint8_t)(lba >> 32);
    cmd_fis->lba5 = (uint8_t)(lba >> 40);

    cmd_fis->cntl = (uint8_t)(cnt >> 0);
    cmd_fis->cnth = (uint8_t)(cnt >> 8);

    //Issue the command
    port->ci = 1;
    //Wait for the command to complete
    while(1){
        if((port->ci & 1) == 0)
            break;
        if(port->is & (1 << 30))
            krnl_write_msgf(__FILE__, __LINE__, "drive read error");
    }
}

/*
 * Writes data to a drive
 */
void ahci_write(uint32_t dev, void* buf, size_t cnt, uint64_t lba){
    //Get port pointer
    ahci_hba_port_t* port = sata_devs[dev].ptr;

    //Clear interrupt flags
    port->is = 0xFFFF;

    //Setup the command header
    ahci_cmd_hdr_t* cmd_hdr = sata_devs[dev].cmd_hdr;
    cmd_hdr->cfl   = sizeof(ahci_fis_reg_h2d_t) / sizeof(uint32_t);
    cmd_hdr->w     = 1;
    cmd_hdr->c     = 1;
    cmd_hdr->p     = 1;
    cmd_hdr->prdtl = 1; //only one PRDT entry

    //Setup the command table
    ahci_cmd_tbl_t* cmd_tbl = sata_devs[dev].cmd_tbl;
    memset((void*)cmd_tbl, 0, sizeof(*cmd_tbl));

    //Setup the PRDT
    memset((void*)&cmd_tbl->prdt_entries[0], 0, sizeof(cmd_tbl->prdt_entries[0]));
    cmd_tbl->prdt_entries[0].dba = (uint64_t)vmem_virt_to_phys(vmem_get_cr3(), buf);
    cmd_tbl->prdt_entries[0].dbc = (cnt * 512) - 1;
    cmd_tbl->prdt_entries[0].i   = 0;

    //Setup the command FIS
    ahci_fis_reg_h2d_t* cmd_fis = (ahci_fis_reg_h2d_t*)&cmd_tbl->cfis;
    memset((void*)cmd_fis, 0, sizeof(*cmd_fis));
    cmd_fis->fis_type = AHCI_FIS_TYPE_REG_H2D;
    cmd_fis->c        = 1;
    cmd_fis->cmd      = 0x35; //DMA read

    cmd_fis->lba0 = (uint8_t)(lba >>  0);
    cmd_fis->lba1 = (uint8_t)(lba >>  8);
    cmd_fis->lba2 = (uint8_t)(lba >> 16);
    cmd_fis->dev  = 1 << 6;
    cmd_fis->lba3 = (uint8_t)(lba >> 24);
    cmd_fis->lba4 = (uint8_t)(lba >> 32);
    cmd_fis->lba5 = (uint8_t)(lba >> 40);

    cmd_fis->cntl = (uint8_t)(cnt >> 0);
    cmd_fis->cnth = (uint8_t)(cnt >> 8);

    //Issue the command
    port->ci = 1;
    //Wait for the command to complete
    while(1){
        if((port->ci & 1) == 0)
            break;
        if(port->is & (1 << 30))
            krnl_write_msgf(__FILE__, __LINE__, "drive write error");
    }
}

/*
 * Reads drive information
 */
void ahci_identify(uint32_t dev, uint8_t info[512]){
    //Get port pointer
    ahci_hba_port_t* port = sata_devs[dev].ptr;

    //Clear interrupt flags
    port->is = 0xFFFF;

    //Setup the command header
    ahci_cmd_hdr_t* cmd_hdr = sata_devs[dev].cmd_hdr;
    cmd_hdr->cfl   = sizeof(ahci_fis_reg_h2d_t) / sizeof(uint32_t);
    cmd_hdr->w     = 0;
    cmd_hdr->c     = 1;
    cmd_hdr->p     = 0;
    cmd_hdr->prdtl = 1; //only one PRDT entry

    //Setup the command table
    ahci_cmd_tbl_t* cmd_tbl = sata_devs[dev].cmd_tbl;
    memset((void*)cmd_tbl, 0, sizeof(*cmd_tbl));

    //Setup the PRDT
    memset((void*)&cmd_tbl->prdt_entries[0], 0, sizeof(cmd_tbl->prdt_entries[0]));
    cmd_tbl->prdt_entries[0].dba = (uint64_t)vmem_virt_to_phys(vmem_get_cr3(), info);
    cmd_tbl->prdt_entries[0].dbc = 512;
    cmd_tbl->prdt_entries[0].i   = 0;

    //Setup the command FIS
    ahci_fis_reg_h2d_t* cmd_fis = (ahci_fis_reg_h2d_t*)&cmd_tbl->cfis;
    memset((void*)cmd_fis, 0, sizeof(*cmd_fis));
    cmd_fis->fis_type = AHCI_FIS_TYPE_REG_H2D;
    cmd_fis->c        = 1;
    cmd_fis->dev      = 0;
    cmd_fis->cmd      = 0xEC; //Identify

    //Wait for the drive to perform previous commands
    while((port->tfd & 0x88));
    //Issue the command
    port->ci = 1;
    //Wait for the command to complete
    while(1){
        if((port->ci & 1) == 0)
            break;
        if(port->is & (1 << 30))
            krnl_write_msgf(__FILE__, __LINE__, "drive identification error");
    }
}

/*
 * Returns the pointer to the drive descriptor structure
 */
sata_dev_t* ahci_get_drive(uint32_t dev){
    return &sata_devs[dev];
}