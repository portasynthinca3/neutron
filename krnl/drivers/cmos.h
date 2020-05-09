#ifndef CMOS_H
#define CMOS_H

#include "../stdlib.h"

//Definitions

#define CMOS_STATUS_REG_A           0x0A
#define CMOS_STATUS_REG_B           0x0B
#define CMOS_STATUS_REG_C           0x0C
#define CMOS_HOUR_REG               0x04
#define CMOS_MIN_REG                0x02
#define CMOS_SEC_REG                0x00
#define CMOS_DAY_REG                0x07
#define CMOS_MONTH_REG              0x08
#define CMOS_YEAR_REG               0x09

//Function prototypes

//Register reading/writing
void    cmos_write (uint8_t reg, uint8_t val);
uint8_t cmos_read  (uint8_t reg);
//RTC stuff
void    rtc_init         (void);
void    rtc_intr         (void);
int64_t rtc_read_time    (void);
//Timekeeping
time_t  time_get         (void);
time_t  rtc_to_timestamp (int64_t y, int64_t mo, int64_t d, int64_t h, int64_t m, int64_t s, int64_t ms);

#endif