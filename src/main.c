#include "Render_wrapper.h"
#include "render_string.h"
#include "logic_thread.h"
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

    logic_thread_init();

    return 0;
}
