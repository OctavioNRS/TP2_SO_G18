#include <naiveConsole.h>
#include <lib.h>
#include <stddef.h>

static uint32_t uintToBase(uint64_t value, char * buffer, uint32_t base);

static char buffer[64] = { '0' };
static uint8_t * const video = (uint8_t*)0xB8000;
static uint8_t * currentVideo = (uint8_t*)0xB8000;
static const uint32_t width = 80;
static const uint32_t height = 25;

void ncPrint(const char * string)
{
	int i;

	for (i = 0; string[i] != 0; i++)
		ncPrintChar(string[i]);
}

void ncPrintChar(char character)
{
	*currentVideo = character;
	currentVideo += 2;
}

/*void displayCurrentTime() {
	uint8_t timeBuffer[7]; // [seconds, minutes, hours, weekday, day, month, year]
	char dateStr[30];
	char timeStr[20];
	char temp[3];
	
	getTime(timeBuffer);
	
	// Obtengo el dia de la semana desde el buffer
	uint8_t weekdayValue = bcdToDecimal(timeBuffer[3]);
	Weekday weekday = (Weekday)weekdayValue;
	
	// Formateo la cadena de fecha con el prefijo del dia de la semana
	strcpy(dateStr, weekdayShortNames[weekday-1]);
	strcat(dateStr, " ");
	
	// Agrego el dia
	formatTwoDigits(bcdToDecimal(timeBuffer[4]), temp);
	strcat(dateStr, temp);
	strcat(dateStr, "/");
	
	// Agrego el mes
	formatTwoDigits(bcdToDecimal(timeBuffer[5]), temp);
	strcat(dateStr, temp);
	strcat(dateStr, "/20");
	
	// Agrego el año
	formatTwoDigits(bcdToDecimal(timeBuffer[6]), temp);
	strcat(dateStr, temp);
	
	// Formateo la cadena de hora manualmente
	formatTwoDigits(bcdToDecimal(timeBuffer[2]), temp);
	strcpy(timeStr, temp);
	strcat(timeStr, ":");
	
	formatTwoDigits(bcdToDecimal(timeBuffer[1]), temp);
	strcat(timeStr, temp);
	strcat(timeStr, ":");
	
	formatTwoDigits(bcdToDecimal(timeBuffer[0]), temp);
	strcat(timeStr, temp);
	
	ncPrint("Current Date: ");
	colorPrint(dateStr, "blue", "white");
	ncNewline();
	
	ncPrint("Current Time: ");
	colorPrint(timeStr, "red", "white");
	ncNewline();
}*/

void ncNewline()
{
	do
	{
		ncPrintChar(' ');
	}
	while((uint64_t)(currentVideo - video) % (width * 2) != 0);
}

void ncPrintDec(uint64_t value)
{
	ncPrintBase(value, 10);
}

void ncPrintHex(uint64_t value)
{
	ncPrintBase(value, 16);
}

void ncPrintBin(uint64_t value)
{
	ncPrintBase(value, 2);
}

void ncPrintBase(uint64_t value, uint32_t base)
{
    uintToBase(value, buffer, base);
    ncPrint(buffer);
}

void ncClear()
{
	int i;

	for (i = 0; i < height * width; i++) {
		video[i * 2] = ' ';
		video[i * 2 + 1] = 0x0f;
	}
	currentVideo = video;
}

static uint32_t uintToBase(uint64_t value, char * buffer, uint32_t base)
{
	char *p = buffer;
	char *p1, *p2;
	uint32_t digits = 0;

	//Calculate characters for each digit
	do
	{
		uint32_t remainder = value % base;
		*p++ = (remainder < 10) ? remainder + '0' : remainder + 'A' - 10;
		digits++;
	}
	while (value /= base);

	// Terminate string in buffer.
	*p = 0;

	//Reverse string in buffer.
	p1 = buffer;
	p2 = p - 1;
	while (p1 < p2)
	{
		char tmp = *p1;
		*p1 = *p2;
		*p2 = tmp;
		p1++;
		p2--;
	}

	return digits;
}
