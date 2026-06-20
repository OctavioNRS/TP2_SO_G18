#include <cstandard.h>
#include <stdarg.h>

static int vprintf(const char *format, va_list args) {
    int count = 0;
    char buffer[32];
    
    while (*format) {
        if (*format == '%' && *(format + 1)) {
            format++; // Salteo el '%'
            
            switch (*format) {
                case 'd':
                case 'i': {
                    int value = va_arg(args, int);
                    numToString(value, buffer);
                    char *ptr = buffer;
                    while (*ptr) {
                        putChar(*ptr);
                        ptr++;
                        count++;
                    }
                    break;
                }
                case 'c': {
                    char c = (char)va_arg(args, int);
                    putChar(c);
                    count++;
                    break;
                }
                case 's': {
                    char *str = va_arg(args, char*);
                    if (str == NULL) str = "(null)";
                    while (*str) {
                        putChar(*str);
                        str++;
                        count++;
                    }
                    break;
                }
                case '%': {
                    putChar('%');
                    count++;
                    break;
                }
                default: {
                    // Si no es un formato conocido, imprimimos el % y el caracter
                    putChar('%');
                    putChar(*format);
                    count += 2;
                    break;
                }
            }
        } else {
            // Si no es un formato, imprimimos el caracter
            putChar(*format);
            count++;
        }
        format++;
    }
    
    return count;
}

int printf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    return 0;
}

static int vscanf(const char *format, va_list args) {
    int count = 0;
    char buffer[256];
    int bufferIndex = 0;
    
    while (*format) {
        if (*format == '%' && *(format + 1)) {
            format++; // Salteo el '%'
            
            switch (*format) {
                case 'd':
                case 'i': {
                    // Salteo los espacios en blanco
                    char c;
                    do {
                        c = getChar();
                    } while (c == ' ' || c == '\t' || c == '\n' || c == '\r');
                    
                    // Leo el numero (negativo o positivo)
                    bufferIndex = 0;
                    if (c == '-' || isNumber(c)) {
                        buffer[bufferIndex++] = c;
                        while ((c = getChar()) && isNumber(c)) {
                            buffer[bufferIndex++] = c;
                        }
                        buffer[bufferIndex] = '\0';
                        
                        int *ptr = va_arg(args, int*);
                        *ptr = stringToInt(buffer);
                        count++;
                    }
                    break;
                }
                case 'c': {
                    char c = getChar();
                    char *ptr = va_arg(args, char*);
                    *ptr = c;
                    count++;
                    break;
                }
                case 's': {
                    // Salteo los espacios en blanco
                    char c;
                    do {
                        c = getChar();
                    } while (c == ' ' || c == '\t' || c == '\n' || c == '\r');
                    
                    // Leo la cadena de caracteres hasta que encuentre un espacio en blanco
                    char *ptr = va_arg(args, char*);
                    int strIndex = 0;
                    if (c != '\t' && c != '\n' && c != '\r' && c != '\0') {
                        ptr[strIndex++] = c;
                        while ((c = getChar()) && c != '\t' && c != '\n' && c != '\r' && c != '\0') {
                            ptr[strIndex++] = c;
                        }
                    }
                    ptr[strIndex] = '\0';
                    count++;
                    break;
                }
                case '%': {
                    char c = getChar();
                    if (c != '%') {
                        // Si no es un %, retorno
                        return count;
                    }
                    break;
                }
                default: {
                    // Si no es un formato conocido, salteo
                    break;
                }
            }
        } else {
            // Si no es un formato, salteo
            char c = getChar();
            if (c != *format) {
                // Si no coincide con el formato, retorno
                return count;
            }
        }
        format++;
    }
    
    return count;
}

int scanf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    int result = vscanf(format, args);
    va_end(args);
    return result;
}

int puts(char *string) {
    int out = sys_write(STDOUT, string, strlen(string));
    sys_write(STDOUT, "\n", 1);
    return out;
}

int putc(char c, int fd) {
    return sys_write(fd, &c, 1);
}

int putChar(char c) {
    return putc(c, STDOUT);
}

int getc(int fd) {
    char c;
    int bytesRead = sys_read(fd, &c, 1);
    if (bytesRead <= 0) return bytesRead; //Esto contempla el caso de error
    return c;
}

int getChar() {
    return getc(STDIN);
}

int strcmp(const char *p1, const char *p2)
{
    const unsigned char *s1 = (const unsigned char *)p1;
    const unsigned char *s2 = (const unsigned char *)p2;
    unsigned char c1, c2;

    do
    {
        c1 = (unsigned char)*s1++;
        c2 = (unsigned char)*s2++;
        if (c1 == '\0')
            return c1 - c2;
    } while (c1 == c2);

    return c1 - c2;
}

int strlen(const char *str) {
    int len = 0;
    while (*str) {
        len++, str++;
    }
    return len;
}

void numToString(long number, char *outBuffer) {
    char stack[20];
    int stackPointer = 0;

    int index = 0;
    if (number < 0) {
        number *= -1;
        outBuffer[index++] = '-';
    }
    do {
        char digit = number % 10;
        stack[stackPointer++] = digit + '0';
        number /= 10;
    } while (number > 0);

    while (stackPointer > 0) {
        stackPointer--;
        char pop = stack[stackPointer];
        outBuffer[index++] = pop;
    }

    outBuffer[index] = '\0';
}

int64_t stringToInt(char *str) {
    int64_t out = 0;
    char c;
    int negative = 0;
    if (str[0] == '-') {
        negative = 1;
        str++;
    }
    while((c = *str)) {
        if (!isNumber(c)) return -1;
        out *= 10;
        out += (c - '0');
        str++;
    }
    if (negative) out *= -1;
    return out;
}

int32_t hexStringToInt(char *str) {
    uint64_t out = 0;
    char c;
    if (str[0] == '-') {
        return -1;
    }
    if (str[0] == '0' && toLower(str[1]) == 'x') {
        str+=2;
    }
    while((c = *str) != '\0') {
        c = toLower(c);
        int isNum = isNumber(c);
        if (!isNum && (c < 'a' || c > 'f')) return c;
        int value = (isNum ? (c-'0') : c-'a'+10);
        out *= 16;
        out += value;
        str++;
    }
    return out;
}

int isNumericalString(char *str) {
    if (*str == 0) return 0;
    while (*str) {
        if (!isNumber(*str)) return 0;
        str++;
    }
    return 1;
}

int isNumber(char c) {
    return '0' <= c && c <= '9';
}

int isLower(char c) {
    return 'a' <= c && c <= 'z';
}

int isUpper(char c) {
    return 'A' <= c && c <= 'Z';
}

int isLetter(char c) {
    return isLower(c) || isUpper(c);
}

char toLower(char c) {
    if (isUpper(c)) return c + ('a'-'A');
    return c;
}
char toUpper(char c) {
    if (isLower(c)) return c + ('A'-'a');
    return c;
}

int isAlnum(char c) {
    return isLetter(c) || isNumber(c);
}

int isPrintableAscii(char c) {
    int symbol = ' ' <= c && c <= '~';
    int validSpecial = (c == '\n') || (c == '\b') || (c == '\t');
    return symbol || validSpecial;
}

int clamp(int num, int min, int max) {
    if (num <= min) return min;
    if (num >= max) return max;
    return num;
}

static uint16_t rngCurrent = 0;

uint16_t rng(uint16_t input) {
    if (input == 0x560A) input = 0;
    uint16_t S0 = (uint8_t)input << 8;
    S0 = S0 ^ input;
    input = ((S0 & 0xFF) << 8) | ((S0 & 0xFF00) >> 8);
    S0 = ((uint8_t)S0 << 1) ^ input;
    short S1 = (S0 >> 1) ^ 0xFF00;
    if ((S0 & 1) == 0) {
        if (S1 == 0xAA55) input = 0;
        else input = S1 ^ 0x1FF4;
    }
    else input = S1 ^ 0x8180;
    return (uint16_t)input;
}

void setRngSeed(uint16_t seed) {
    rngCurrent = seed;
}

uint16_t randomNext() {
    uint16_t out = rngCurrent;
    rngCurrent = rng(rngCurrent);
    return out;
}

uint64_t randomRange(int64_t min, int64_t max) {
    unsigned long long range = (unsigned long long)(max - min) + 1;
    unsigned long long scaled = (unsigned long long)randomNext() * range / RANDOM_MAX;
    return min + (int)scaled;
}

void * memcpy(void * destination, const void * source, uint64_t length)
{
	uint64_t i;

	if ((uint64_t)destination % sizeof(uint32_t) == 0 &&
		(uint64_t)source % sizeof(uint32_t) == 0 &&
		length % sizeof(uint32_t) == 0)
	{
		uint32_t *d = (uint32_t *) destination;
		const uint32_t *s = (const uint32_t *)source;

		for (i = 0; i < length / sizeof(uint32_t); i++)
			d[i] = s[i];
	}
	else
	{
		uint8_t * d = (uint8_t*)destination;
		const uint8_t * s = (const uint8_t*)source;

		for (i = 0; i < length; i++)
			d[i] = s[i];
	}

	return destination;
}