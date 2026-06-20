/*
 * hexColors.h — Constantes de color RGB 24-bit y macros para las escape
 * sequences que interpreta el terminal de userland (`\033f<hex>;` foreground,
 * `\033b<hex>;` background, `\033r;` reset, `\033c;` clear).
 */
#ifndef HEXCOLORS_H
#define HEXCOLORS_H
#include <stdint.h>

#define BLACK 0x000000
#define WHITE 0xFFFFFF
#define RED 0xFF0000
#define GREEN 0x00FF00
#define BLUE 0x0000FF
#define YELLOW 0xFFFF00
#define CYAN 0x00FFFF
#define MAGENTA 0xFF00FF
#define GRAY 0x7F7F7F
#define LIGHT_GRAY 0xBEBEBE
#define DARK_GRAY 0x3F3F3F
#define ORANGE 0xFFA500
#define PURPLE 0x8000FF

#define COLOR_FG(hex6)  "\033f" hex6 ";"     /* setea color de texto */
#define COLOR_BG(hex6)  "\033b" hex6 ";"     /* setea color de fondo (limpia) */
#define COLOR_RESET     "\033r;"             /* vuelve a defaults */
#define COLOR_CLEAR     "\033c;"             /* limpia pantalla */

#endif