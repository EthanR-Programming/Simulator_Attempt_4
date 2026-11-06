#ifndef play_defines_h
#define play_defines_h

#include "Render_wrapper.h"

enum game_states {
    MENU_STATE,
    PLAY_STATE,
    SETTINGS_STATE
};

extern volatile float global_texture_scale;
extern volatile struct input_s user_inputs;
extern volatile uint game_state;
extern volatile float camera_x;
extern volatile float camera_y;
extern volatile uint window_width;
extern volatile uint window_height;
extern volatile uint target_fps;

#endif