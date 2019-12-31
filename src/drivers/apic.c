//Neutron Project
//Local APIC driver

#include "./apic.h"
#include "../stdlib.h"

//Local APIC base address
uint64_t lapic_base;

/*
 * Initializes the Advanced Programmable Interrupt Controller
 */
void apic_init(void){
    //Read LAPIC base MSR
    uint64_t lapic_base_msr = rdmsr(IA32_APIC_BASE_MSR);
    //Actual LAPIC base is held outside the lower 11 bits
    //lapic_base = lapic_base_msr & 0xFFFFFFFFFFFFF800;
    lapic_base = 0xFEE00000;
    //Set the enable flag and write the register
    lapic_base_msr |= 1 << 11;
    wrmsr(IA32_APIC_BASE_MSR, lapic_base_msr);
    //Set task priority to 0x20
    apic_reg_wr(LAPIC_REG_TPR, 0x20);
    //Mask all interrupts in the LVT except for LI0 and LI1
    apic_reg_wr(LAPIC_REG_LVT_ERR, 0x10000);
    apic_reg_wr(LAPIC_REG_LVT_TIM, 0x10000);
    apic_reg_wr(LAPIC_REG_LVT_CMCI, 0x10000);
    apic_reg_wr(LAPIC_REG_LVT_THERM, 0x10000);
    apic_reg_wr(LAPIC_REG_LVT_LINT0, 0x8700);
    apic_reg_wr(LAPIC_REG_LVT_LINT1, 0x400);
    apic_reg_wr(LAPIC_REG_LVT_PERFMON, 0x10000);
    //Write the spurious interrupt register bit 8 to receive interrupts AND spurious interrupt ID 0xFF
    apic_reg_wr(LAPIC_REG_SPUR_INTR, apic_reg_rd(LAPIC_REG_SPUR_INTR) | (1 << 8) | 0xFF);
    //Set logical destination
    apic_reg_wr(LAPIC_REG_DEST_FMT, 0x0FFFFFFF);
    apic_reg_wr(LAPIC_REG_LOGICAL_DEST, (apic_reg_rd(LAPIC_REG_LOGICAL_DEST) & 0x00FFFFFF) | 1);
}

/*
 * Reads LAPIC register
 */
uint32_t apic_reg_rd(uint32_t reg){
    return *(uint32_t*)(lapic_base + reg);
}

/*
 * Writes LAPIC register
 */
void apic_reg_wr(uint32_t reg, uint32_t val){
    *(uint32_t*)(lapic_base + reg) = val;
}

/*
 * Returns the ID of the LAPIC
 */
uint32_t apic_get_id(void){
    return apic_reg_rd(LAPIC_REG_ID);
}

/*
 * Send an EOI signal
 */
void apic_eoi(void){
    apic_reg_wr(LAPIC_REG_EOI, 0);
}