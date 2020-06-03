//Neutron Project
//Local APIC driver

#include "./apic.h"
#include "../stdlib.h"
#include "../krnl.h"
#include "./acpi.h"

//Local APIC base address
uint64_t lapic_base;
//MADT table pointer
uint8_t* madt_ptr;
//All registered I/O APICs
ioapic_t ioapics[IO_APIC_MAX_CNT];
uint8_t next_ioapic = 0;
//IRQ to GSI map
uint32_t gsi_map[256];

/*
 * Initializes the Advanced Programmable Interrupt Controller
 */
void apic_init(void){
    //LAPIC

    //Disable interrupts
    __asm__ volatile("cli");
    //Set LAPIC base
    lapic_base = 0xFFFFFFFFFFFFE000ULL;
    krnl_write_msgf(__FILE__, "LAPIC base: 0x%x", lapic_base);
    //Set task and processor priority to 0
    lapic_reg_wr(LAPIC_REG_TPR, 0);
    lapic_reg_wr(LAPIC_REG_PPR, 0);
    //Mask all interrupts in the LVT except for LI0 and LI1
    lapic_reg_wr(LAPIC_REG_LVT_ERR, 0x10022);
    lapic_reg_wr(LAPIC_REG_LVT_TIM, 0x10021);
    lapic_reg_wr(LAPIC_REG_LVT_CMCI, 0x10021);
    lapic_reg_wr(LAPIC_REG_LVT_THERM, 0x10021);
    lapic_reg_wr(LAPIC_REG_LVT_LINT0, 0x8721);
    lapic_reg_wr(LAPIC_REG_LVT_LINT1, 0x421);
    lapic_reg_wr(LAPIC_REG_LVT_PERFMON, 0x10021);
    //Write the spurious interrupt register bit 8 to receive interrupts AND spurious interrupt ID 0xFF
    lapic_reg_wr(LAPIC_REG_SPUR_INTR, lapic_reg_rd(LAPIC_REG_SPUR_INTR) | (1 << 8) | 0xFF);
    //Set logical destination
    lapic_reg_wr(LAPIC_REG_DEST_FMT, 0x0FFFFFFF);
    lapic_reg_wr(LAPIC_REG_LOGICAL_DEST, (lapic_reg_rd(LAPIC_REG_LOGICAL_DEST) & 0x00FFFFFF) | 1);
    //Send EOI
    lapic_eoi();
    //Clear error
    lapic_reg_wr(LAPIC_REG_ERR_ST, 0);
    krnl_write_msgf(__FILE__, "initialized LAPIC");

    //I/O APIC

    //Fill the default IRQ2GSI mapping
    for(int i = 0; i < 256; i++)
        gsi_map[i] = i;
    //Find the MADT table
    madt_ptr = (uint8_t*)rsdt_find("APIC");
    if(madt_ptr == NULL){
        krnl_write_msgf(__FILE__, "MADT not found");
        return;
    }
    krnl_write_msgf(__FILE__, "MADT is at 0x%x", madt_ptr);
    //Go through the records
    uint32_t cur_offs = 0x2C;
    while(cur_offs < *(uint32_t*)(madt_ptr + 4) - 32){
        madt_record_t* record = (madt_record_t*)(madt_ptr + cur_offs);
        cur_offs += record->len;
        //Parse the record
        if(record->type == 1){
            ioapic_t* ioapic = &ioapics[next_ioapic++];
            ioapic->valid     = 1;
            ioapic->id        = record->data[0];
            ioapic->mmio_base = *(uint32_t*)&record->data[2];
            ioapic->gsi_base  = *(uint32_t*)&record->data[6];
            krnl_write_msgf(__FILE__, "found I/O APIC %i at 0x%x with gsi_base=%i",
                ioapic->id, ioapic->mmio_base, ioapic->gsi_base);
        } else if(record->type == 2){
            uint8_t  irq   = record->data[1];
            uint32_t gsi   = *(uint32_t*)&record->data[2];
            uint16_t flags = *(uint32_t*)&record->data[6];
            gsi_map[irq] = gsi | (flags << 8);
            krnl_write_msgf(__FILE__, "override: IRQ%i -> GSI%i",
                irq, gsi);
        }
    }

    krnl_write_msg(__FILE__, "I/O APIC initialized");
}

/*
 * Reads LAPIC register
 */
uint32_t lapic_reg_rd(uint32_t reg){
    return *(uint32_t*)(lapic_base + (uint64_t)reg);
}

/*
 * Writes LAPIC register
 */
void lapic_reg_wr(uint32_t reg, uint32_t val){
    *(uint32_t*)(lapic_base + (uint64_t)reg) = val;
}

/*
 * Returns the ID of the LAPIC
 */
uint32_t lapic_get_id(void){
    return lapic_reg_rd(LAPIC_REG_ID);
}

/*
 * Send an EOI signal
 */
void lapic_eoi(void){
    lapic_reg_wr(LAPIC_REG_EOI, 0);
}

/*
 * Reads I/O APIC register
 */
uint32_t ioapic_reg_rd(uint32_t id, uint32_t reg){
    //Get the base address of the APIC
    uint64_t base = 0;
    for(int i = 0; i < IO_APIC_MAX_CNT; i++)
        if(ioapics[i].valid && ioapics[i].id == id)
            base = ioapics[i].mmio_base;
    //Set IOREGSEL to the register number
    *(volatile uint32_t*)base = reg;
    //Read IOWIN
    return *(volatile uint32_t*)(base + 0x10);
}

/*
 * Writes I/O APIC register
 */
void ioapic_reg_wr(uint32_t id, uint32_t reg, uint32_t val){
    //Get the base address of the APIC
    uint64_t base = 0;
    for(int i = 0; i < IO_APIC_MAX_CNT; i++)
        if(ioapics[i].valid && ioapics[i].id == id)
            base = ioapics[i].mmio_base;
    //Set IOREGSEL to the register number
    *(volatile uint32_t*)base = reg;
    //Write to IOWIN
    *(volatile uint32_t*)(base + 0x10) = val;
}

/*
 * Maps an IRQ to the CPU vector
 */
void ioapic_map_irq(uint32_t id, uint8_t irq, uint8_t vect){
    //Get interrupt flags and override the IRQ value
    uint32_t flags = gsi_map[irq] >> 8;
    irq = (uint8_t)gsi_map[irq];
    uint8_t low_trig = (flags >> 1) & 1;
    uint8_t lev_trig = (flags >> 3) & 1;
    //Calculate I/O APIC redirection table entry register num
    uint32_t redir_entry_reg = 0x10 + (irq * 2);
    //Get this CPU's LAPIC's APIC ID
    uint64_t apic_id = ioapic_reg_rd(id, 0);
    //Construct the redirection table entry
    uint64_t redir = 0;
    redir |= vect;           //set vector
    redir |= 0b000    << 8;  //delivery mode: fixed
    redir |= 0b0      << 11; //destination mode: physical
    redir |= 0b0      << 12; //clear delivery status
    redir |= low_trig << 13; //pin polarity: depending on flags
    redir |= lev_trig << 15; //trigger mode: depending on flags
    redir |= 0b0      << 16; //masked: no
    redir |= apic_id  << 56; //destination: this CPU
    //Write the entry
    ioapic_reg_wr(id, redir_entry_reg, redir);
    ioapic_reg_wr(id, redir_entry_reg + 1, redir >> 32);

    krnl_write_msgf(__FILE__, "mapped GSI %i triggered at %s %s to intr vector %i",
        irq, low_trig ? "low" : "high", lev_trig ? "level" : "edge", vect);
}