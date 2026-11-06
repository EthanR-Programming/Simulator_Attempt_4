#ifndef ui_h
#define ui_h

#include "render_string.h"
#include "Render_wrapper.h"
#include "button.h"
#include "large_textures.h"
#include "colours.h"

// Create a UI. Returns its index.
int ui_create();

// Add elements. Each returns the element index, or -1 if ui_index doesn't exist.
int ui_element_add_solid_tex(int ui_index, int x, int y, int width, int height, colour_t colour, uchar flags, uchar depth);
int ui_element_add_text(int ui_index, const char *string, char text_type, int x, int y, float scale, uchar depth, char flags, char draw_from_center);
int ui_element_add_tex(int ui_index, int x, int y, ushort texture_id, float scale, uchar depth, uchar flags);
int ui_element_add_button(int ui_index, int x, int y, int width, int height,
                          void (*on_press_func_ptr)(button_t *c), void (*render_function)(button_t *c , uchar hover),
                          const char *text_str, uchar text_type, float text_scale,
                          colour_t colour, colour_t hover_colour, uchar flags);

button_t *ui_element_get_button(int ui_index, int element_index);

// These return 0 on success, -1 if the UI or element wasn't found
int ui_element_remove(int ui_index, int element_index);
int ui_element_move(int ui_index, int element_index, int x, int y);
int ui_clear(int ui_index);
int ui_destroy(int ui_index);

#endif