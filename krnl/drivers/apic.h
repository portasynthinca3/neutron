#ifndef PIC_H
#define PIC_H

#include "../stdlib.h"

//How many I/O APICs can Neutron handle
#define IO_APIC_MAX_CNT                 16

//ACPI MADT table structures
typedef struct {
    uint8_t type;
    uint8_t len;
    uint8_t data[256];
} __attribute__((packed)) madt_record_t;

//I/O APIC structure
typedef struct {
    uint8_t  valid;
    uint8_t  id;
    uint32_t mmio_base;
    uint32_t gsi_base;
} ioapic_t;

//APIC base Model-Specific Register
#define IA32_APIC_BASE_MSR              0x1B

//Local APIC registers

#define LAPIC_REG_ID                    0x20
#define LAPIC_REG_VER                   0x30
#define LAPIC_REG_TPR                   0x80
#define LAPIC_REG_APR                   0x90
#define LAPIC_REG_PPR                   0xA0
#define LAPIC_REG_EOI                   0xB0
#define LAPIC_REG_RRD                   0xC0
#define LAPIC_REG_LOGICAL_DEST          0xD0
#define LAPIC_REG_DEST_FMT              0xE0
#define LAPIC_REG_SPUR_INTR             0xF0

#define LAPIC_REG_ISR0                  0x100
#define LAPIC_REG_ISR1                  0x110
#define LAPIC_REG_ISR2                  0x120
#define LAPIC_REG_ISR3                  0x130
#define LAPIC_REG_ISR4                  0x140
#define LAPIC_REG_ISR5                  0x150
#define LAPIC_REG_ISR6                  0x160
#define LAPIC_REG_ISR7                  0x170

#define LAPIC_REG_TMR0                  0x180
#define LAPIC_REG_TMR1                  0x190
#define LAPIC_REG_TMR2                  0x1A0
#define LAPIC_REG_TMR3                  0x1B0
#define LAPIC_REG_TMR4                  0x1C0
#define LAPIC_REG_TMR5                  0x1D0
#define LAPIC_REG_TMR6                  0x1E0
#define LAPIC_REG_TMR7                  0x1F0

#define LAPIC_REG_IRR0                  0x200
#define LAPIC_REG_IRR1                  0x210
#define LAPIC_REG_IRR2                  0x220
#define LAPIC_REG_IRR3                  0x230
#define LAPIC_REG_IRR4                  0x240
#define LAPIC_REG_IRR5                  0x250
#define LAPIC_REG_IRR6                  0x260
#define LAPIC_REG_IRR7                  0x270

#define LAPIC_REG_ERR_ST                0x280
#define LAPIC_REG_LVT_CMCI              0x2F0
#define LAPIC_REG_ICR0                  0x300
#define LAPIC_REG_ICR1                  0x310
#define LAPIC_REG_LVT_TIM               0x320
#define LAPIC_REG_LVT_THERM             0x330
#define LAPIC_REG_LVT_PERFMON           0x340
#define LAPIC_REG_LVT_LINT0             0x350
#define LAPIC_REG_LVT_LINT1             0x360
#define LAPIC_REG_LVT_ERR               0x370
#define LAPIC_REG_TIMR_INITCNT          0x380
#define LAPIC_REG_TIMR_CURCNT           0x390
#define LAPIC_REG_TIMR_DIVCONF          0x3E0

//Function prototypes

//Common
void apic_init (void);
//LAPIC operations
uint32_t lapic_reg_rd (uint32_t reg);
void     lapic_reg_wr (uint32_t reg, uint32_t val);
uint32_t lapic_get_id (void);
void     lapic_eoi    (void);
//I/O APIC operations
uint32_t ioapic_reg_rd  (uint32_t id, uint32_t reg);
void     ioapic_reg_wr  (uint32_t id, uint32_t reg, uint32_t val);
void     ioapic_map_irq (uint32_t id, uint8_t irq, uint8_t vect);

#endif