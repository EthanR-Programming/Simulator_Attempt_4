#ifndef button_h
#define button_h

#include "tex_info.h"
#include "memory.h"
#include "types.h"
#include "Render_wrapper.h"
#include "render_string.h"

typedef struct button_s{

    uchar clickable;

    // normal colour
    colour_t colour;
    int colour_id;

    uint backgound_tex_index;

    // hover colour
    colour_t hover_colour;
    int hover_colour_id;
    uchar currently_hovered;

    char * text;
    uchar text_type;
    float text_scale;

    int x;
    int y;

    int width;
    int height;

    void (*function_ptr)(struct button_s *c);

    void (*render_func_ptr)(uchar hover);

    uchar flags;

    tex_ids_t tex_ids;

    struct button_s *next;
} button_t;

button_t *button_add(
    int x, int y, 
    int width, int height, 
    void (*on_press_func_ptr), 
    void (*render_function), 
    const char *text , uchar text_type, float text_scale,
    colour_t colour, 
    colour_t hover_colour, 
    uchar flags
);

void button_render_remove(button_t *c);
void button_render(button_t *c , uchar use_hover_colour);
void button_poll();

#endif