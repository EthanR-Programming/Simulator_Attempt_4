#ifndef grid_h
#define grid_h

#include "Render_wrapper.h"
#include "colours.h"
#include "types.h"

#define GRID_CELL_SIZE      64      // world units between minor lines
#define GRID_MAJOR_EVERY    8       // a major line every N cells
#define GRID_MINOR_WIDTH    1       // line thickness in world units
#define GRID_MAJOR_WIDTH    2
#define GRID_MIN_SPACING_PX 8.0f    // hide a line set when closer than this on screen
#define GRID_DEPTH          1       // minor depth, major lines use depth + 1

// Logical view size (matches the 1920x1080 layout used elsewhere)
#define GRID_VIEW_W 1920
#define GRID_VIEW_H 1080

void game_grid_init();

void game_grid_update();

void game_grid_dirty();

void game_grid_set_colours(uchar minor_r, uchar minor_g, uchar minor_b, uchar major_r, uchar major_g, uchar major_b);

void game_grid_destroy();

#endif
