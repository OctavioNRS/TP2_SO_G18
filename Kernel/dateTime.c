#include <stdint.h>
#include <dateTime.h>

// Funcion auxiliar para convertir BCD a decimal
uint8_t bcdToDecimal(uint8_t bcd) {
	return ((bcd & 0xF0) >> 4) * 10 + (bcd & 0x0F);
}

/*// Funcion auxiliar para convertir decimal a cadena (2 digitos con cero al principio)
void formatTwoDigits(uint8_t value, char* buffer) {
	buffer[0] = '0' + (value / 10);
	buffer[1] = '0' + (value % 10);
	buffer[2] = '\0';
}

// Array de nombres cortos de los dias de la semana (3 letras)
static const char* weekdayShortNames[] = {
	"Sun",
	"Mon",
	"Tue",
	"Wed",
	"Thu",
	"Fri",
	"Sat"
};*/

// Funcion para guardar la fecha actual
void getCurrentTime(uint8_t* hours, uint8_t* minutes, uint8_t* seconds){
	getTime(hours, minutes, seconds);
	*seconds = bcdToDecimal(*seconds);
	*minutes = bcdToDecimal(*minutes);
	*hours = bcdToDecimal(*hours);
}

// Funcion para guardar la hora actual
void getCurrentDate(uint8_t* weekday, uint8_t* day, uint8_t* month, uint8_t* year) {
	getDate(weekday, day, month, year);
	*weekday = bcdToDecimal(*weekday);
	*day = bcdToDecimal(*day);
	*month = bcdToDecimal(*month);
	*year = bcdToDecimal(*year);
}
