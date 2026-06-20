/*
 * soundDriver.c — PC Speaker vía PIT canal 2 + puerto 0x61.
 *
 * `playSound(frequency, duration_ms)` programa el PIT para generar la onda
 * cuadrada, habilita los bits del speaker en el puerto 0x61, espera la duración
 * con `sleep` activo del timer, y deshabilita el speaker al final.
 */
#include <soundDriver.h>
#include <timer.h>
#include <interrupts.h>
#include <lib.h>

// Puertos de control del altavoz
#define SPEAKER_PORT 0x61 // Speaker on/off
#define PIT_CONTROL_PORT 0x43 // Control del PIT
#define PIT_CHANNEL2_PORT 0x42 // Canal 2 del PIT
#define PIT_FREQUENCY 1193182 // Frecuencia del PIT

// Variables para el estado del sonido
static int sound_playing = 0;
static uint32_t sound_start_time = 0;
static uint32_t sound_duration = 0;

// Función para establecer la frecuencia del altavoz
static void setSpeakerFrequency(uint32_t frequency) {
    if (frequency == 0) {
        // Apago el altavoz
        uint8_t speaker_status = readPort(SPEAKER_PORT);
        writePort(SPEAKER_PORT, speaker_status & 0xFC);
        return;
    }
    
    // Calculo el divisor para el PIT
    uint32_t divisor = PIT_FREQUENCY / frequency;
    
    // Configuro el canal 2 del PIT para generar una onda cuadrada
    writePort(PIT_CONTROL_PORT, 0xB6);
    
    // Envío el divisor (primer byte, luego el segundo)
    writePort(PIT_CHANNEL2_PORT, (uint8_t)(divisor & 0xFF));
    writePort(PIT_CHANNEL2_PORT, (uint8_t)((divisor >> 8) & 0xFF));
    
    // Enciendo el altavoz
    uint8_t speaker_status = readPort(SPEAKER_PORT);
    writePort(SPEAKER_PORT, speaker_status | 0x03);
}

void playSound(uint32_t frequency, uint32_t duration_ms) {
    if (frequency == 0 || duration_ms == 0) {
        stopSound();
        return;
    }
    
    // Configuro el sonido
    sound_playing = 1;
    sound_start_time = milliseconds_elapsed();
    sound_duration = duration_ms;
    
    // Inicio la reproducción del sonido
    setSpeakerFrequency(frequency);
}

void stopSound(void) {
    sound_playing = 0;
    sound_start_time = 0;
    sound_duration = 0;
    setSpeakerFrequency(0);
}

int isSoundPlaying(void) {
    if (!sound_playing) {
        return 0;
    }
    
    // Chequeo si el tiempo de sonido ya pasó
    uint32_t current_time = milliseconds_elapsed();
    uint32_t elapsed = current_time - sound_start_time;
    
    if (elapsed >= sound_duration) {
        stopSound();
        return 0;
    }
    
    return 1;
} 