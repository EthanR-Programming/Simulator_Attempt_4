#include "button.h"
#include "collision.h"

extern volatile struct input_s user_inputs;

button_t *button_linked_list;

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

void button_render(button_t *c , uchar use_hover_colour){

    // Not transparent then draw button colour
    if (c->colour.a != 0x0){
        
        // create colour id if its not already created
        if (!c->colour_id) c->colour_id = solid_colour_add(c->colour.r , c->colour.g , c->colour.b);

        if (!c->hover_colour_id) c->hover_colour_id = solid_colour_add(c->hover_colour.r , c->hover_colour.g , c->hover_colour.b);

        c->tex_ids.tex_ids_ptr[(c->backgound_tex_index == -1) ? c->tex_ids.tex_id_num++ : c->backgound_tex_index] = solid_colour_tex_add(c->x , c->y , c->width , c->height , (use_hover_colour) ? c->hover_colour_id : c->colour_id , 1.0f,  240 , c->flags);
        c->backgound_tex_index = (c->backgound_tex_index == -1) ? c->tex_ids.tex_id_num - 1 : c->backgound_tex_index;
    }

    if (c->text){
        if (c->text[0] != '\0'){

            int mid_x = c->x + (c->width / 2);
            int mid_y = c->y + (c->height / 2);

            int string_length = string_get_draw_width(c->text , c->text_type , 1.0f);

            float scale = c->text_scale;

            if (c->text_scale == 0.0f){
                scale = (float)((float)(c->width - 10) / (float)string_length);
            }

            c->tex_ids.str_ids_ptr[c->tex_ids.str_id_num++] = string_render(c->text , c->text_type , mid_x , mid_y , scale , 255 , c->flags & ~(round_edges) , 1);
        }
    }
}



void button_render_remove(button_t *c){

    for (int i = 0;i < c->tex_ids.tex_id_num;i++){
        tex_remove(c->tex_ids.tex_ids_ptr[i]);
    }
    for (int i = 0;i < c->tex_ids.str_id_num;i++){
        string_remove(c->tex_ids.str_ids_ptr[i]);
    }
    c->tex_ids.tex_id_num = 0;
    c->tex_ids.str_id_num = 0;
}

// -1 on failure
char button_remove(
    button_t *button
){
    if (button == NULL){
        return -1;
    }
    
    button_t *c = button_linked_list;
    button_t *prev = c;

    if (!c) return -1;

    while(c != button) {
        if (c == 0) return -1;
        prev = c;
        c = c -> next;
    }

    if (c == button_linked_list){
        button_linked_list = c->next;
    }
    else{
        prev->next = c->next;
    }

    button_render_remove(button);  // take its textures/strings off screen
    free(button->tex_ids.tex_ids_ptr);
    free(button->tex_ids.str_ids_ptr);
    free(button);

    return 0;
}

// optional on_press_func_ptr and render_function and text, 0 ALPHA for no colour
button_t *button_add(
    int x, int y, 
    int width, int height, 
    void (*on_press_func_ptr), 
    void (*render_function), 
    const char *text , uchar text_type, float text_scale,
    colour_t colour, 
    colour_t hover_colour, 
    uchar flags
){
    button_t* c = button_add_ll_element();

    c->backgound_tex_index = -1;
    c->x = x - (width >> 1);
    c->y = y - (height >> 1);
    c->width = width;
    c->height = height;
    c->function_ptr = on_press_func_ptr;
    c->text = text;
    c->colour = colour;
    c->render_func_ptr = render_function;    
    c->flags = flags;
    c->text_type = text_type;
    c->text_scale = text_scale;
    c->hover_colour = hover_colour;
    c->clickable = 1;

    if (!c->tex_ids.tex_ids_ptr){
        c->tex_ids.tex_ids_ptr = mem_malloc(sizeof(int) * 64);
        c->tex_ids.str_ids_ptr = mem_malloc(sizeof(int) * 64);
    }

    if (!c->render_func_ptr) button_render(c , false);

    return c;
}

// use custom rendering function

void button_poll(){

    button_t *c = button_linked_list;

    while(c){

        if (cursor_on_texture(c->x , c->y , c->width , c->height , 1.0f , c->flags)){

            if (user_inputs.mouse_buttons[GLFW_MOUSE_BUTTON_LEFT]){
                if (c->function_ptr) (c->function_ptr)(c);
            }

            if (c->currently_hovered == 1){
                c = c -> next;
                continue;
            }

            if (c->render_func_ptr){
                (c->render_func_ptr)(true);
                c->currently_hovered = 1;
                c = c -> next;
                continue;
            }

            // cursor on button
            if (c->hover_colour.a != 0x0){

                if (c->backgound_tex_index == -1){
                    printf("Button does not have a background index when it should? %s\n" , c->text);
                    return;
                }
                
                if (c->tex_ids.tex_ids_ptr[c->backgound_tex_index] != - 1) tex_remove(c->tex_ids.tex_ids_ptr[c->backgound_tex_index]);
                c->tex_ids.tex_ids_ptr[c->backgound_tex_index] = solid_colour_tex_add(c->x , c->y , c->width , c->height , c->hover_colour_id , 1.0f , 240 , c->flags);
                c->currently_hovered = 1;
            }
            else{
                if (c->tex_ids.tex_ids_ptr[c->backgound_tex_index] != -1) tex_remove(c->tex_ids.tex_ids_ptr[c->backgound_tex_index]);
            }
        }
        else{
            if (c->currently_hovered == 1){

                if (c->render_func_ptr){
                    (c->render_func_ptr)(false);
                    c->currently_hovered = 0;
                    c = c -> next;
                    continue;
                }

                if (c->tex_ids.tex_ids_ptr[c->backgound_tex_index] != -1) tex_remove(c->tex_ids.tex_ids_ptr[c->backgound_tex_index]);
                c->tex_ids.tex_ids_ptr[c->backgound_tex_index] = solid_colour_tex_add(c->x , c->y , c->width , c->height , c->colour_id , 1.0f , 240 , c->flags);

                c->currently_hovered = 0;
            }
        }
        c = c -> next;
    }
}

