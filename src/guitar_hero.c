#include "guitar_hero.h" 

// Tabela de consulta para os displays de 7 segmentos (ativo alto) 
// Cada valor representa a combinação de segmentos para um dígito (0-9) 
//  _a_ 
// |   | 
// f   b 
// |___| 
//  _g_ 
// |   | 
// e   c 
// |___| 
//  _d_ 
const uint32_t seg7_codes[] = { 
    0b00111111, // 0: a,b,c,d,e,f 
    0b00000110, // 1: b,c 
    0b01011011, // 2: a,b,d,e,g 
    0b01001111, // 3: a,b,c,d,g 
    0b01100110, // 4: b,c,f,g 
    0b01101101, // 5: a,c,d,f,g 
    0b01111101, // 6: a,c,d,e,f,g 
    0b00000111, // 7: a,b,c 
    0b01111111, // 8: a,b,c,d,e,f,g 
    0b01101111  // 9: a,b,c,d,f,g 
}; 

struct termios orig_termios; 

// Flag para indicar que o jogo deve ser encerrado
volatile sig_atomic_t game_should_exit = 0; 

// Handler para o sinal SIGINT (Ctrl+C)
void handle_sigint(int sig) {
    game_should_exit = 1;
}

//============================================== 
// MAIN (Ponto de entrada do programa) 
//============================================== 
int main() { 
    // Registra o handler para o Ctrl+C
    signal(SIGINT, handle_sigint);

    enableRawMode(); 
    init_terminal(); 
    
    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_TIMER) < 0) { 
        fprintf(stderr, "Não foi possível inicializar o SDL: %s\n", SDL_GetError()); 
        return -1; 
    } 
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096) < 0) { 
        fprintf(stderr, "Não foi possível inicializar o SDL_mixer: %s\n", Mix_GetError()); 
        SDL_Quit(); 
        return -1; 
    } 
    Mix_AllocateChannels(16); 
    
    GameState game_state; 
    memset(&game_state, 0, sizeof(GameState)); 
    
    game_state.fd_hardware = init_hardware(); 
    if (game_state.fd_hardware == -1) { 
        fprintf(stderr, "O jogo não pode continuar sem acesso ao hardware. A comunicação com a placa pode estar com problemas.\n"); 
        SDL_Quit(); 
        return -1; 
    } 

    const char *arquivo_musica = "better_than_revenge.mp3"; 
    AudioData *audio_data = load_mp3_file(arquivo_musica); 
    if (!audio_data) { 
        fprintf(stderr, "Erro ao carregar áudio para análise!\n"); 
        finalizar_jogo(&game_state); 
        return -1; 
    } 
    analyze_audio_to_file(audio_data, LEVEL_FILENAME); 
    free_audio_data(audio_data); 

    carregar_nivel(&game_state); 
    inicializar_jogo(&game_state); 
    game_state.joy_fd = init_joystick(&game_state); 
    
    game_state.musica = Mix_LoadMUS(arquivo_musica); 
    if (game_state.musica == NULL) { 
        fprintf(stderr, "Não foi possível carregar a música '%s': %s\n", arquivo_musica, Mix_GetError()); 
        finalizar_jogo(&game_state); 
        return -1; 
    } 

    if (system("clear") != 0) { 
        fprintf(stderr, "Falha ao limpar tela\n"); 
    } 
    printf("=== GUITAR HERO ===\n\n"); 
    for (int i = 3; i > 0; i--) { 
        printf("Começando em: %d\n", i); 
        fflush(stdout); 
        SDL_Delay(1000); 
    } 
    printf("\nJOGUE!\n"); 

    if (Mix_PlayMusic(game_state.musica, 1) == -1) { 
        fprintf(stderr, "Erro ao tocar musica: %s\n", Mix_GetError()); 
    } else { 
        game_state.musica_playing = 1; 
    } 
    game_state.start_time = SDL_GetTicks(); 
    SDL_Delay(50); 
    float tempo_final_do_nivel = game_state.note_count > 0 ?  
        game_state.level_notes[game_state.note_count - 1].timestamp + 2.0f : 5.0f; 

    while (!game_state.game_over && game_state.musica_playing && !game_should_exit) { 
        Uint32 frame_start = SDL_GetTicks(); 
        double tempo_decorrido = (double)(frame_start - game_state.start_time) / 1000.0; 
        if (tempo_decorrido > 0.5f) { 
            process_input(&game_state, tempo_decorrido - 0.5f); 
            update_game(&game_state, tempo_decorrido - 0.5f); 
        } 
        render_game(&game_state, tempo_decorrido); 
        if (!Mix_PlayingMusic()) { 
            game_state.musica_playing = 0; 
        } 
        Uint32 frame_time = SDL_GetTicks() - frame_start; 
        if (frame_time < FRAME_DELAY) { 
            SDL_Delay(FRAME_DELAY - frame_time); 
        } 
        if (tempo_decorrido > tempo_final_do_nivel) { 
            game_state.game_over = 1; 
        } 
    } 
    if (game_state.game_over || game_should_exit) { 
        Mix_HaltMusic(); 
        game_state.musica_playing = 0; 
    } 
    
    // Mostra o estado final do jogo até uma tecla ser pressionada
    while (!kbhit()) { 
        if (game_should_exit) break; // Sai se Ctrl+C foi pressionado
        render_game(&game_state, 0); 
        SDL_Delay(100); 
    } 
    
    finalizar_jogo(&game_state); 
    return 0; 
} 


//============================================== 
// FUNÇÕES DO JOGO (TERMINAL) 
//============================================== 

// Restaura as configurações originais do terminal 
void disableRawMode() { 
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios); 
    printf("\033[?25h"); // Mostra o cursor novamente 
} 

// Habilita o modo "raw" para leitura de teclas instantânea 
void enableRawMode() { 
    tcgetattr(STDIN_FILENO, &orig_termios); 
    atexit(disableRawMode); 
    struct termios raw = orig_termios; 
    raw.c_lflag &= ~(ECHO | ICANON); 
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw); 
} 

// Verifica se uma tecla foi pressionada 
int kbhit(void) { 
    struct timeval tv = {0, 0}; 
    fd_set fds; 
    FD_ZERO(&fds); 
    FD_SET(STDIN_FILENO, &fds); 
    return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0; 
} 

// Inicializa o terminal para o jogo 
void init_terminal() { 
    printf("\033[?25l"); // Esconde cursor 
    printf("\033[2J");   // Limpa tela 
} 

//============================================== 
// FUNÇÕES DE HARDWARE (DE2i-150) 
//============================================== 

// Abre o dispositivo /dev/mydev apenas uma vez 
int init_hardware() { 
    int fd = open("/dev/mydev", O_RDWR); 
    if (fd == -1) { 
        perror("Falha ao abrir /dev/mydev"); 
        return -1; 
    } 
    printf("Dispositivo /dev/mydev aberto com sucesso (fd: %d).\n", fd); 
    
    // Inicializa LEDs e displays para 0 
    uint32_t val = 0; 
    ioctl(fd, WR_GREEN_LEDS); 
    write(fd, &val, sizeof(val)); 
    
    ioctl(fd, WR_RED_LEDS); 
    write(fd, &val, sizeof(val)); 
    
    // Limpa os displays 
    val = 0xFFFFFFFF; 
    ioctl(fd, WR_L_DISPLAY); 
    write(fd, &val, sizeof(val)); 
    
    ioctl(fd, WR_R_DISPLAY); 
    write(fd, &val, sizeof(val)); 
    
    return fd; 
} 

// Fecha o dispositivo e apaga os LEDs/displays 
void close_hardware(int fd) { 
    if (fd != -1) { 
        uint32_t val = 0; 
        
        // Apaga os LEDs 
        ioctl(fd, WR_GREEN_LEDS); 
        write(fd, &val, sizeof(val)); 
        ioctl(fd, WR_RED_LEDS); 
        write(fd, &val, sizeof(val)); 
        
        // Apaga os displays 
        val = 0xFFFFFFFF; 
        ioctl(fd, WR_L_DISPLAY); 
        write(fd, &val, sizeof(val)); 
        ioctl(fd, WR_R_DISPLAY); 
        write(fd, &val, sizeof(val)); 
        
        close(fd); 
    } 
} 

// Atualiza os displays de 7 segmentos com o score e erros 
void update_displays(int fd, int score, int errors) { 
    if (fd == -1) return; 
    
    // --- Lógica Corrigida para a Pontuação (display esquerdo de 4 dígitos) ---
    // A pontuação agora é exibida corretamente, da esquerda para a direita (milhares, centenas, dezenas, unidades).
    uint32_t left_display_val = 0; 
    
    // Dígito dos milhares (maior deslocamento para o display mais à esquerda)
    left_display_val |= ((~seg7_codes[(score / 1000) % 10]) & 0x7F) << 21;

    // Dígito das centenas (deslocamento em 14 bits)
    left_display_val |= ((~seg7_codes[(score / 100) % 10]) & 0x7F) << 14; 
    
    // Dígito das dezenas (deslocamento em 7 bits)
    left_display_val |= ((~seg7_codes[(score / 10) % 10]) & 0x7F) << 7; 
    
    // Dígito das unidades (menor deslocamento para o display mais à direita) 
    left_display_val |= (~seg7_codes[score % 10]) & 0x7F; 
    
    ioctl(fd, WR_L_DISPLAY); 
    write(fd, &left_display_val, sizeof(left_display_val)); 

    // --- Lógica Corrigida para os Erros (display direito de 4 dígitos) ---
    // Os erros agora são exibidos com até quatro casas.
    uint32_t right_display_val = 0; 
    
    // Dígito dos milhares (deslocamento em 21 bits)
    right_display_val |= ((~seg7_codes[(errors / 1000) % 10]) & 0x7F) << 21;

    // Dígito das centenas (deslocamento em 14 bits)
    right_display_val |= ((~seg7_codes[(errors / 100) % 10]) & 0x7F) << 14;

    // Dígito das dezenas (deslocamento em 7 bits)
    right_display_val |= ((~seg7_codes[(errors / 10) % 10]) & 0x7F) << 7; 

    // Dígito das unidades (menor deslocamento)
    right_display_val |= (~seg7_codes[errors % 10]) & 0x7F; 

    ioctl(fd, WR_R_DISPLAY); 
    write(fd, &right_display_val, sizeof(right_display_val)); 
} 

// Pisca os LEDs (verde para acerto, vermelho para erro) 
void flash_led(int fd, int color) { 
    if (fd == -1) return; 

    // Acende todos os 4 LEDs (0b00001111)
    uint32_t leds_on = 0xFFFFFFFF; 
    uint32_t leds_off = 0x00; 

    if (color == 1) { // LED Verde (acerto) 
        ioctl(fd, WR_GREEN_LEDS); 
        write(fd, &leds_on, sizeof(leds_on)); 
        ioctl(fd, WR_RED_LEDS); 
        write(fd, &leds_off, sizeof(leds_off)); 
    } else { // LED Vermelho (erro) 
        ioctl(fd, WR_RED_LEDS); 
        write(fd, &leds_on, sizeof(leds_on)); 
        ioctl(fd, WR_GREEN_LEDS); 
        write(fd, &leds_off, sizeof(leds_off)); 
    } 

    usleep(100000); // Mantém LEDs acesos por 100ms 
    
    // Apaga os LEDs 
    ioctl(fd, WR_GREEN_LEDS); 
    write(fd, &leds_off, sizeof(leds_off)); 
    ioctl(fd, WR_RED_LEDS); 
    write(fd, &leds_off, sizeof(leds_off)); 
} 

// Lê o estado dos push-buttons 
int read_pbuttons(int fd) { 
    if (fd == -1) return 0; 
    
    uint32_t buttons = 0; 
    
    if (ioctl(fd, RD_PBUTTONS) < 0) { 
        perror("ioctl RD_PBUTTONS falhou"); 
        return 0; 
    } 
    
    ssize_t r = read(fd, &buttons, sizeof(buttons)); 
    if (r == sizeof(buttons)) { 
        return (~buttons) & 0xF; 
    } 
    
    return 0; 
} 

//============================================== 
// FUNÇÕES DO JOGO (LÓGICA) 
//============================================== 

int init_joystick(GameState *state) { 
    int joy_fd = open("/dev/input/js0", O_RDONLY | O_NONBLOCK); 
    if (joy_fd == -1) { 
        printf("Joystick não detectado. Usando apenas teclado.\n"); 
    } 
    return joy_fd; 
} 

void inicializar_jogo(GameState *state) { 
    state->score = 0; 
    state->combo = 1; 
    state->consecutive_misses = 0; 
    state->game_over = 0; 
    state->musica_playing = 0; 
    
    for (int i = 0; i < state->note_count; i++) { 
        state->level_notes[i].foi_processada = 0; 
        state->level_notes[i].foi_pressionada = 0; 
    } 

    state->joy_fd = -1; 
    state->fd_pbuttons = -1; 
} 

void carregar_nivel(GameState *state) { 
    FILE *file = fopen(LEVEL_FILENAME, "r"); 
    if (!file) { 
        perror("Não foi possível abrir o arquivo de nível"); 
        exit(1); 
    } 
    state->note_count = 0; 
    float timestamp; 
    char note_name[5]; 
    
    while (fscanf(file, "%f %s", &timestamp, note_name) == 2) { 
        if (state->note_count < MAX_NOTES) { 
            int pista_mapeada = -1; 
            switch (note_name[0]) { 
                case 'C': case 'D': pista_mapeada = 0; break; 
                case 'E': case 'F': pista_mapeada = 1; break; 
                case 'G': case 'A': pista_mapeada = 2; break; 
                case 'B': pista_mapeada = 3; break; 
            } 
            if (pista_mapeada != -1) { 
                state->level_notes[state->note_count].timestamp = timestamp; 
                strcpy(state->level_notes[state->note_count].note_name, note_name); 
                state->level_notes[state->note_count].note_index = pista_mapeada; 
                state->level_notes[state->note_count].foi_processada = 0; 
                state->level_notes[state->note_count].foi_pressionada = 0; 
                state->note_count++; 
            } 
        } 
    } 
    fclose(file); 
} 

void check_hits(GameState *state, int pista, double tempo_decorrido) { 
    int hit = 0; 

    for (int i = 0; i < state->note_count; i++) { 
        if (state->level_notes[i].foi_processada) continue; 
        float timestamp_nota = state->level_notes[i].timestamp; 
        int pista_nota = state->level_notes[i].note_index; 

        if ((pista == pista_nota + 1) &&  
            (tempo_decorrido > timestamp_nota - 0.15 &&  
             tempo_decorrido < timestamp_nota + 0.15)) { 
            printf("\a"); // Beep sonoro 
            state->score += 10 * state->combo; 
            state->combo++; 
            state->consecutive_misses = 0; 
            state->level_notes[i].foi_processada = 1; 
            state->level_notes[i].foi_pressionada = 1; 
            hit = 1; 
            flash_led(state->fd_hardware, 1); // Acende LEDs verdes 
            break; 
        } 
    } 

    if (!hit && pista != 0) { 
        state->consecutive_misses++; 
        state->combo = 1; 
        flash_led(state->fd_hardware, 0); // Acende LEDs vermelhos 

        if (state->consecutive_misses >= MAX_MISSES) { 
            state->game_over = 1; 
            Mix_HaltMusic(); 
            state->musica_playing = 0; 
        } 
    } 
} 

void process_input(GameState *state, double tempo_decorrido) { 
    if (state->game_over || !state->musica_playing || game_should_exit) return; 

    // 1) Teclado 
    if (kbhit()) { 
        char ch = getchar(); 
        // Apenas as teclas '1', '2', '3' e '4' são consideradas entradas válidas
        if (ch >= '1' && ch <= '4') { 
            check_hits(state, ch - '0', tempo_decorrido); 
        } 
        // Qualquer outra tecla é ignorada para evitar erros.
    } 

    // 2) Joystick 
    if (state->joy_fd != -1) { 
        struct js_event e; 
        while (read(state->joy_fd, &e, sizeof(e)) > 0) { 
            if (e.type == JS_EVENT_BUTTON && e.value == 1 && e.number < 4) { 
                check_hits(state, e.number + 1, tempo_decorrido); 
            } 
        } 
    } 
    
    // 3) Push-buttons da placa 
    if (state->fd_hardware != -1) { 
        int buttons = read_pbuttons(state->fd_hardware); 
        for (int i = 0; i < 4; i++) { 
            if (buttons & (1 << i)) { 
                check_hits(state, i + 1, tempo_decorrido); 
            } 
        } 
    } 
} 

void update_game(GameState *state, double tempo_decorrido) { 
    for (int i = 0; i < state->note_count; i++) { 
        if (!state->level_notes[i].foi_processada &&  
            tempo_decorrido > state->level_notes[i].timestamp + 0.15) { 
            state->level_notes[i].foi_processada = 1; 
            if (!state->level_notes[i].foi_pressionada) { 
                state->consecutive_misses++; 
                state->combo = 1; 
                flash_led(state->fd_hardware, 0); // LED vermelho (erro por não apertar a tempo) 
                if (state->consecutive_misses >= MAX_MISSES) { 
                    state->game_over = 1; 
                    Mix_HaltMusic(); 
                    state->musica_playing = 0; 
                } 
            } 
        } 
    } 
    update_displays(state->fd_hardware, state->score, state->consecutive_misses); 
} 

void render_game(GameState *state, double tempo_decorrido) { 
    if (system("clear") != 0) { 
        fprintf(stderr, "Falha ao limpar tela\n"); 
    } 
    char pista_visual[ALTURA_DA_PISTA][5]; 
    for (int i = 0; i < ALTURA_DA_PISTA; i++) { 
        sprintf(pista_visual[i], "    "); 
    } 
    double tempo_ajustado = tempo_decorrido > 0.5f ? tempo_decorrido - 0.5f : 0; 
    for (int i = 0; i < state->note_count; i++) { 
        if (!state->level_notes[i].foi_processada) { 
            float tempo_da_nota = state->level_notes[i].timestamp; 
            float dist_temporal = tempo_da_nota - tempo_ajustado; 
            if (dist_temporal >= 0 && dist_temporal < TEMPO_DE_ANTEVISAO) { 
                int linha = ALTURA_DA_PISTA - 1 - (int)((dist_temporal / TEMPO_DE_ANTEVISAO) * ALTURA_DA_PISTA); 
                if (linha >= 0 && linha < ALTURA_DA_PISTA) { 
                    int pista_da_nota = state->level_notes[i].note_index; 
                    pista_visual[linha][pista_da_nota] = (pista_da_nota + 1) + '0'; 
                } 
            } 
        } 
    } 
    printf(COLOR_GREEN "PISTA 1 " COLOR_RESET "| "  
            COLOR_RED "2 " COLOR_RESET "| "  
            COLOR_BLUE "3 " COLOR_RESET "| "  
            COLOR_YELLOW "4\n" COLOR_RESET); 
    printf("+--------+\n"); 
    for (int i = 0; i < ALTURA_DA_PISTA; i++) { 
        printf("|"); 
        for (int j = 0; j < 4; j++) { 
            char note = pista_visual[i][j]; 
            if (note != ' ') { 
                switch (j) { 
                    case 0: printf(COLOR_GREEN "%c" COLOR_RESET, note); break; 
                    case 1: printf(COLOR_RED "%c" COLOR_RESET, note); break; 
                    case 2: printf(COLOR_BLUE "%c" COLOR_RESET, note); break; 
                    case 3: printf(COLOR_YELLOW "%c" COLOR_RESET, note); break; 
                } 
            } else { 
                printf(" "); 
            } 
            printf("|"); 
        } 
        printf("\n"); 
    } 
    printf("+--------+  <-- ZONA DE ACERTO\n"); 
    printf("Tempo: %.2f s | Pontos: %d | Combo: x%d | Erros: %d/%d\n",  
            tempo_ajustado, state->score, state->combo,  
            state->consecutive_misses, MAX_MISSES); 
    if (state->game_over) { 
        printf("\n\033[31mGAME OVER! Você errou 20 vezes consecutivas.\033[0m\n"); 
        printf("\033[33mPontuação final: %d\033[0m\n", state->score); 
        printf("Pressione qualquer tecla para sair...\n"); 
    } else if (game_should_exit) {
        printf("\n\033[31mJogo encerrado pelo usuário.\033[0m\n");
        printf("Pressione qualquer tecla para sair...\n");
    }
    
    // Certifique-se de que a tela não é atualizada se o jogo já terminou ou foi encerrado
    if (state->game_over || game_should_exit) {
        fflush(stdout);
        // Não faz nada para não piscar a tela
    } else {
        fflush(stdout);
    }
} 

void finalizar_jogo(GameState *state) { 
    if (state->musica_playing) { 
        Mix_HaltMusic(); 
    } 
    close_hardware(state->fd_hardware); 
    if (state->joy_fd != -1) close(state->joy_fd); 
    if (state->musica != NULL) Mix_FreeMusic(state->musica); 
    Mix_CloseAudio(); 
    SDL_Quit(); 
    disableRawMode(); 
} 