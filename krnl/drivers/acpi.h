#ifndef ACPI_H
#define ACPI_H

#include "../stdlib.h"

//ACPI SDT header
typedef struct {
    char signature[4];
    uint32_t len;
    uint8_t rev;
    uint8_t checksum;
    char oem[6];
    char oem_table_id[8];
    uint32_t oem_rev;
    uint32_t creator_id;
    uint32_t creator_rev;
} __attribute__((packed)) acpi_sdt_hdr_t;

//ACPI RSDT table
typedef struct {
    acpi_sdt_hdr_t hdr;
    uint32_t ptrs[256];
} __attribute__((packed)) acpi_rsdt_t;

//ACPI RSDP structure
typedef struct {
    char signature[8];
    uint8_t checksum;
    char oem[6];
    uint8_t rev;
    uint32_t rsdt_ptr;
} __attribute__((packed)) acpi_rsdp_t;

//ACPI Generic Address Structure (GAS)
typedef struct {
    uint8_t addr_space;
    uint8_t bit_width;
    uint8_t bit_offset;
    uint8_t access_size;
    uint64_t addr;
} __attribute__((packed)) acpi_gas_t;

//ACPI FADT table
typedef struct {
    acpi_sdt_hdr_t hdr;
    uint32_t firmware_ctl;
    uint32_t dsdt;

    uint8_t reserved;

    uint8_t pref_pwr_mgmt_mode;
    uint16_t sci_intr;
    uint32_t smi_comm_port;
    uint8_t acpi_dis;
    uint8_t acpi_en;
    uint8_t s4bios_rq;
    uint8_t pstate_ctl;
    uint32_t pm1a_evt_blk;
    uint32_t pm1b_evt_blk;
    uint32_t pm1a_ctl_blk;
    uint32_t pm1b_ctl_blk;
    uint32_t pm2_ctl_blk;
    uint32_t pm_tim_blk;
    uint32_t gpe0_blk;
    uint32_t gpe1_blk;
    uint8_t pm1_evt_len;
    uint8_t pm1_ctl_len;
    uint8_t pm2_ctl_len;
    uint8_t pm_tim_len;
    uint8_t gpe0_len;
    uint8_t gpe1_len;
    uint8_t gpe1_bas;
    uint8_t c_state_ctl;
    uint16_t worst_c2_latency;
    uint16_t worst_c3_latency;
    uint16_t flush_size;
    uint16_t flush_stride;
    uint8_t duty_offset;
    uint8_t duty_width;
    uint8_t day_alarm;
    uint8_t mon_alarm;
    uint8_t century;

    uint16_t boot_arch_flags;

    uint8_t reserved_2;
    uint32_t flags;

    acpi_gas_t reset_reg;

    uint8_t reset_val;
    uint8_t reserved_3[3];

    uint64_t x_firm_ctl;
    uint64_t x_dsdt;

    acpi_gas_t x_pm1a_evt_blk;
    acpi_gas_t x_pm1b_evt_blk;
    acpi_gas_t x_pm1a_ctl_blk;
    acpi_gas_t x_pm1b_ctl_blk;
    acpi_gas_t x_pm2_ctl_blk;
    acpi_gas_t x_pm_tim_blk;
    acpi_gas_t x_gpe0_blk;
    acpi_gas_t x_gpe1_blk;
} __attribute__((packed)) acpi_fadt_t;

//Function prototypes

//Initialization
uint32_t acpi_init (void);
//Finding tables
uint8_t      acpi_sdt_checksum (acpi_sdt_hdr_t* rsdt);
acpi_rsdp_t* acpi_find_rsdp    (void);
void*        rsdt_find         (char* table);
//Power management
void acpi_shutdown (void);
void acpi_reboot   (void);

#endif