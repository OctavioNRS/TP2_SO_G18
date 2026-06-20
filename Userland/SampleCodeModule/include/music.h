/*
 * music.h — TAD `Song` (lista de `Note` + BPM) y partituras predefinidas para
 * reproducir por el PC Speaker vía `sys_play_sound`.
 */
#ifndef MUSIC_H
#define MUSIC_H

#define MAX_SONG_LENGTH 512

// Struct para representar una nota musical
typedef struct {
    int frequency;      // Frecuencia en Hz
    float duration;     // Duración como fracción de negra (ej.: 2.0 = redonda, 1.0 = negra, 0.5 = corchea)
} Note;

// Struct para representar una canción
typedef struct {
    int length;         // Número de notas en la canción
    int bpm;           // BPM
    int quarter_note;  // 60000/bpm (duración de una negra en milisegundos)
    Note notes[MAX_SONG_LENGTH];       // Array de notas
} Song;

void init_music();
void play_song(Song* song);
Song* get_rickroll_song();
Song* get_coffin_intro();
Song* get_coffin_chorus();

#endif