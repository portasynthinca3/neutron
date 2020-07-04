//Neutron Project
//AHCI driver

#include "./ahci.h"
#include "../stdlib.h"
#include "../krnl.h"
#include "./diskio.h"

//The list of AHCI MMIO bases
ahci_hba_mem_t* ahci_base[MAX_CONTROLLERS];
uint8_t ahci_cnt = 0;

/*
 * An AHCI controller was detected
 */
void ahci_init(ahci_hba_mem_t* base){
    ahci_base[ahci_cnt++] = base;

    krnl_write_msg(__FILE__, "a controller was detected");

    uint32_t pi = base->pi;
    for(int i = 0; i < 32; i++){
        //Check if there is a drive at this port
        if(((pi >> i) & 1) == 0)
            continue;
        //Determine and print its type
        char* t_str;
        ahci_dev_type_t type = ahci_dev_type(&base->ports[i]);
        if(type == AHCI_NONE)
            continue;
        switch(type){
            case AHCI_SATA:
                t_str = "SATA drive";
                {
                    //Mount the drive
                    char mount_path[32];
                    sprintf(mount_path, "/disk/sata%i", (ahci_cnt - 1) * 32 + i);
                    diskio_mount((diskio_dev_t){.bus_type = DISKIO_BUS_SATA,
                                                .device_no = ((ahci_cnt - 1) << 8) | i},
                                 mount_path);
                }
                break;
            case AHCI_SATAPI:
                t_str = "SATAPI drive";
                break;
            case AHCI_SEMB:
                t_str = "enclosure management bridge";
                break;
            case AHCI_PM:
                t_str = "port multiplier";
                break;
            default:
                t_str = "???";
                break;
        }
        krnl_write_msgf(__FILE__, "Device in port %i: %s", i, t_str);
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