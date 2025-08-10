#ifndef GUITAR_HERO_H
#define GUITAR_HERO_H

#include "ioctl_cmds.h"
#include "mapeamento_audio.h"
#include "device.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <linux/joystick.h>
#include <time.h>
#include <stdint.h>
#include <sys/time.h>
#include <errno.h>


#define MAX_NOTES 2000
#define LEVEL_FILENAME "notes.txt"
#define TARGET_FPS 60
#define FRAME_DELAY 16
#define ALTURA_DA_PISTA 20
#define TEMPO_DE_ANTEVISAO 3.0f
#define MAX_MISSES 20
#define AUDIO_BUFFER_SIZE 1024

// Cores ANSI para cada pista
#define COLOR_GREEN "\033[32m"
#define COLOR_RED "\033[31m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_BLUE "\033[34m"
#define COLOR_RESET "\033[0m"

typedef struct {
    float timestamp;
    char note_name[5];
    int note_index;
    int foi_processada;
    int foi_pressionada;
} Note;

typedef struct {
    int score;
    int combo;
    int consecutive_misses;
    int game_over;
    Uint32 start_time;
    int musica_playing;
    Mix_Music *musica;
    Note level_notes[MAX_NOTES];
    int note_count;
    int joy_fd;
    int fd_pbuttons;
    int fd_hardware; // Novo descritor para todas as operações
} GameState;

/***********************************************
 *           DECLARAÇÕES DE FUNÇÕES
 ***********************************************/
// Funções de inicialização
void init_terminal();
int init_joystick(GameState *state);
void enableRawMode();
void disableRawMode();
int kbhit(void);

// Funções do jogo
void carregar_nivel(GameState *state);
void inicializar_jogo(GameState *state);
void process_input(GameState *state, double tempo_decorrido);
void check_joystick_input(GameState *state, double tempo_decorrido);
void check_hits(GameState *state, int pista, double tempo_decorrido);
void update_game(GameState *state, double tempo_decorrido);
void render_game(GameState *state, double tempo_decorrido);
void finalizar_jogo(GameState *state);
int init_pbuttons(GameState *state);
int init_hardware();
void close_hardware(int fd);
int read_pbuttons(int fd);
void flash_led(int fd, int acerto);
void update_displays(int fd, int score, int misses);


#endif