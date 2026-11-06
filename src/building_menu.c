#include "building_menu.h"
#include "play_defines.h"
#include "ui.h"
#include "button.h"
#include "grid.h"
#include <math.h>
#include <stdio.h>

// REPLACE THIS WHEN YOU FINISH SETTINGS MENU, THIS IS TEMP FIX CURRENTLY
void set_wire_colours(){

    for (int i = 0; i < 6; i++) wire_colours[i].a = 255;

    wire_colours[0].r = 255;
    wire_colours[0].g = 255;
    wire_colours[0].b = 255;

    // Red
    wire_colours[1].r = 235;
    wire_colours[1].g = 64;
    wire_colours[1].b = 52;

    // Green
    wire_colours[2].r = 72;
    wire_colours[2].g = 199;
    wire_colours[2].b = 116;

    // Blue
    wire_colours[3].r = 58;
    wire_colours[3].g = 125;
    wire_colours[3].b = 240;

    // Yellow
    wire_colours[4].r = 245;
    wire_colours[4].g = 205;
    wire_colours[4].b = 60;

    // Purple
    wire_colours[5].r = 170;
    wire_colours[5].g = 95;
    wire_colours[5].b = 230;
}

static uint building_menu_ui;

static char *component_type_names[] = {"1-bit" , "8-bit" , "16-bit" , "32-bit" , "64-bit" , "IO" , "Custom"};
static int component_type_count = 7;

static uint component_type_selected = 0;

colour_t wire_colours[6];

button_t *prev_button_type_callback;
static void component_type_callback(button_t *button){
    for (int i = 0; i < component_type_count;i++){
        if (!strcmp(button->text ,component_type_names[i])){
            component_type_selected = i;
        }
    }

    if (prev_button_type_callback){
        prev_button_type_callback->clickable = true;
        prev_button_type_callback->colour_id = solid_colour_try(45 , 50 , 59);
        prev_button_type_callback->hover_colour_id = solid_colour_try(57 , 63 , 74);
        button_render(prev_button_type_callback , false);
    }

    prev_button_type_callback = button;

    button->clickable = false;
    button->colour_id = solid_colour_try(58 , 125 , 240);
    button->hover_colour_id = solid_colour_try(58 , 125 , 240);

    button_render(button , false);
}

static void building_menu_add_component_type(const char *string , int x , int y){
    static colour_t comp_type_button_col = {.a = 255, .r = 45 , .g = 50 , .b = 59};
    static colour_t comp_type_button_hover_col = {.a = 255, .r = 57 , .g = 63 , .b = 74}; 
    ui_element_add_button(building_menu_ui , x , y, 75 , 34 , component_type_callback , 0 , string , grey_text , 0.1f , comp_type_button_col , comp_type_button_hover_col,  scale_to_window | round_edges);
}

void wire_colour_option_render_callback(button_t *c , uchar use_hover_colour){

    button_render_remove(c);

    // Outline on hover
    if (use_hover_colour){
        c->tex_ids.tex_ids_ptr[c->tex_ids.tex_id_num++] = solid_colour_tex_add(c->x , c->y , c->width , c->height , c->hover_colour_id , 1.0f , 240 , c->flags);
    }

    // Colour
    c->tex_ids.tex_ids_ptr[c->tex_ids.tex_id_num++] = solid_colour_tex_add(c->x + 4 , c->y + 4 , c->width - 8 , c->height - 8 , c->colour_id , 1.0f , 241 , c->flags);
}

button_t * prev_button_colour_callback;
void wire_colour_option_press_callback(button_t *c){

    if (prev_button_colour_callback){
        prev_button_colour_callback->hover_colour_id = solid_colour_try(120, 130, 120);
        prev_button_colour_callback->render_func_ptr(prev_button_colour_callback, false);
        prev_button_colour_callback->clickable = true;
    }
    prev_button_colour_callback = c;

    c->hover_colour_id = solid_colour_try(240 , 240 , 240);
    c->render_func_ptr(c, true);
    c->clickable = false;
}

// --------------------------------------------------------------- //
//      This code is for misc things on building menu like fps     //
// --------------------------------------------------------------- //

extern int rendering_fps;

static int stat_text_ids[3];

static int stat_last_cell_x;
static int stat_last_cell_y;
static int stat_last_zoom;
static int stat_last_fps;

static inline int stat_floor_div(int a , int b){
    int q = a / b;
    return (a % b != 0 && (a < 0) != (b < 0)) ? q - 1 : q;
}

// Grid cell under the mouse, using the same maths as camera_zoom in play.c
static void stat_mouse_cell(int *cell_x , int *cell_y){
    float scale = global_texture_scale > 0.0f ? global_texture_scale : 1.0f;

    float mouse_x = (float)user_inputs.mouse_x * (float)GRID_VIEW_W / (float)window_width;
    float mouse_y = (float)user_inputs.mouse_y * (float)GRID_VIEW_H / (float)window_height;

    int world_x = (int)floorf(camera_x + mouse_x / scale);
    int world_y = (int)floorf(camera_y + mouse_y / scale);

    *cell_x = stat_floor_div(world_x , GRID_CELL_SIZE);
    *cell_y = stat_floor_div(world_y , GRID_CELL_SIZE);
}

// Rebuilds the three strings, laid out left to right and centred in the menu
static void stat_display_render(int cell_x , int cell_y , int zoom , int fps){

    for (int i = 0; i < 3; i++){
        if (stat_text_ids[i]) ui_element_remove(building_menu_ui , stat_text_ids[i]);
        stat_text_ids[i] = 0;
    }

    char strings[3][32];
    snprintf(strings[0] , sizeof(strings[0]) , "Cell %d, %d" , cell_x , cell_y);
    snprintf(strings[1] , sizeof(strings[1]) , "Zoom %d%%" , zoom);
    snprintf(strings[2] , sizeof(strings[2]) , "FPS %d" , fps);

    int x = (BUILDING_MENU_WIDTH - 100) / 2 - 60;
    if (x < 8) x = 8;

    for (int i = 0; i < 3; i++){
        stat_text_ids[i] = ui_element_add_text(building_menu_ui , strings[i] , grey_text , x - 35 , 30 , 0.08f , 255 , scale_to_window , 2);
        x += 110;
    }

    stat_last_cell_x = cell_x;
    stat_last_cell_y = cell_y;
    stat_last_zoom = zoom;
    stat_last_fps = fps;
}

void stat_display_init(){

    // Old ids are gone after building_menu_destroy clears the UI
    for (int i = 0; i < 3; i++) stat_text_ids[i] = -1;

    int cell_x , cell_y;
    stat_mouse_cell(&cell_x , &cell_y);
    stat_display_render(cell_x , cell_y , (int)roundf(global_texture_scale * 100.0f) , rendering_fps);

}

// Only re-renders the text when one of the values changes
void stat_display_update(){

    int cell_x , cell_y;
    stat_mouse_cell(&cell_x , &cell_y);
    int zoom = (int)roundf(global_texture_scale * 100.0f);
    int fps = rendering_fps;

    if (cell_x == stat_last_cell_x && cell_y == stat_last_cell_y && zoom == stat_last_zoom && fps == stat_last_fps) return;

    stat_display_render(cell_x , cell_y , zoom , fps);
}



// --------------------------------------------------------------- //
//                  building menu init, poll, destroy              //
// --------------------------------------------------------------- //


void building_menu_init(){

    set_wire_colours();

    if (!building_menu_ui) building_menu_ui = ui_create();

    // "Components"
    ui_element_add_text(building_menu_ui, "Components" , white_text , 110 , (1080) - 35, 0.175f , 255 , scale_to_window , true);
    
    // Background
    colour_t building_menu_background_colour = {.a = 255 , .r = 24 , .g = 27, .b = 32};
    ui_element_add_solid_tex(building_menu_ui , 0 , 0 , BUILDING_MENU_WIDTH , 1080 , building_menu_background_colour , scale_to_window , 201); 

    // Outline
    colour_t building_menu_outline_colour = {.a = 255 , .r = 42 , .g = 47, .b = 56};
    ui_element_add_solid_tex(building_menu_ui , BUILDING_MENU_WIDTH , 0 , 3 , 1080 , building_menu_outline_colour , scale_to_window , 200); 

    // Component types
    for (int i = 0; i < component_type_count ; i++){
        building_menu_add_component_type(component_type_names[i], (i % 4) * 80 + 45, 1080 - 100 - ((i >> 2)) * 40); 
    }

    // Divider Lines
    colour_t divider_line_colour = {.a = 255 , .r = 42 , .g = 47, .b = 56};
    ui_element_add_solid_tex(building_menu_ui , 10 , 910 , BUILDING_MENU_WIDTH - 20, 3 , divider_line_colour , scale_to_window , 201); 
    ui_element_add_solid_tex(building_menu_ui , 10 , 310 , BUILDING_MENU_WIDTH - 20, 3 , divider_line_colour , scale_to_window , 201); 
    ui_element_add_solid_tex(building_menu_ui , 10 , 75  , BUILDING_MENU_WIDTH - 20, 3 , divider_line_colour , scale_to_window , 201); 

    // "Wire Colour"
    ui_element_add_text(building_menu_ui, "Wire colour" , grey_text , 60 , 290, 0.090f , 255 , scale_to_window , true);

    // Cell / zoom / fps along the bottom
    stat_display_init();


    colour_t background_wire_select_colour_hover = {.a = 255 , .r = 120 , .g = 130, .b = 120};
    // Wire colours
    for (int i = 0; i < 6; i++){
        ui_element_add_button(building_menu_ui , (i * ((BUILDING_MENU_WIDTH) / 6 - 4)) + 38, 245 , ((BUILDING_MENU_WIDTH) / 6) - 5 , ((BUILDING_MENU_WIDTH) / 6) - 5 , wire_colour_option_press_callback , wire_colour_option_render_callback , 0 , 0 , 0.0f , wire_colours[i] , background_wire_select_colour_hover, scale_to_window | round_edges);
    }

    

}

void building_menu_poll(){
    stat_display_update();

    if (user_inputs.mouse_x > BUILDING_MENU_WIDTH) return;
}

void building_menu_destroy(){
    ui_clear(building_menu_ui);
    prev_button_colour_callback = 0;
    prev_button_type_callback = 0;
}