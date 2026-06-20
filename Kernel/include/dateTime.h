/*
 * dateTime.h — Lectura de hora/fecha desde el RTC (CMOS 0x70/0x71).
 */
#ifndef DATE_TIME_H
#define DATE_TIME_H

uint8_t bcdToDecimal(uint8_t bcd);

void getCurrentTime(uint8_t* seconds, uint8_t* minutes, uint8_t* hours);

void getCurrentDate(uint8_t* weekday, uint8_t* day, uint8_t* month, uint8_t* year);

#endif