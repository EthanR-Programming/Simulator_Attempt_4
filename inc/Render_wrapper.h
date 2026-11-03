#ifndef render_wrapper_h
#define render_wrapper_h

#include "opencl_wrapper.h"
#include "opengl_wrapper.h"
#include "file_loader.h"
#include "text.h"
#include "stdatomic.h"
#include <pthread.h>
#include "mem.h"

#define black_text 0
#define white_text 1
#define red_text 2
#define green_text 3
#define blue_text 4
#define grey_text 5
#define orange_text 6


void render_init();
int tex_add(int x, int y, unsigned short texture_id, float scale, unsigned char depth, unsigned char flags);
int solid_colour_add(unsigned char r, unsigned char g, unsigned char b);
void solid_colour_get_rgb(int id, unsigned char *r, unsigned char *g, unsigned char *b);
int solid_colour_tex_add(int x, int y, int width, int height, unsigned short texture_id, float scale, unsigned char depth, unsigned char flags);
void tex_remove(int translation_index);

// Text info for rendering text
struct text_s {
    int advance[256];
    int yoffset[256];
    int xoffset[256];
    int height[256];

    unsigned int text_begining;
};

struct input_s {

    int mouse_x;
    int mouse_y;

    char mouse_buttons[4];

    char keys[512];

    char update;

    float scroll_wheel;
};

#define scale_to_window     0x1 << 0
#define solid_texture       0x1 << 1
#define use_global_scale    0x1 << 2
#define use_camera_offset   0x1 << 3
#define round_edges         0x1 << 4

#endif