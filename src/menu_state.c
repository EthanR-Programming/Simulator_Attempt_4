#include "menu_state.h"
#include "button.h"
#include "text.h"
#include "Render_wrapper.h"
#include "play_defines.h"
#include <stdlib.h>
#include <time.h>

extern volatile uchar window_background_r;
extern volatile uchar window_background_g;
extern volatile uchar window_background_b;

static button_t *play_button;
static button_t *settings_button;
static button_t *exit_button;

extern volatile uint game_state;

int *grid_background_ids;
int grid_background_num;
int grid_background_colour;

int menu_background_colour;
int menu_background_tex_id;
int menu_background_border_colour;
int menu_background_border_tex_id;

extern volatile uint target_fps; 

long long game_name_text_id;

int *rising_edge_tex_ids;
int rising_edge_tex_num;
int rising_edge_colour;

void play_button_callback(button_t *b){
    game_state = PLAY_STATE;
}

void settings_button_callback(button_t *b){
    game_state = SETTINGS_STATE;
}

void exit_button_callback(button_t *b){
    exit(0);
}

// Background animation

#define GRID_COLS 30            // 1920 / 64
#define GRID_ROWS 17            // 1080 / 64, rounded up
#define GLOW_LEVELS 16          // number of fade colours
#define GLOW_MAX_BLEND 0.35f    // brightest tile = 35% of the way to green
#define GLOW_FADE_SECONDS 0.6f
#define PULSE_STEP_SECONDS 0.1f
#define PULSE_SPAWN_SECONDS 0.5f
#define PULSE_TURN_CHANCE 10    // percent chance to turn 90 degrees per step
#define MAX_PULSES 5

int glow_colours[GLOW_LEVELS];
int glow_colours_made;

float tile_glow[GRID_COLS * GRID_ROWS];
unsigned char tile_level[GRID_COLS * GRID_ROWS];

typedef struct {
    int active;
    int col, row;
    int dx, dy;
    float step_timer;
} pulse_t;

pulse_t pulses[MAX_PULSES];
float spawn_timer;
struct timespec last_poll_time;

void glow_colours_make(){
    if (glow_colours_made) return;

    unsigned char br, bg, bb;
    solid_colour_get_rgb(grid_background_colour, &br, &bg, &bb);

    // Logo green
    const float gr = 52, gg = 224, gb = 110;

    for (int i = 0; i < GLOW_LEVELS; i++){
        float t = GLOW_MAX_BLEND * (float)i / (GLOW_LEVELS - 1);
        glow_colours[i] = solid_colour_add(
            (unsigned char)(br + (gr - br) * t),
            (unsigned char)(bg + (gg - bg) * t),
            (unsigned char)(bb + (gb - bb) * t));
    }
    glow_colours_made = 1;
}

void tile_set_level(int col, int row, int level){
    int i = col * GRID_ROWS + row;     // same order grid_init adds tiles
    if (tile_level[i] == level) return;
    tile_level[i] = level;

    tex_remove(grid_background_ids[i]);
    grid_background_ids[i] = solid_colour_tex_add(col * 64, row * 64, 60, 60, glow_colours[level], 1.0f, 1, scale_to_window);
}

void pulse_spawn(){
    pulse_t *p = 0;
    for (int i = 0; i < MAX_PULSES; i++){
        if (!pulses[i].active){ p = &pulses[i]; break; }
    }
    if (!p) return;

    // Start on a random edge, heading inwards
    switch (rand() % 4){
        case 0: p->col = 0;             p->row = rand() % GRID_ROWS; p->dx = 1;  p->dy = 0;  break;
        case 1: p->col = GRID_COLS - 1; p->row = rand() % GRID_ROWS; p->dx = -1; p->dy = 0;  break;
        case 2: p->row = 0;             p->col = rand() % GRID_COLS; p->dx = 0;  p->dy = 1;  break;
        default:p->row = GRID_ROWS - 1; p->col = rand() % GRID_COLS; p->dx = 0;  p->dy = -1; break;
    }
    p->step_timer = 0;
    p->active = 1;
    tile_glow[p->col * GRID_ROWS + p->row] = 1.0f;
}

void pulse_step(pulse_t *p){
    if (rand() % 100 < PULSE_TURN_CHANCE){
        // Turn left or right
        int tmp = p->dx;
        if (rand() & 1){ p->dx = -p->dy; p->dy = tmp; }
        else           { p->dx = p->dy;  p->dy = -tmp; }
    }

    p->col += p->dx;
    p->row += p->dy;

    if (p->col < 0 || p->col >= GRID_COLS || p->row < 0 || p->row >= GRID_ROWS){
        p->active = 0;
        return;
    }
    tile_glow[p->col * GRID_ROWS + p->row] = 1.0f;
}

void pulses_reset(){
    for (int i = 0; i < MAX_PULSES; i++) pulses[i].active = 0;
    for (int i = 0; i < GRID_COLS * GRID_ROWS; i++){
        tile_glow[i] = 0;
        tile_level[i] = 0;
    }
    spawn_timer = 0;
    clock_gettime(CLOCK_MONOTONIC, &last_poll_time);
}

void menu_poll(){
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    float dt = (now.tv_sec - last_poll_time.tv_sec) + (now.tv_nsec - last_poll_time.tv_nsec) / 1e9f;
    last_poll_time = now;
    if (dt > 0.1f) dt = 0.1f;   // don't jump after a stall

    spawn_timer -= dt;
    if (spawn_timer <= 0){
        pulse_spawn();
        spawn_timer = PULSE_SPAWN_SECONDS * (0.5f + (rand() % 100) / 100.0f);
    }

    for (int i = 0; i < MAX_PULSES; i++){
        pulse_t *p = &pulses[i];
        if (!p->active) continue;
        p->step_timer += dt;
        while (p->active && p->step_timer >= PULSE_STEP_SECONDS){
            p->step_timer -= PULSE_STEP_SECONDS;
            pulse_step(p);
        }
    }

    for (int col = 0; col < GRID_COLS; col++){
        for (int row = 0; row < GRID_ROWS; row++){
            float *g = &tile_glow[col * GRID_ROWS + row];
            if (*g <= 0 && tile_level[col * GRID_ROWS + row] == 0) continue;

            *g -= dt / GLOW_FADE_SECONDS;
            if (*g < 0) *g = 0;

            // Squared so the trail drops off quickly behind the head
            tile_set_level(col, row, (int)(*g * *g * (GLOW_LEVELS - 1) + 0.5f));
        }
    }
}

void grid_init(){

    grid_background_colour = solid_colour_try(19, 21, 26);

    grid_background_ids = mem_malloc(511 * sizeof(int));

    for (int x = 0; x < 1920; x+= 64){
        for (int y = 0; y < 1080; y+= 64){
            grid_background_ids[grid_background_num++] = solid_colour_tex_add(x, y, 60, 60, grid_background_colour, 1.0f, 1, scale_to_window);
        }
    }
}

void grid_remove(){

    for (int i = 0; i < grid_background_num; i++){
        tex_remove(grid_background_ids[i]);
    }
    grid_background_num = 0;
    free(grid_background_ids);
    grid_background_ids = 0;
}


void rising_edge_add(){
    rising_edge_tex_ids = mem_malloc(sizeof(int) * 10);

    rising_edge_colour = solid_colour_try(52, 224, 110);

    rising_edge_tex_ids[rising_edge_tex_num++] = solid_colour_tex_add((1920 >> 1) - 250, (1080 >> 1) + 150 + 25, 150, 5, rising_edge_colour, 1.0f, 255, scale_to_window);

    rising_edge_tex_ids[rising_edge_tex_num++] = solid_colour_tex_add((1920 >> 1) - 100, (1080 >> 1) + 150 + 25, 5, 50, rising_edge_colour, 1.0f, 255, scale_to_window);

    rising_edge_tex_ids[rising_edge_tex_num++] = solid_colour_tex_add((1920 >> 1) - 100, (1080 >> 1) + 200 + 25, 205, 5, rising_edge_colour, 1.0f, 255, scale_to_window);

    rising_edge_tex_ids[rising_edge_tex_num++] = solid_colour_tex_add((1920 >> 1) + 100, (1080 >> 1) + 150 + 25, 5, 55, rising_edge_colour, 1.0f, 255, scale_to_window);

    rising_edge_tex_ids[rising_edge_tex_num++] = solid_colour_tex_add((1920 >> 1) + 100, (1080 >> 1) + 150 + 25, 150, 5, rising_edge_colour, 1.0f, 255, scale_to_window);
}

void rising_edge_remove(){
    for (int i = 0; i < rising_edge_tex_num; i++){
        tex_remove(rising_edge_tex_ids[i]);
    }
    rising_edge_tex_num = 0;

    free(rising_edge_tex_ids);
    rising_edge_tex_ids = 0;
}

void menu_init(){

    grid_init();
    glow_colours_make();
    srand((unsigned)time(NULL));
    pulses_reset();

    target_fps = 60;

    window_background_r = 27;
    window_background_g = 30;
    window_background_b = 36;    

    colour_t play_colour = {
        .a = 255,
        .r = 58,
        .g = 125,
        .b = 240
    };

    colour_t play_colour_hover = {
        .a = 255,
        .r = 84,
        .g = 146,
        .b = 255
    };

    colour_t settings_colour = {
        .a = 255,
        .r = 45,
        .g = 50,
        .b = 59
    };

    colour_t settings_colour_hover = {
        .a = 255,
        .r = 57,
        .g = 63,
        .b = 74
    };

    colour_t exit_colour = {
        .a = 255,
        .r = 45,
        .g = 50,
        .b = 59
    };

    colour_t exit_colour_hover = {
        .a = 255,
        .r = 226,
        .g = 88,
        .b = 88
    };

    play_button = button_add((1920 >> 1), 550, 550, 80, play_button_callback, 0x0, "Play", grey_text, 0.25f , play_colour , play_colour_hover , round_edges | scale_to_window);
    settings_button = button_add((1920 >> 1), 450, 550, 80, settings_button_callback, 0x0, "Settings", grey_text, 0.25f , settings_colour , settings_colour_hover , round_edges | scale_to_window);
    exit_button = button_add((1920 >> 1), 350, 550, 80, exit_button_callback, 0x0, "Exit", grey_text, 0.25f , exit_colour , exit_colour_hover , round_edges | scale_to_window);

    menu_background_colour = solid_colour_try(32 , 36 , 43);
    menu_background_tex_id = solid_colour_tex_add((1920 >> 1) - (700 >> 1), (1080 >> 1) - (600 >> 1), 700, 700, menu_background_colour, 1.0f, 3, round_edges | scale_to_window);

    menu_background_border_colour = solid_colour_try(42, 47, 56);
    menu_background_border_tex_id = solid_colour_tex_add((1920 >> 1) - (706 >> 1), (1080 >> 1) - (606 >> 1), 706, 706, menu_background_border_colour, 1.0f, 2, round_edges | scale_to_window);

    game_name_text_id = string_render("Rising Edge", white_text, 1920 >> 1, 850, 0.50f, 255, scale_to_window, true);

    rising_edge_add();
}

void menu_remove(){
    button_remove(play_button);
    button_remove(settings_button);
    button_remove(exit_button);
    play_button = 0;
    settings_button = 0;
    exit_button = 0;
    tex_remove(menu_background_tex_id);
    tex_remove(menu_background_border_tex_id);
    string_remove(game_name_text_id);
    grid_remove();
    rising_edge_remove();
}