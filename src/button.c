#include "button.h"
#include "collision.h"
#include <string.h>

extern volatile struct input_s user_inputs;

button_t *button_linked_list;

// Left mouse state from the last button_poll, so a click only fires once
static char last_left_mouse;

button_t *button_add_ll_element(){
    if (!button_linked_list){
        button_linked_list = mem_malloc(sizeof(button_t));
        return button_linked_list;
    }

    button_t *c = button_linked_list;

    while(c->next) c = c -> next;

    c->next = mem_malloc(sizeof(button_t));
    return c->next;
}

// draws or redraws the background in the normal or hover colour.
// transparent colours draw nothing.
static void button_background_draw(button_t *c, uchar hover){
    if (c->background_tex_id != -1){
        tex_remove(c->background_tex_id);
        c->background_tex_id = -1;
    }

    colour_t colour = hover ? c->hover_colour : c->colour;
    if (colour.a == 0x0) return;

    int colour_id = hover ? c->hover_colour_id : c->colour_id;
    c->background_tex_id = solid_colour_tex_add(c->x , c->y , c->width , c->height , colour_id , 1.0f , 240 , c->flags);
}

void button_render_remove(button_t *c){

    if (c->background_tex_id != -1){
        tex_remove(c->background_tex_id);
        c->background_tex_id = -1;
    }

    for (int i = 0;i < c->tex_ids.tex_id_num;i++){
        tex_remove(c->tex_ids.tex_ids_ptr[i]);
    }
    for (int i = 0;i < c->tex_ids.str_id_num;i++){
        string_remove(c->tex_ids.str_ids_ptr[i]);
    }
    c->tex_ids.tex_id_num = 0;
    c->tex_ids.str_id_num = 0;
}

void button_render(button_t *c , uchar use_hover_colour){

    // Clear anything already drawn so repeated renders don't pile up
    button_render_remove(c);

    button_background_draw(c, use_hover_colour);

    if (c->text && c->text[0] != '\0' && c->tex_ids.str_id_num < BUTTON_MAX_IDS){

        int mid_x = c->x + (c->width / 2);
        int mid_y = c->y + (c->height / 2);

        float scale = c->text_scale;

        if (c->text_scale == 0.0f){
            int string_length = string_get_draw_width(c->text , c->text_type , 1.0f);
            if (string_length > 0) scale = (float)(c->width - 10) / (float)string_length;
        }

        c->tex_ids.str_ids_ptr[c->tex_ids.str_id_num++] = string_render(c->text , c->text_type , mid_x , mid_y , scale , 255 , c->flags & ~(round_edges) , 1);
    }
}

// -1 on failure
char button_remove(
    button_t *button
){
    if (button == NULL){
        return -1;
    }

    button_t *c = button_linked_list;
    button_t *prev = 0;

    while(c && c != button) {
        prev = c;
        c = c -> next;
    }
    if (!c) return -1;

    if (prev) prev->next = c->next;
    else button_linked_list = c->next;

    button_render_remove(button);  // take its textures/strings off screen
    free(button->tex_ids.tex_ids_ptr);
    free(button->tex_ids.str_ids_ptr);
    free(button->text);
    free(button);

    return 0;
}

// optional on_press_func_ptr and render_function and text, 0 ALPHA for no colour
button_t *button_add(
    int x, int y,
    int width, int height,
    void (*on_press_func_ptr)(button_t *c),
    void (*render_function)(button_t *c , uchar hover),
    const char *text , uchar text_type, float text_scale,
    colour_t colour,
    colour_t hover_colour,
    uchar flags
){
    button_t* c = button_add_ll_element();

    c->background_tex_id = -1;
    c->x = x - (width >> 1);
    c->y = y - (height >> 1);
    c->width = width;
    c->height = height;
    c->function_ptr = on_press_func_ptr;
    c->colour = colour;
    c->render_func_ptr = render_function;
    c->flags = flags;
    c->text_type = text_type;
    c->text_scale = text_scale;
    c->hover_colour = hover_colour;
    c->clickable = 1;

    // Keep our own copy so the caller's string can go away
    if (text){
        c->text = mem_malloc(strlen(text) + 1);
        strcpy(c->text, text);
    }

    // Both colours are made up front, whichever one is visible
    if (colour.a != 0x0) c->colour_id = solid_colour_try(colour.r , colour.g , colour.b);
    if (hover_colour.a != 0x0) c->hover_colour_id = solid_colour_try(hover_colour.r , hover_colour.g , hover_colour.b);

    c->tex_ids.tex_ids_ptr = mem_malloc(sizeof(int) * BUTTON_MAX_IDS);
    c->tex_ids.str_ids_ptr = mem_malloc(sizeof(long long) * BUTTON_MAX_IDS);

    if (!c->render_func_ptr) button_render(c , false);
    else if (c->render_func_ptr) c->render_func_ptr(c , false);

    return c;
}

void button_poll(){

    char left_mouse = user_inputs.mouse_buttons[GLFW_MOUSE_BUTTON_LEFT];
    char clicked = left_mouse && !last_left_mouse;   // only on the frame it's pressed
    last_left_mouse = left_mouse;

    button_t *c = button_linked_list;

    while(c){

        uchar hovered = cursor_on_texture(c->x , c->y , c->width , c->height , 1.0f , c->flags) ? 1 : 0;

        if (hovered != c->currently_hovered){
            c->currently_hovered = hovered;

            if (c->render_func_ptr && c->clickable) (c->render_func_ptr)(c ,hovered);
            else if (c->clickable) button_background_draw(c, hovered);
        }

        if (hovered && clicked && c->clickable && c->function_ptr){
            (c->function_ptr)(c);
            // return as this may remove the current button or other buttons
            return;
        }

        c = c -> next;
    }
}
