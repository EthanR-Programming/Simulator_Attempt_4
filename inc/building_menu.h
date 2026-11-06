#ifndef building_menu_h
#define building_menu_h

#include "colours.h"

#define BUILDING_MENU_WIDTH 330

#define MAX_WIRE_COLOURS 6

extern colour_t wire_colours[6];

void building_menu_init();
void building_menu_poll();
void building_menu_destroy();

#endif