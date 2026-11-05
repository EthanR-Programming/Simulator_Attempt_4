#include "grid.h"
#include <math.h>
#include <stdlib.h>

extern volatile float camera_x;
extern volatile float camera_y;
extern volatile float global_texture_scale;

#define GRID_FLAGS (scale_to_window | use_global_scale | use_camera_offset)

// Lines are drawn one chunk (one major cell) longer than the view and snapped
// to chunk boundaries, so the camera can move a whole chunk before a rebuild.
#define GRID_CHUNK (GRID_CELL_SIZE * GRID_MAJOR_EVERY)

static int *line_ids;
static int line_num;
static int line_max;

static int minor_colour;
static int major_colour;

static int  last_chunk_x;
static int  last_chunk_y;
static float last_scale;
static char built;
static char active;

static inline int floor_div(int a, int b){
    int q = a / b;
    return (a % b != 0 && (a < 0) != (b < 0)) ? q - 1 : q;
}

static void lines_clear(){
    for (int i = 0; i < line_num; i++) tex_remove(line_ids[i]);
    line_num = 0;
}

static inline void line_add(int x, int y, int w, int h, int colour, uchar depth){
    if (line_num == line_max){
        line_max = line_max ? line_max * 2 : 256;
        line_ids = realloc(line_ids, sizeof(int) * line_max);
    }
    line_ids[line_num++] = solid_colour_tex_add(x, y, w, h, colour, 1.0f, depth, GRID_FLAGS);
}

static void grid_build(int chunk_x, int chunk_y, float scale){

    lines_clear();

    const float spacing_px = GRID_CELL_SIZE * scale;
    const char draw_minor = spacing_px >= GRID_MIN_SPACING_PX;
    const char draw_major = spacing_px * GRID_MAJOR_EVERY >= GRID_MIN_SPACING_PX;
    if (!draw_major) return;

    // World area covered: view plus one chunk of slack
    const int x0 = chunk_x * GRID_CHUNK;
    const int y0 = chunk_y * GRID_CHUNK;
    const int w  = (int)ceilf(GRID_VIEW_W / scale) + GRID_CHUNK;
    const int h  = (int)ceilf(GRID_VIEW_H / scale) + GRID_CHUNK;

    const int cols = w / GRID_CELL_SIZE + 1;
    const int rows = h / GRID_CELL_SIZE + 1;

    // Vertical lines (x0 is a multiple of the chunk, so i % MAJOR finds majors)
    for (int i = 0; i <= cols; i++){
        const int major = (i % GRID_MAJOR_EVERY) == 0;
        if (!major && !draw_minor) continue;
        const int lw = major ? GRID_MAJOR_WIDTH : GRID_MINOR_WIDTH;
        line_add(x0 + i * GRID_CELL_SIZE - lw / 2, y0, lw, h,
                 major ? major_colour : minor_colour, GRID_DEPTH + major);
    }

    // Horizontal lines
    for (int i = 0; i <= rows; i++){
        const int major = (i % GRID_MAJOR_EVERY) == 0;
        if (!major && !draw_minor) continue;
        const int lw = major ? GRID_MAJOR_WIDTH : GRID_MINOR_WIDTH;
        line_add(x0, y0 + i * GRID_CELL_SIZE - lw / 2, w, lw,
                 major ? major_colour : minor_colour, GRID_DEPTH + major);
    }
}

void game_grid_set_colours(uchar minor_r, uchar minor_g, uchar minor_b,
                           uchar major_r, uchar major_g, uchar major_b){
    minor_colour = solid_colour_try(minor_r, minor_g, minor_b);
    major_colour = solid_colour_try(major_r, major_g, major_b);
    built = 0;
}

void game_grid_init(){
    if (!minor_colour && !major_colour) game_grid_set_colours(34, 37, 45, 48, 52, 63);

    // Nothing else sets the zoom yet, and the kernel skips every
    // use_global_scale sprite while it's 0
    if (global_texture_scale <= 0.0f) global_texture_scale = 1.0f;
    active = 1;
    built = 0;
    game_grid_update();
}

void game_grid_dirty(){
    built = 0;
}

void game_grid_update(){
    if (!active) return;

    const float scale = global_texture_scale > 0.0f ? global_texture_scale : 1.0f;
    const int cx = floor_div((int)ceilf(camera_x), GRID_CHUNK);
    const int cy = floor_div((int)ceilf(camera_y), GRID_CHUNK);

    if (built && cx == last_chunk_x && cy == last_chunk_y && scale == last_scale) return;

    grid_build(cx, cy, scale);
    last_chunk_x = cx;
    last_chunk_y = cy;
    last_scale = scale;
    built = 1;
}

void game_grid_destroy(){
    lines_clear();
    free(line_ids);
    line_ids = 0;
    line_max = 0;
    active = 0;
    built = 0;
}
