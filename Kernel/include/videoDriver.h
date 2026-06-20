/*
 * videoDriver.h — Primitivas de dibujo sobre el framebuffer VESA.
 *
 * Coordenadas en píxeles (x crece a la derecha, y hacia abajo). Los glifos del
 * font tienen 8x16 píxeles; `scale` permite agrandarlos por múltiplo entero.
 */
#ifndef VIDEO_DRIVER_H
#define VIDEO_DRIVER_H
#include <stdint.h>

#define CHAR_WIDTH 8
#define CHAR_HEIGHT 16
#define STRING_SPACING 0

uint16_t getScreenWidth();
uint16_t getScreenHeight();
int withinScreenBounds(uint64_t x, uint64_t y);
void putPixel(uint32_t hexColor, uint64_t x, uint64_t y);
uint32_t readPixel(uint64_t x, uint64_t y);
void drawRectangle(uint32_t hexColor, uint64_t x, uint64_t y, uint64_t width, uint64_t height);
void drawCircle(uint32_t hexColor, int64_t centerX, int64_t centerY, uint64_t radius);
void drawCircumference(uint32_t hexColor, int64_t centerX, int64_t centerY, uint64_t radius);
void drawSymbol(unsigned char c, uint64_t x, uint64_t y, uint32_t fgcolor, uint32_t bgcolor, int scale);
void drawString(unsigned char *string, int x, int y, uint32_t fgcolor, uint32_t bgcolor, int scale);
void setBackground(uint32_t color);
void setAuxFramebuffer(uint8_t *buffer, uint32_t width, uint32_t height);
void renderAux();
void scrollUp(uint64_t pixelRows, uint32_t fillColor);

#endif