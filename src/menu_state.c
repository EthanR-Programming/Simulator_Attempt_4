#include "menu_state.h"
#include "button.h"
#include "text.h"
#include "Render_wrapper.h"
#include "play_defines.h"

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


int game_name_text_id;

int *rising_edge_tex_ids;
int rising_edge_tex_num;
int rising_edge_colour;

void play_button_callback(){
    game_state = PLAY_STATE;
}

void settings_button_callback(){
    game_state = SETTINGS_STATE;
}

void exit_button_callback(){
    exit(0);
}

void menu_poll(){
    return;
}

void grid_init(){

    if (!grid_background_colour) grid_background_colour = solid_colour_add(19, 21, 26);

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

    if (!rising_edge_colour) rising_edge_colour = solid_colour_add(52, 204, 110);

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
}

void menu_init(){

    grid_init();

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

    if (!menu_background_colour) menu_background_colour = solid_colour_add(32 , 36 , 43);
    menu_background_tex_id = solid_colour_tex_add((1920 >> 1) - (700 >> 1), (1080 >> 1) - (600 >> 1), 700, 700, menu_background_colour, 1.0f, 3, round_edges | scale_to_window);

    if (!menu_background_border_colour) menu_background_border_colour = solid_colour_add(42, 47, 56);
    menu_background_tex_id = solid_colour_tex_add((1920 >> 1) - (706 >> 1), (1080 >> 1) - (606 >> 1), 706, 706, menu_background_border_colour, 1.0f, 2, round_edges | scale_to_window);

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
    grid_remove();
    rising_edge_remove();
}