#include "Render_wrapper.h"
#include "render_string.h"
#include <unistd.h>

extern volatile char rendering_init;
extern volatile char window_active;
extern volatile unsigned int target_fps;
extern volatile unsigned char window_background_r;
extern volatile unsigned char window_background_g;
extern volatile unsigned char window_background_b;
extern volatile struct input_s user_inputs;

int main(){

    render_init();
    while(!rendering_init) usleep(1000);

    target_fps = 60;
    window_background_r = 30; window_background_g = 32; window_background_b = 38;

    int panel = solid_colour_add(60, 64, 80);
    solid_colour_tex_add(100, 100, 400, 200, panel, 1, 10, 0);
    string_render("Rising Edge renderer", 1, 300, 220, 0.25f, 20, 0, 1);

    while(window_active){
        user_inputs.update = 1;
        usleep(1000000 / 120);
    }
    return 0;
}
