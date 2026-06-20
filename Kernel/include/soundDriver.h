/*
 * soundDriver.h — Reproducción por PC Speaker (PIT canal 2 + puerto 0x61).
 */
#ifndef _SOUND_DRIVER_H_
#define _SOUND_DRIVER_H_

#include <stdint.h>

// Function to play a sound with specified frequency and duration
void playSound(uint32_t frequency, uint32_t duration_ms);

// Function to stop sound
void stopSound(void);

// Function to check if sound is currently playing
int isSoundPlaying(void);

#endif 