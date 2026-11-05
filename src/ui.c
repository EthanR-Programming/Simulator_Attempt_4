#include "ui.h"
#include "large_textures.h"
#include <string.h>
#include <stdlib.h>

struct large_tex_s {int *tex_ptr; int tex_num; int x; int y; int width; int height; int colour_id; uchar flags; uchar depth;};
struct small_tex_s {int tex_id; int x; int y; int width; int height; int colour_id; uchar flags; uchar depth;};
struct string_s {long long string_id; char *string; char text_type; int x; int y; float scale; uchar depth; char flags; char draw_from_center;};
struct normal_tex_s {int tex_id; ushort texture_id; int x; int y; float scale; uchar depth; uchar flags;};

struct ui_element_s{
    uint element_index;
    uchar element_type;
    void *element_ptr;
    struct ui_element_s *next;
};

enum element_types{
    large_tex,
    text,
    small_tex,
    button,
    normal_tex,
};

struct ui_s {
    uint ui_index;
    struct ui_element_s *UI_ELEMENTS;
    struct ui_element_s *UI_ELEMENTS_end; 
    struct ui_s *next_ui;
};

struct ui_s* ui_ll_head;
struct ui_s* ui_ll_end;

static uint current_ui_index;
static uint current_ui_element_index;
int ui_create(){

    struct ui_s *c = mem_malloc(sizeof(struct ui_s));
    memset(c, 0, sizeof(struct ui_s));

    if (!ui_ll_head) ui_ll_head = c;
    else ui_ll_end->next_ui = c;
    ui_ll_end = c;

    c->ui_index = current_ui_index;
    return current_ui_index++;
}

static struct ui_s *find_ui(int index){
    for (struct ui_s *c = ui_ll_head; c != 0; c = c->next_ui) if (c->ui_index == index) return c;
    return 0;
}

static struct ui_element_s *add_ui_element(int index){
    struct ui_s *ui = find_ui(index);
    if (!ui) return 0;

    struct ui_element_s *new = mem_malloc(sizeof(struct ui_element_s));
    memset(new, 0, sizeof(struct ui_element_s));

    if (!ui->UI_ELEMENTS) ui->UI_ELEMENTS = new;
    else ui->UI_ELEMENTS_end->next = new;
    ui->UI_ELEMENTS_end = new;

    new->element_index = current_ui_element_index++;

    return new;
}

int ui_element_add_solid_tex(int ui_index, int x, int y, int width, int height, colour_t colour, uchar flags, uchar depth){

    struct ui_element_s *c = add_ui_element(ui_index);
    if (!c) return -1;

    uint colour_id = solid_colour_try(colour.r , colour.g , colour.b);

    if (width * height > 64 * 64){
        struct large_tex_s*element = mem_malloc(sizeof(struct large_tex_s));
        
        int *tex_ptr = 0;
        int tex_count = 0;
        large_tex_create(tex_ptr, tex_count, x, y, width, height, colour_id, depth, flags);

        element->colour_id = colour_id;
        element->depth = depth;
        element->flags = flags;
        element->height = height;
        element->width = width;
        element->x = x;
        element->y = y;

        element->tex_ptr = tex_ptr;
        element->tex_num = tex_count;
    
        c->element_ptr = element;
        c->element_type = large_tex;        
    }
    else{
        struct small_tex_s *element = mem_malloc(sizeof(struct small_tex_s));

        element->tex_id = solid_colour_tex_add(x, y, width, height, colour_id, 1.0f, depth, flags);

        element->colour_id = colour_id;
        element->depth = depth;
        element->flags = flags;
        element->height = height;
        element->width = width;
        element->x = x;
        element->y = y;

        c->element_ptr = element;
        c->element_type = small_tex;
    }

    return c->element_index;
}

int ui_element_add_text(int ui_index, const char *string, char text_type, int x, int y, float scale, uchar depth, char flags, char draw_from_center){

    struct ui_element_s *c = add_ui_element(ui_index);
    if (!c) return -1;

    struct string_s *element = mem_malloc(sizeof(struct string_s));

    // Keep our own copy so the caller's buffer can change or be freed
    element->string = mem_malloc(strlen(string) + 1);
    strcpy(element->string, string);

    element->text_type = text_type;
    element->x = x;
    element->y = y;
    element->scale = scale;
    element->depth = depth;
    element->flags = flags;
    element->draw_from_center = draw_from_center;
    element->string_id = string_render(element->string, text_type, x, y, scale, depth, flags, draw_from_center);

    c->element_ptr = element;
    c->element_type = text;

    return c->element_index;
}

int ui_element_add_tex(int ui_index, int x, int y, ushort texture_id, float scale, uchar depth, uchar flags){

    struct ui_element_s *c = add_ui_element(ui_index);
    if (!c) return -1;

    struct normal_tex_s *element = mem_malloc(sizeof(struct normal_tex_s));

    element->texture_id = texture_id;
    element->x = x;
    element->y = y;
    element->scale = scale;
    element->depth = depth;
    element->flags = flags;
    element->tex_id = tex_add(x, y, texture_id, scale, depth, flags);

    c->element_ptr = element;
    c->element_type = normal_tex;

    return c->element_index;
}

int ui_element_add_button(int ui_index, int x, int y, int width, int height,
                          void (*on_press_func_ptr)(button_t *c), void (*render_function)(uchar hover),
                          const char *text_str, uchar text_type, float text_scale,
                          colour_t colour, colour_t hover_colour, uchar flags){

    struct ui_element_s *c = add_ui_element(ui_index);
    if (!c) return -1;

    button_t *b = button_add(x, y, width, height, on_press_func_ptr, render_function,
                             text_str, text_type, text_scale, colour, hover_colour, flags);

    c->element_ptr = b;
    c->element_type = button;

    return c->element_index;
}

// Returns the button_t behind a button element, or 0
button_t *ui_element_get_button(int ui_index, int element_index){
    struct ui_s *ui = find_ui(ui_index);
    if (!ui) return 0;

    for (struct ui_element_s *e = ui->UI_ELEMENTS; e; e = e->next)
        if (e->element_index == element_index && e->element_type == button) return e->element_ptr;

    return 0;
}

// Takes an element off the screen and frees its data (not the list node)
static void ui_element_free(struct ui_element_s *e){
    switch (e->element_type){
        case large_tex: {
            struct large_tex_s *t = e->element_ptr;
            for (int i = 0; i < t->tex_num; i++) tex_remove(t->tex_ptr[i]);
            free(t->tex_ptr);
            break;
        }
        case small_tex: {
            struct small_tex_s *t = e->element_ptr;
            tex_remove(t->tex_id);
            break;
        }
        case text: {
            struct string_s *t = e->element_ptr;
            string_remove(t->string_id);
            free(t->string);
            break;
        }
        case normal_tex: {
            struct normal_tex_s *t = e->element_ptr;
            tex_remove(t->tex_id);
            break;
        }
        case button:
            button_remove(e->element_ptr);   // frees the button itself
            e->element_ptr = 0;
            break;
    }
    free(e->element_ptr);
    e->element_ptr = 0;
}

int ui_element_remove(int ui_index, int element_index){
    struct ui_s *ui = find_ui(ui_index);
    if (!ui) return -1;

    struct ui_element_s *prev = 0;
    for (struct ui_element_s *e = ui->UI_ELEMENTS; e; prev = e, e = e->next){
        if (e->element_index != element_index) continue;

        if (prev) prev->next = e->next;
        else ui->UI_ELEMENTS = e->next;
        if (ui->UI_ELEMENTS_end == e) ui->UI_ELEMENTS_end = prev;

        ui_element_free(e);
        free(e);
        return 0;
    }
    return -1;
}

// Moves a texture or text element. Buttons aren't supported (returns -1).
int ui_element_move(int ui_index, int element_index, int x, int y){
    struct ui_s *ui = find_ui(ui_index);
    if (!ui) return -1;

    struct ui_element_s *e = ui->UI_ELEMENTS;
    while (e && e->element_index != element_index) e = e->next;
    if (!e) return -1;

    switch (e->element_type){
        case large_tex: {
            struct large_tex_s *t = e->element_ptr;
            for (int i = 0; i < t->tex_num; i++) tex_remove(t->tex_ptr[i]);
            free(t->tex_ptr);
            t->x = x; t->y = y;
            large_tex_create(t->tex_ptr, t->tex_num, x, y, t->width, t->height, t->colour_id, t->depth, t->flags);
            return 0;
        }
        case small_tex: {
            struct small_tex_s *t = e->element_ptr;
            tex_remove(t->tex_id);
            t->x = x; t->y = y;
            t->tex_id = solid_colour_tex_add(x, y, t->width, t->height, t->colour_id, 1.0f, t->depth, t->flags);
            return 0;
        }
        case text: {
            struct string_s *t = e->element_ptr;
            string_remove(t->string_id);
            t->x = x; t->y = y;
            t->string_id = string_render(t->string, t->text_type, x, y, t->scale, t->depth, t->flags, t->draw_from_center);
            return 0;
        }
        case normal_tex: {
            struct normal_tex_s *t = e->element_ptr;
            tex_remove(t->tex_id);
            t->x = x; t->y = y;
            t->tex_id = tex_add(x, y, t->texture_id, t->scale, t->depth, t->flags);
            return 0;
        }
    }
    return -1;
}

// Removes every element but keeps the UI so it can be refilled
int ui_clear(int ui_index){
    struct ui_s *ui = find_ui(ui_index);
    if (!ui) return -1;

    struct ui_element_s *e = ui->UI_ELEMENTS;
    while (e){
        struct ui_element_s *next = e->next;
        ui_element_free(e);
        free(e);
        e = next;
    }
    ui->UI_ELEMENTS = 0;
    ui->UI_ELEMENTS_end = 0;
    return 0;
}

// Removes every element and the UI itself
int ui_destroy(int ui_index){
    struct ui_s *prev = 0, *ui = ui_ll_head;
    while (ui && ui->ui_index != ui_index){ prev = ui; ui = ui->next_ui; }
    if (!ui) return -1;

    ui_clear(ui_index);

    if (prev) prev->next_ui = ui->next_ui;
    else ui_ll_head = ui->next_ui;
    if (ui_ll_end == ui) ui_ll_end = prev;

    free(ui);
    return 0;
}
