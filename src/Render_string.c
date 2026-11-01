#include "render_string.h"

// Holds Tex sprite Positions
extern struct text_s *text_info;

// Holds all the IDS of the individual chars
struct string_s {

    long long string_ID;

    int *ID_array;
    int ID_array_size;

    struct string_s * next;
};

// Holds starting ptr of string IDS
struct string_s * string_id_buffer;
struct string_s * string_id_buffer_end;
long long global_string_ID;

// Adds the string to render list
long long string_render(char *string , char text_type, int x, int y , float scale, unsigned char depth, char flags, char draw_from_center){

    if (!text_info){
        printf("called text rendering before text loaded :(\n");
        return;
    }

    int string_x = x;
    int string_y = y;

    struct string_s * c;

    if (string_id_buffer_end){
        string_id_buffer_end->next = malloc(sizeof(struct string_s));
        memset(string_id_buffer_end ->next , 0 , sizeof(struct string_s));
        string_id_buffer_end = string_id_buffer_end -> next;
        c = string_id_buffer_end;
    }
    else{
        string_id_buffer = malloc(sizeof(struct string_s));
        memset(string_id_buffer , 0 , sizeof(struct string_s));
        string_id_buffer_end = string_id_buffer;
        c = string_id_buffer;
    }

    int string_length = strlen(string);

    int min_y = INT_MAX;
    int max_y = INT_MIN;

    int total_offset = 0;
    for (int i = 0; i < string_length;i++){
        total_offset += ceil(text_info[text_type].advance[string[i]] * scale);
    
        
        int yoffset = ceil(text_info[text_type].yoffset[string[i]] * scale);
        int height = ceil(text_info[text_type].height[string[i]] * scale);

        int top = -yoffset - height;
        int bottom = -yoffset;

        if (top < min_y) min_y = top;
        if (bottom > max_y) max_y = bottom;

    }

    string_x += - (total_offset / 2);
    if (draw_from_center) string_y = y - (min_y + max_y) / 2;

    c->ID_array = malloc((string_length + 1) * sizeof(int));
    memset(c->ID_array , 0 , (string_length + 1) * sizeof(int));

    for (int i = 0; i < string_length ;i++){
        int char_x = string_x + ceil(text_info[text_type].xoffset[string[i]] * scale);
        int char_y = string_y - ceil(text_info[text_type].yoffset[string[i]] * scale) - ceil(text_info[text_type].height[string[i]] * scale);
        
        c->ID_array[i] = tex_add(char_x, char_y, text_info[text_type].text_begining + string[i] , scale , depth , flags);

        c->ID_array_size++;

        string_x += ceil(text_info[text_type].advance[string[i]] * scale);
    }

    c->string_ID = global_string_ID;
    global_string_ID++;

    return c->string_ID;
}

// Removes string IDS from render list
void string_remove(long long ID){

    struct string_s * prev = 0;

    for (struct string_s *c = string_id_buffer; c != 0; c = c->next){

        if (c->string_ID == ID){
            for (int i = 0; i < c->ID_array_size;i++){
                tex_remove(c->ID_array[i]);
            }
            if (prev) prev->next = c->next;
            if (string_id_buffer_end == c) string_id_buffer_end = prev;
            if (c == string_id_buffer){
                if (c->next){
                    string_id_buffer = c->next;
                }
                else{
                    string_id_buffer = 0;
                    string_id_buffer_end = 0;
                }
            }
            memset(c->ID_array , 0 , c->ID_array_size * sizeof(int));
            free(c->ID_array);
            free(c);
            return;
        }
        prev = c;
    }
}

int string_get_draw_width(char *string , int text_type, float scale){

    int string_length = strlen(string);

    int total_offset = 0;
    for (int i = 0; i < string_length;i++){
        total_offset += ceil(text_info[text_type].advance[string[i]] * scale);
    }
    return total_offset;
}