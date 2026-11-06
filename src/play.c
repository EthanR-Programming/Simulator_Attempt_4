#include "play.h"
#include "grid.h"
#include "play_defines.h"
#include "building_menu.h"

enum play_ui_s {
    SAVE,
    LOAD,
    CONSTANT,
    RAM,
    MONITOR,
    BREAK_POINT,
    INPUT,
    OUTPUT,
    LABEL,
};
uint current_ui_open;

char check_esc(){
    if (user_inputs.keys[GLFW_KEY_ESCAPE]){
        if (!current_ui_open) game_state = MENU_STATE;
        return 1;
    }
    return 0;
}

char mouse_middle_prev_state;

int mouse_x_middle_mouse;
int mouse_y_middle_mouse;

void camera_movement(){

    // middle mouse button dragging the camera
    if (user_inputs.mouse_buttons[GLFW_MOUSE_BUTTON_MIDDLE] && !mouse_middle_prev_state){

        mouse_x_middle_mouse = user_inputs.mouse_x;
        mouse_y_middle_mouse = user_inputs.mouse_y;

        mouse_middle_prev_state = 1;
    }
    if (!user_inputs.mouse_buttons[GLFW_MOUSE_BUTTON_MIDDLE]){
        mouse_middle_prev_state = 0;
    }
    if (mouse_middle_prev_state){
        // convert window pixels into the 1920x1080 layout space (same as camera_zoom)
        float dx = (float)(user_inputs.mouse_x - mouse_x_middle_mouse) * (float)GRID_VIEW_W / (float)window_width;
        float dy = (float)(user_inputs.mouse_y - mouse_y_middle_mouse) * (float)GRID_VIEW_H / (float)window_height;

        camera_x -= dx / global_texture_scale;
        camera_y -= dy / global_texture_scale;

        mouse_x_middle_mouse = user_inputs.mouse_x;
        mouse_y_middle_mouse = user_inputs.mouse_y;
    }

    if (user_inputs.keys[GLFW_KEY_A]){
        camera_x -= (8.0f) / global_texture_scale;
    }
    if (user_inputs.keys[GLFW_KEY_D]){
        camera_x += (8.0f) / global_texture_scale;
    }
    if (user_inputs.keys[GLFW_KEY_W]){
        camera_y += (8.0f) / global_texture_scale;
    }
    if (user_inputs.keys[GLFW_KEY_S]){
        camera_y -= (8.0f) / global_texture_scale;
    }
    
}

void camera_zoom(){

    if (user_inputs.scroll_wheel == 0.0f) return;

    // canvas sprites use scale_to_window, so convert the cursor from window
    // pixels into the 1920x1080 layout space before applying the zoom
    float mouse_x = (float)user_inputs.mouse_x * (float)GRID_VIEW_W / (float)window_width;
    float mouse_y = (float)user_inputs.mouse_y * (float)GRID_VIEW_H / (float)window_height;

    // world position under the cursor before zooming
    float mouse_x_scaled = camera_x + mouse_x / global_texture_scale;
    float mouse_y_scaled = camera_y + mouse_y / global_texture_scale;

    global_texture_scale += user_inputs.scroll_wheel / 15.0f;

    if (global_texture_scale < 0.08){
        global_texture_scale = 0.08f;
    }
    if (global_texture_scale > 1.25){
        global_texture_scale = 1.25f;
    }

    // world position under the cursor after zooming
    float mouse_x_scaled_after = camera_x + mouse_x / global_texture_scale;
    float mouse_y_scaled_after = camera_y + mouse_y / global_texture_scale;

    // shift the camera so the same point stays under the cursor
    camera_x -= (mouse_x_scaled_after - mouse_x_scaled);
    camera_y -= (mouse_y_scaled_after - mouse_y_scaled);

    user_inputs.scroll_wheel = 0;
}

void play_poll(){

    // If user pressed esc, then we either close in game menu or go to menu
    if (check_esc()){
        if (!current_ui_open) return;
        current_ui_open = 0;
    }

    if (!current_ui_open){
        camera_movement();
        if ((int)((float)user_inputs.mouse_x * (1920.0f / (float)window_width)) > BUILDING_MENU_WIDTH) camera_zoom();
    }

    building_menu_poll();

    // rebuild the grid lines if the camera moved a chunk or the zoom changed
    game_grid_update();
}

void play_init(){
    game_grid_init();
    building_menu_init();
    target_fps = 360;
    global_texture_scale = 1.0f;
}

void play_destroy(){
    game_grid_destroy();
    building_menu_destroy();
}