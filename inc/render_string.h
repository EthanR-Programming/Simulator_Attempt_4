#ifndef render_string_h
#define render_string_h

#include "stdio.h"
#include "Render_wrapper.h"
#include "math.h"

long long string_render(char *string , char text_type, int x, int y , float scale, unsigned char depth, char flags, char draw_from_center);
void string_remove(long long ID);
int string_get_draw_width(char *string , int text_type, float scale);

#endif