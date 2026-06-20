/*
 * lib.c — Implementaciones mínimas de utilidades estándar del kernel.
 *
 * Incluye `memset`, `memcpy`, `strcmp`, `strcpy`, `strlen`, `uint64ToHex` y
 * `concat_strings`. Sustituyen a la libc, que no está disponible en este
 * entorno freestanding (-nostdlib).
 */
#include <stdint.h>

void * memset(void * destination, int32_t c, uint64_t length)
{
	uint8_t chr = (uint8_t)c;
	char * dst = (char*)destination;

	while(length--)
		dst[length] = chr;

	return destination;
}

void * memcpy(void * destination, const void * source, uint64_t length)
{
	/*
	* memcpy does not support overlapping buffers, so always do it
	* forwards. (Don't change this without adjusting memmove.)
	*
	* For speedy copying, optimize the common case where both pointers
	* and the length are word-aligned, and copy word-at-a-time instead
	* of byte-at-a-time. Otherwise, copy by bytes.
	*
	* The alignment logic below should be portable. We rely on
	* the compiler to be reasonably intelligent about optimizing
	* the divides and modulos out. Fortunately, it is.
	*/
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

/* Funciones de la librería estándar de C añadidas para el proyecto */

int strcmp(const char *p1, const char *p2)
{
  const unsigned char *s1 = (const unsigned char *) p1;
  const unsigned char *s2 = (const unsigned char *) p2;
  unsigned char c1, c2;

  do
    {
      c1 = (unsigned char) *s1++;
      c2 = (unsigned char) *s2++;
      if (c1 == '\0')
	return c1 - c2;
    }
  while (c1 == c2);

  return c1 - c2;
}

char *strcpy(char *dest, const char *src) {
    char *temp = dest;
    while((*dest++ = *src++) != 0);
    return temp;
}

// Función helper para convertir un uint64_t a una cadena de caracteres en hexadecimal
void uint64ToHex(uint64_t value, char *buffer) {
    const char hexChars[] = "0123456789ABCDEF";
    buffer[0] = '0';
    buffer[1] = 'x';
    
    for (int i = 15; i >= 0; i--) {
        buffer[2 + (15 - i)] = hexChars[(value >> (i * 4)) & 0xF];
    }
    buffer[18] = '\0';
}

// Función helper para obtener la longitud de una cadena de caracteres
uint64_t strlen(const char *str) {
    uint64_t len = 0;
    while (str[len]) len++;
    return len;
}

// Función helper para concatenar strings
void concat_strings(char *dest, const char *str1, const char *str2) {
    int pos = 0;
    
    // Copio str1
    for (int i = 0; str1[i]; i++) {
        dest[pos++] = str1[i];
    }
    
    // Copio str2
    for (int i = 0; str2[i]; i++) {
        dest[pos++] = str2[i];
    }
    
    dest[pos] = '\0';
}
