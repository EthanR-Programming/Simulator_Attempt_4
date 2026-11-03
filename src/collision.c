#include "collision.h"
#include "Render_wrapper.h"

extern volatile struct input_s user_inputs;
extern volatile uint window_width;
extern volatile uint window_height;


// checks if an object collides with cursor
char cursor_on_texture(int object_x , int object_y, int object_width , int object_height , float scale , unsigned char flags){

    float scale_x = scale;
    float scale_y = scale;

    float mouse_x = (float)user_inputs.mouse_x;
    float mouse_y = (float)user_inputs.mouse_y;

    if ((flags & scale_to_window) != 0){
        mouse_x /= ((float)window_width / (float)1920.0f);
        mouse_y /= ((float)window_height / (float)1080.0f);
    }

    float LX = (float)object_x; 
    float RX = (float)object_x + (float)((float)object_width * (float)scale);

    float BY = (float)object_y;
    float TY = (float)object_y + (float)((float)object_height * (float)scale);

    if (mouse_x > LX && mouse_x < RX && mouse_y > BY && mouse_y < TY){
        return 1;
    }
    return 0;

}