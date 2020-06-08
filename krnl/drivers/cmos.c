//Neuton Project
//CMOS stuff handler

#include "./cmos.h"
#include "../stdlib.h"
#include "./apic.h"
#include "../krnl.h"

time_t cur_timestamp = 0;

/*
 * Returns the current timestamp
 */
time_t time_get(void){
    return cur_timestamp;
}

/*
 * Writes a value to the CMOS register
 */
void cmos_write(uint8_t reg, uint8_t val){
    outb(0x70, reg);
    outb(0x71, val);
}

/*
 * Reads a value from the CMOS register
 */
uint8_t cmos_read(uint8_t reg){
    outb(0x70, reg);
    return inb(0x71);
}

/*
 * Enables update finished interrupt
 */
void rtc_init(void){
    //Enable that interrupt
    uint8_t status_b = cmos_read(CMOS_STATUS_REG_B);
    status_b |= 1 << 4;
    cmos_write(CMOS_STATUS_REG_B, status_b);
    //Route it to CPU vector #35
    ioapic_map_irq(0, 8, 35);
    //Just to be sure, read SR_C
    cmos_read(CMOS_STATUS_REG_C);

    krnl_write_msg(__FILE__, "initialized");
}

/*
 * Processes the RTC update finished interrupt
 */
void rtc_intr(void){
    cur_timestamp = rtc_read_time();
    //Read status register C (an additional EOI for this chip)
    cmos_read(CMOS_STATUS_REG_C);
}

/*
 * Reads RTC time and converts it to the timestamp
 */
int64_t rtc_read_time(void){
    uint64_t h, m, s, d, mo, y;
    h = cmos_read(CMOS_HOUR_REG);
    m = cmos_read(CMOS_MIN_REG);
    s = cmos_read(CMOS_SEC_REG);
    d = cmos_read(CMOS_DAY_REG);
    mo = cmos_read(CMOS_MONTH_REG);
    y = cmos_read(CMOS_YEAR_REG);

    //Read Status Register B to find out the data format
    uint8_t status_b = cmos_read(CMOS_STATUS_REG_B);
    //If bit 2 is set, the values are BCD
    if(!(status_b & (1 << 2))){
        h = (((h >> 4) & 0x0F) * 10) + (h & 0x0F);
        m = (((m >> 4) & 0x0F) * 10) + (m & 0x0F);
        s = (((s >> 4) & 0x0F) * 10) + (s & 0x0F);
        d = (((d >> 4) & 0x0F) * 10) + (d & 0x0F);
        mo = (((mo >> 4) & 0x0F) * 10) + (mo & 0x0F);
        y = (((y >> 4) & 0x0F) * 10) + (y & 0x0F);
    }
    //If bit 1 is set, the time is in the 12-hour format
    //  add 12 to the hour count if its highest bit is set
    if(status_b & (1 << 1)){
        if(h & (1 << 7)){
            h += 12;
            //Reset the highest bit as this bit set will now mess with the value
            h &= ~(1 << 7);
        }
    }
    //We're in the 21st century after all
    //(unless humans are extinct and you are an alien or a some kind of
    // post-human civilization member reading this piece of source code
    // and judging the entire civilization just by what I was able to create
    // because that's the only thing that was left/you were able to decypher)
    y += 2000;

    //Now we need to convert this mess into neutron timestamp
    return rtc_to_timestamp(y, mo, d, h, m, s, 0);
}

/*
 * Converts date and time to timestamp
 */
time_t rtc_to_timestamp(int64_t y, int64_t mo, int64_t d, int64_t h, int64_t m, int64_t s, int64_t ms){
    y -= 1970;
    int64_t days[4][12] = {{31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
                           {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
                           {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
                           {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}};

    time_t stamp = y * 3600 * 24 * 365;
    //Add extra days for leap years
    for (int i = 0; i < y; i++)
        if (i % 400 == 0 || (i % 4 == 0 && i % 100 != 0))
            stamp += 3600 * 24;
    //Add days for this year
    for (int i = 1; i < mo; i++)
        stamp += 3600 * 24 * days[y % 4][i - 1];
    //Add days, hours, minutes and seconds
    stamp += (d - 1) * 3600 * 24;
    stamp += h * 3600;
    stamp += m * 60;
    stamp += s;
    //Add milliseconds
    stamp *= 1000;
    stamp += ms;
    return stamp;
}