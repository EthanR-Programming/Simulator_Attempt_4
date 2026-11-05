#ifndef large_textures_h
#define large_textures_h

#include "Render_wrapper.h"

#define large_tex_create(tex_ptr,tex_count,objx,objy,width,height,colour_id,depth,flags) do { \
    int lt_x0 = (objx), lt_y0 = (objy); \
    int lt_x1 = lt_x0 + (width), lt_y1 = lt_y0 + (height); \
    int lt_i = 0; \
    (tex_ptr) = mem_malloc(sizeof(int) * ((width) / 64 + 1) * ((height) / 64 + 1)); \
    for(int lt_x = lt_x0; lt_x < lt_x1; lt_x += 64){ \
        for(int lt_y = lt_y0; lt_y < lt_y1; lt_y += 64){ \
            (tex_ptr)[lt_i++] = solid_colour_tex_add(lt_x, lt_y, \
                (lt_x1 - lt_x > 64) ? 64 : lt_x1 - lt_x, \
                (lt_y1 - lt_y > 64) ? 64 : lt_y1 - lt_y, \
                (colour_id), \
                1.0f, \
                (depth), \
                (flags) \
            ); \
        } \
    } \
    (tex_count) = lt_i; \
} while (0)

#endif
