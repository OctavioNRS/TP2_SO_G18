/*
 * music.c — Partituras hardcodeadas para reproducir por el speaker PC.
 *
 * Cada canción es un arreglo de `Note { frecuencia, duración_ms }` que
 * `play_song` recorre llamando a `sys_play_sound` con un pequeño "finger delay"
 * entre notas. Lo usan el comando `music` y el easter egg `ls`.
 */
#include <syscalls.h>
#include <music.h>
#include <cstandard.h>

enum Notes {
    C, C_SHARP, D, D_SHARP, E, F, F_SHARP, G, G_SHARP, A, A_SHARP, B
};

// Frecuencias de notas (en Hz)
// Formato: noteFrequencies[octava][nota]
// Notas: 0=C, 1=C#, 2=D, 3=D#, 4=E, 5=F, 6=F#, 7=G, 8=G#, 9=A, 10=A#, 11=B
static const int noteFrequencies[][12] = {
    {16, 17, 18, 19, 21, 22, 23, 25, 26, 28, 29, 31},     // Octave 0
    {33, 35, 37, 39, 41, 44, 46, 49, 52, 55, 58, 62},     // Octave 1
    {65, 69, 73, 78, 82, 87, 93, 98, 104, 110, 117, 123}, // Octave 2
    {131, 139, 147, 156, 165, 175, 185, 196, 208, 220, 233, 247},  // Octave 3
    {262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494},  // Octave 4
    {523, 554, 587, 622, 659, 698, 740, 784, 831, 880, 932, 988},  // Octave 5
    {1047, 1109, 1175, 1245, 1319, 1397, 1480, 1568, 1661, 1760, 1865, 1976}  // Octave 6
};

static Song init_song(Note* notes, int length, int bpm) {
    Song song;
    for (int i = 0; i < length; i++)
    {
        song.notes[i] = notes[i];
    }
    
    song.length = length;
    song.bpm = bpm;
    song.quarter_note = 60000/bpm;
    return song;
}

static Song rickroll_song;
static Song coffin_intro;
static Song coffin_chorus;

void init_music() {
    Note rickroll_notes[] = {
        {noteFrequencies[4][C], 1.5},   // C4 (Negra con puntillo)
        {noteFrequencies[4][D], 1.5},   // D4 (Corchea ligada a negra)
        {noteFrequencies[3][G], 1.0},   // G3 (negra)
        {noteFrequencies[4][D], 1.5},   // D4 (Negra con puntillo)
        {noteFrequencies[4][E], 1.5},   // E4 (Corchea ligada a corchea)
        {noteFrequencies[4][G], 0.125}, // G4 (fusa) La partitura dice semicorchea, pero suena mejor con fusa
        {noteFrequencies[4][F], 0.125}, // F4 (fusa) La partitura dice semicorchea, pero suena mejor con fusa
        {noteFrequencies[4][E], 0.5},   // E4 (corchea)
        {noteFrequencies[4][C], 1.5},   // C4 - (negra con puntillo)
        {noteFrequencies[4][D], 1.5},   // D4 - (corchea ligada a negra)
        {noteFrequencies[3][G], 3},     // G3 - (negra ligada a redonda)
    };

    // La partitura dice 114 bpm, pero suena mejor con 130 bpm
    rickroll_song = init_song(rickroll_notes, sizeof(rickroll_notes) / sizeof(Note), 130);

    Note coffin_intro_notes[] = {
        // Compás 1
        {noteFrequencies[4][C], 0.5},   // C4 (semicorchea)
        {noteFrequencies[4][C], 0.5},
        {noteFrequencies[4][C], 0.5},
        {noteFrequencies[4][C], 0.5},

        {noteFrequencies[4][E], 0.5},   // E4 (semicorchea)
        {noteFrequencies[4][E], 0.5},
        {noteFrequencies[4][E], 0.5},
        {noteFrequencies[4][E], 0.5},

        // Compás 2
        {noteFrequencies[4][D], 0.5},   // D4 (semicorchea)
        {noteFrequencies[4][D], 0.5},
        {noteFrequencies[4][D], 0.5},
        {noteFrequencies[4][D], 0.5},

        {noteFrequencies[4][G], 0.5},   // G4 (semicorchea)
        {noteFrequencies[4][G], 0.5},
        {noteFrequencies[4][G], 0.5},
        {noteFrequencies[4][G], 0.5},

        // Compás 3 y 1era mitad de 4
        {noteFrequencies[4][A], 0.5},   // A4 (semicorchea)
        {noteFrequencies[4][A], 0.5},
        {noteFrequencies[4][A], 0.5},
        {noteFrequencies[4][A], 0.5},
        {noteFrequencies[4][A], 0.5},
        {noteFrequencies[4][A], 0.5},
        {noteFrequencies[4][A], 0.5},
        {noteFrequencies[4][A], 0.5},
        {noteFrequencies[4][A], 0.5},
        {noteFrequencies[4][A], 0.5},
        {noteFrequencies[4][A], 0.5},
        {noteFrequencies[4][A], 0.5},

        // 2da mitad de Compás 4
        {noteFrequencies[4][D], 0.5},   // D4 (semicorchea)
        {noteFrequencies[4][C], 0.5},   // C4 (semicorchea)
        {noteFrequencies[3][B], 0.5},   // B3 (semicorchea)
        {noteFrequencies[3][G], 0.5},   // G3 (semicorchea)
    };

    Note coffin_chorus_notes[] = {
        // Compás 5
        {noteFrequencies[3][A], 1},     // A3 (corchea)
        {noteFrequencies[3][A], 0.5},   // A3 (semicorchea)
        {noteFrequencies[4][E], 0.5},   // E4 (semicorchea)
        {noteFrequencies[4][D], 1},     // D4 (corchea)
        {noteFrequencies[4][C], 1},     // C4 (corchea)

        // Compás 6
        {noteFrequencies[3][B], 1},     // B3 (corchea)
        {noteFrequencies[3][B], 0.5},   // B3 (semicorchea)
        {noteFrequencies[3][B], 0.5},   // B3 (semicorchea)
        {noteFrequencies[4][D], 1},     // D4 (corchea)
        {noteFrequencies[4][C], 0.5},   // C4 (semicorchea)
        {noteFrequencies[3][B], 0.5},   // B3 (semicorchea)

        // Compás 7
        {noteFrequencies[3][A], 1},     // A3 (corchea)
        {noteFrequencies[3][A], 0.5},   // A3 (semicorchea)
        {noteFrequencies[5][C], 0.5},   // C5 (semicorchea)
        {noteFrequencies[4][B], 0.5},   // B4 (semicorchea)
        {noteFrequencies[5][C], 0.5},   // C5 (semicorchea)
        {noteFrequencies[4][B], 0.5},   // B4 (semicorchea)
        {noteFrequencies[5][C], 0.5},   // C5 (semicorchea)

        // Compás 8 (idem. 7)
        {noteFrequencies[3][A], 1},     // A3 (corchea)
        {noteFrequencies[3][A], 0.5},   // A3 (semicorchea)
        {noteFrequencies[5][C], 0.5},   // C5 (semicorchea)
        {noteFrequencies[4][B], 0.5},   // B4 (semicorchea)
        {noteFrequencies[5][C], 0.5},   // C5 (semicorchea)
        {noteFrequencies[4][B], 0.5},   // B4 (semicorchea)
        {noteFrequencies[5][C], 0.5},   // C5 (semicorchea)
    };

    // La partitura dice 70 bpm, pero suena mejor con 100 bpm
    coffin_intro = init_song(coffin_intro_notes, sizeof(coffin_intro_notes) / sizeof(Note), 100);
    coffin_chorus = init_song(coffin_chorus_notes, sizeof(coffin_chorus_notes) / sizeof(Note), 100);
}

Song* get_rickroll_song() {
    return &rickroll_song;
}

Song* get_coffin_intro() {
    return &coffin_intro;
}

Song* get_coffin_chorus() {
    return &coffin_chorus;
}

void play_song(Song* song) {
    const int finger_delay = song->quarter_note / 32;
    
    for(int i = 0; i < song->length; i++) {
        Note note = song->notes[i];
        sys_play_sound(note.frequency, song->quarter_note * note.duration);
        sys_sleep(song->quarter_note * note.duration + finger_delay);
    }
}

void coffin_dance() {
    const int bpm = 100; 
    const int quarter_note = 60000 / bpm;
    const int eighth_note = quarter_note / 2;
    const int finger_delay = quarter_note / 32;

    // Bar 1 - Am
    for (int i = 0; i < 4; i++) {
        sys_play_sound(noteFrequencies[4][C], eighth_note); // C5
        sys_sleep(eighth_note); sys_sleep(finger_delay);
    }
    for (int i = 0; i < 4; i++) {
        sys_play_sound(noteFrequencies[4][E], eighth_note); // E5
        sys_sleep(eighth_note); sys_sleep(finger_delay);
    }

    // Bar 2 - Am continued
    for (int i = 0; i < 4; i++) {
        sys_play_sound(noteFrequencies[4][D], eighth_note); // D5
        sys_sleep(eighth_note); sys_sleep(finger_delay);
    }
    for (int i = 0; i < 4; i++) {
        sys_play_sound(noteFrequencies[4][G], eighth_note); // G5
        sys_sleep(eighth_note); sys_sleep(finger_delay);
    }

    // Bar 3 - Am continued
    for (int i = 0; i < 8; i++) {
        sys_play_sound(noteFrequencies[4][A], eighth_note); // A5
        sys_sleep(eighth_note); sys_sleep(finger_delay);
    }

    // Bar 4
    for (int i = 0; i < 4; i++) {
        sys_play_sound(noteFrequencies[4][A], eighth_note); // A5
        sys_sleep(eighth_note); sys_sleep(finger_delay);
    }   
    sys_play_sound(noteFrequencies[4][D], eighth_note); // D5
    sys_sleep(eighth_note); sys_sleep(finger_delay);
    sys_play_sound(noteFrequencies[4][C], eighth_note); // C5
    sys_sleep(eighth_note); sys_sleep(finger_delay);
    sys_play_sound(noteFrequencies[3][B], eighth_note); // B4
    sys_sleep(eighth_note); sys_sleep(finger_delay);
    sys_play_sound(noteFrequencies[3][G], eighth_note); // G4
    sys_sleep(eighth_note); sys_sleep(finger_delay);

    // Bar 5 - F
    sys_play_sound(noteFrequencies[3][A], quarter_note); // A4
    sys_sleep(quarter_note); sys_sleep(finger_delay);
    sys_play_sound(noteFrequencies[3][A], eighth_note); // A4 again
    sys_sleep(eighth_note); sys_sleep(finger_delay);
    sys_play_sound(noteFrequencies[4][E], eighth_note); // E5
    sys_sleep(eighth_note); sys_sleep(finger_delay);
    sys_play_sound(noteFrequencies[4][D], quarter_note); // D5
    sys_sleep(quarter_note); sys_sleep(finger_delay);
    sys_play_sound(noteFrequencies[4][C], quarter_note); // C5
    sys_sleep(quarter_note); sys_sleep(finger_delay);

    // Bar 6 - G
    sys_play_sound(noteFrequencies[3][B], quarter_note); // B4
    sys_sleep(quarter_note); sys_sleep(finger_delay);
    sys_play_sound(noteFrequencies[3][B], eighth_note); // B4
    sys_sleep(eighth_note); sys_sleep(finger_delay);
    sys_play_sound(noteFrequencies[3][B], eighth_note); // B4
    sys_sleep(eighth_note); sys_sleep(finger_delay);
    sys_play_sound(noteFrequencies[4][D], quarter_note); // D5
    sys_sleep(quarter_note); sys_sleep(finger_delay);
    sys_play_sound(noteFrequencies[4][C], eighth_note); // C5
    sys_sleep(eighth_note); sys_sleep(finger_delay);
    sys_play_sound(noteFrequencies[3][B], eighth_note); // B4
    sys_sleep(eighth_note); sys_sleep(finger_delay);

    // Bar 7 & 8 - Am
    // Por alguna razón me funciona mal al final de la segunda vuelta
    for (int i = 0; i < 2; i++) {
        sys_play_sound(noteFrequencies[3][A], quarter_note); // A4
        sys_sleep(quarter_note); sys_sleep(finger_delay);
        sys_play_sound(noteFrequencies[3][A], eighth_note); // A4
        sys_sleep(eighth_note); sys_sleep(finger_delay);
        sys_play_sound(noteFrequencies[5][C], eighth_note); // C6
        sys_sleep(eighth_note); sys_sleep(finger_delay);
        sys_play_sound(noteFrequencies[4][B], eighth_note); // B5
        sys_sleep(eighth_note); sys_sleep(finger_delay);
        sys_play_sound(noteFrequencies[5][C], eighth_note); // C6
        sys_sleep(eighth_note); sys_sleep(finger_delay);
        sys_play_sound(noteFrequencies[4][B], eighth_note); // B5
        sys_sleep(eighth_note); sys_sleep(finger_delay);
        sys_play_sound(noteFrequencies[5][C], eighth_note); // C6
        sys_sleep(eighth_note); sys_sleep(finger_delay);
    }
}
