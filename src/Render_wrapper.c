#include "Render_wrapper.h"

// Timings for Rendering Engine
#define CLOCK CLOCK_MONOTONIC

#include <time.h>
#include "math.h"

// Basic definintions 
#define uchar unsigned char
#define uint unsigned int
#define ushort unsigned short
#define ulong unsigned long

// OpenGL window
GLFWwindow * window; 
uint window_width;
uint window_height;
atomic_char window_change;
atomic_uint window_width_new;
atomic_uint window_height_new;
volatile char window_active;

volatile char fullscreen;
char past_fullscreen;

// Window background
volatile uchar window_background_r;
volatile uchar window_background_g;
volatile uchar window_background_b;

// Rendering Engine TARGET FPS
volatile uint target_fps;

// OpenGL frame textures
GLuint opengl_frame_texture[2];

// OpenCL frame textures
cl_mem opencl_frame_textures[2];

// OpenCL depth buffer
cl_mem opencl_depth_buffer;

// OpenCL context
extern struct cl_main_context_s opencl_context;

// OpenCL kernel
cl_kernel render_kernel;
cl_kernel resolve_kernel;

// Kernel Buffers
struct kernel_buffer {
    cl_mem buffer;
    uint max_allocated;
};
volatile struct kernel_buffer kernel_sprite_buffer;
volatile struct kernel_buffer kernel_solid_colour_buffer;

// camera position
volatile float camera_x;
volatile float camera_y;

// Texure Data for the texture buffers
struct single_tex_data_s {
    uint width;
    uint height;
    uchar* data;

    struct single_tex_data_s *next;
};
struct full_tex_data_s {

    uint *width;
    uint *height;
    uchar* data;
    uint* offsets;

    uint total_textures;
    uint total_size;
};

// text struct 
volatile struct text_s *text_info;
uint text_info_index;

// Holds ALL sprite Information
struct sprite_s {

    int x;
    int y;
    float scale;

    ushort textureID;

    uchar depth;
    uchar flags;

    uint solid_width;
    uint solid_height;
};

// Sprite buffer
struct sprite_buffer_s {
    struct sprite_s *buffer;
    uint buffer_index;
    uint buffer_max;
    atomic_char lock;

    // Stores the index of translation layer per sprite cause when removing sprite ->
    // End sprite must be moved so translation index of end sprite must be changed
    uint *sprite_buffer_translation;

    // These are used to store old / deleted sprites
    uint *buffer_sprite_translation;
    uint buffer_sprite_translation_end_index;

    uint *buffer_sprite_translation_stack;
    uint buffer_sprite_translation_stack_end_index;
};
volatile struct sprite_buffer_s sprite_buffer;

// Solid colour buffer
struct solid_colour_buffer_s {

    volatile uchar *buffer;
    volatile uint buffer_index;
    volatile uint buffer_max;
};
volatile struct solid_colour_buffer_s solid_colour_buffer;
volatile char solid_colour_buffer_change;

// Rendering engine has been INIT or not
volatile char rendering_init;

// global texture scale
volatile float global_texture_scale;

// user inputs
volatile struct input_s user_inputs;

void opengl_frame_texture_create(){

    glFinish();

    if (opengl_frame_texture[0]) glDeleteTextures(1 , &opengl_frame_texture[0]);
    if (opengl_frame_texture[1]) glDeleteTextures(1 , &opengl_frame_texture[1]);

    opengl_frame_texture[0] = opengl_texture_create(window_width , window_height);
    opengl_frame_texture[1] = opengl_texture_create(window_width , window_height);
}

void opencl_frame_texture_create(){

    clFinish(opencl_context.queue);

    cl_int err;

    if (opencl_frame_textures[0]) clReleaseMemObject(opencl_frame_textures[0]);
    if (opencl_frame_textures[1]) clReleaseMemObject(opencl_frame_textures[1]);

    opencl_frame_textures[0] = clCreateFromGLTexture(opencl_context.context,CL_MEM_READ_WRITE,GL_TEXTURE_2D,0,opengl_frame_texture[0],&err);
    checkError(err , "creating opencl frame buffer 1");
    opencl_frame_textures[1] = clCreateFromGLTexture(opencl_context.context,CL_MEM_READ_WRITE,GL_TEXTURE_2D,0,opengl_frame_texture[1],&err);
    checkError(err , "creating opencl frame buffer 2");
}

void frame_resize(){
    
    // Free OpenCL Textures
    clFinish(opencl_context.queue);
    if (opencl_frame_textures[0]) clReleaseMemObject(opencl_frame_textures[0]);
    if (opencl_frame_textures[1]) clReleaseMemObject(opencl_frame_textures[1]);
    clFinish(opencl_context.queue);

    // Free OpenGL textures
    glFinish();
    if (opengl_frame_texture[0]) glDeleteTextures(1 , &opengl_frame_texture[0]);
    if (opengl_frame_texture[1]) glDeleteTextures(1 , &opengl_frame_texture[1]);
    glFinish();

    // Create OpenGL textures
    opengl_frame_texture[0] = opengl_texture_create(window_width , window_height);
    opengl_frame_texture[1] = opengl_texture_create(window_width , window_height);
    glFinish();

    // Create OpenCL textures
    cl_int err;
    opencl_frame_textures[0] = clCreateFromGLTexture(opencl_context.context,CL_MEM_READ_WRITE,GL_TEXTURE_2D,0,opengl_frame_texture[0],&err);
    checkError(err , "creating opencl frame buffer 1");
    opencl_frame_textures[1] = clCreateFromGLTexture(opencl_context.context,CL_MEM_READ_WRITE,GL_TEXTURE_2D,0,opengl_frame_texture[1],&err);
    checkError(err , "creating opencl frame buffer 2");
    clFinish(opencl_context.queue);
}

void opencl_frame_buffer_acquire(int frame_buffer_index){
    checkError(clEnqueueAcquireGLObjects(opencl_context.queue, 1, &opencl_frame_textures[frame_buffer_index], 0, NULL, NULL), "Acquire GL Object");
}

void opencl_frame_buffer_release(int frame_buffer_index){
    checkError(clEnqueueReleaseGLObjects(opencl_context.queue,1,&opencl_frame_textures[frame_buffer_index],0,NULL,NULL),"Release GL Object");
}

void opencl_frame_buffer_fill(int frame_buffer_index){
    // Frame default colour
    //cl_float4 back_buffer_colour = {0.55f, 0.66f, 0.87f, 1.0f}; // medium grey
    
    cl_float4 back_buffer_colour = {window_background_r / 255.0f, window_background_g / 255.0f, window_background_b / 255.0f, 1.0f}; // medium grey

    // Frame fill properties
    size_t origin[3] = {0,0,0};
    size_t region[3] = {window_width , window_height,1};

    // Fill frame buffer
    checkError(clEnqueueFillImage(opencl_context.queue,opencl_frame_textures[frame_buffer_index],&back_buffer_colour,origin,region,0,0,0) , "Filling image\n");
    
    // Fill depth buffer
    int pattern = 0;
    if (opencl_depth_buffer) checkError(clEnqueueFillBuffer(opencl_context.queue , opencl_depth_buffer , &pattern , sizeof(uint) , 0 , (window_width * window_height * sizeof(uint)), 0 , 0 , 0) ,  "Filling Depth Buffer");
    
}

void opencl_resolve_run(int frame_buffer_index){
    if (!opencl_depth_buffer) return;
    cl_kernel_set_arg(resolve_kernel , 0 , sizeof(cl_mem) , &opencl_depth_buffer);
    cl_kernel_set_arg(resolve_kernel , 1 , sizeof(cl_mem) , &opencl_frame_textures[frame_buffer_index]);
    cl_kernel_set_arg(resolve_kernel , 2 , sizeof(uint) , &window_width);
    cl_kernel_set_arg(resolve_kernel , 3 , sizeof(uint) , &window_height);
    size_t global_work[2] = {window_width , window_height};
    checkError(clEnqueueNDRangeKernel(opencl_context.queue , resolve_kernel , 2 , NULL , global_work , NULL , 0 , NULL , NULL) , "Resolve kernel");
}

void opencl_kernel_arg_set(int frame_buffer_index){
    
    int cam_x = ceil(camera_x);
    int cam_y = ceil(camera_y);
    // Camera
    cl_kernel_set_arg(render_kernel, 5 , sizeof(int) , &cam_x);
    cl_kernel_set_arg(render_kernel, 6 , sizeof(int) , &cam_y);

    // scale
    cl_kernel_set_arg(render_kernel , 14 , sizeof(float) , &global_texture_scale);

    // frame buffer
    cl_kernel_set_arg(render_kernel , 9 , sizeof(cl_mem) , &opencl_frame_textures[frame_buffer_index]);
}

void opencl_kernel_arg_set_init(cl_kernel kernel){

    // Sets backbuffer width and height
    cl_kernel_set_arg(render_kernel, 10 , sizeof(uint) , &window_width);
    cl_kernel_set_arg(render_kernel, 11 , sizeof(uint) , &window_height);

    // Get maximum screen width / height
    const GLFWvidmode *mode = glfwGetVideoMode(glfwGetPrimaryMonitor());

    // Sets the screen width and height
    cl_kernel_set_arg(render_kernel, 12 , sizeof(uint) , &mode->width);
    cl_kernel_set_arg(render_kernel, 13 , sizeof(uint) , &mode->height);

    // Depth buffer
    opencl_depth_buffer = cl_buffer_create(CL_MEM_READ_WRITE , window_width * window_height * sizeof(uint));
    cl_kernel_set_arg(render_kernel , 8 , sizeof(cl_mem) , &opencl_depth_buffer);

}

struct full_tex_data_s *tex_data_combine(struct single_tex_data_s* data){

    // Starts at begin and combines all data
    struct single_tex_data_s *c = data;

    // Gets total size of data
    ulong total_size = 0;
    uint tex_number = 0; 
    while(c){
        total_size += (c->width * c->height) << 2;
        c = c -> next; tex_number++;
    }
    //printf("total size of all textures: %d\n" , total_size);

    // Resets current ptr
    c = data;

    // struct that will hold all tex info
    struct full_tex_data_s *ret = malloc(sizeof(struct full_tex_data_s));
    memset(ret , 0 , sizeof(struct full_tex_data_s));

    // Creates arrays for return struct
    ret->data = malloc(total_size); memset(ret->data , 0 , total_size);
    ret->width = malloc(sizeof(uint) * tex_number); memset(ret->width , 0 , sizeof(uint) * tex_number);
    ret->height = malloc(sizeof(uint) * tex_number); memset(ret->height , 0 , sizeof(uint) * tex_number);
    ret->offsets = malloc(sizeof(uint) * tex_number); memset(ret->offsets , 0 , sizeof(uint) * tex_number);

    uint index = 0;
    uint data_index = 0;
    // Loops through and combines all texture data and makes the width and height lists
    while(c){

        // Sets the current index to the corresponding values
        ret->width[index] = c->width;
        ret->height[index] = c->height;
        ret->offsets[index] = data_index;

        //MemCPYs the tex data into the full tex array
        memcpy(&ret->data[data_index] , c->data , (c->width * c->height) << 2);

        // Frees data not needed anymore
        free(c->data);

        // Increments data_index
        data_index += (c->width * c->height) << 2;

        // stores current so can free it after next
        struct single_tex_data_s *s = c;

        c = c->next;
        free(s);
        index++;
    }

    ret->total_textures = index;
    ret->total_size = total_size;

    return ret;
}

// stores all textures in this before they go to rendering engine
struct single_tex_data_s *global_tex_data_list;

void tex_add_png(uchar *path){
    
    uint sizeof_tex_struct = sizeof(struct single_tex_data_s);

    // Inits the list if needed
    struct single_tex_data_s * c = global_tex_data_list;
    if (!c){
        global_tex_data_list = malloc(sizeof_tex_struct); memset(global_tex_data_list , 0 , sizeof_tex_struct);
        c = global_tex_data_list;
    }
    else{
        // Gets to end of list
        while(c->next) c = c ->next;

        // appends element to list
        c->next = malloc(sizeof_tex_struct); memset(c->next , 0 , sizeof_tex_struct);

        // sets c to new element
        c = c -> next;
    }

    // Loads PNG
    int width = 0; int height = 0;
    unsigned char *data =load_png_rgba(path , &width , &height);

    // Sets data
    c->width = width;
    c->height = height;
    c->data = data;
}

void text_add(uchar *path, uchar colour[3]){
    
    // Inits text_info if needed
    if (!text_info){
        text_info = malloc(sizeof(struct text_s) * 255); memset(text_info , 0 , sizeof(struct text_s) * 255);
    }

    // Opens TTF file and returns if NULL
    FILE *f = fopen(path , "rb");
    if (!f) {
        printf("failed to open %s." , path);
        return;
    }

    // Files Length of TTF file
    fseek(f, 0, SEEK_END);
    ulong file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    // Reads TTF file
    uchar *ttf_buffer = malloc(file_size);
    fread(ttf_buffer , 1 , file_size , f);
    fclose(f);

    // text begining index in sprite arrays
    int text_begining = 0;

    // finds latest render item
    struct single_tex_data_s * c = global_tex_data_list;
    if (!c){
        global_tex_data_list = malloc(sizeof(struct single_tex_data_s)); memset(global_tex_data_list , 0 ,sizeof(struct single_tex_data_s));
        c = global_tex_data_list;
    }
    else{
        while(c->next) {
            text_begining++;
            c = c->next;
        }
        text_begining++;
        c->next = malloc(sizeof(struct single_tex_data_s)); memset(c->next , 0 , sizeof(struct single_tex_data_s));
        c = c->next;
    }

    // sets the font begining
    text_info[text_info_index].text_begining = text_begining;

    // init font
    font_init(ttf_buffer , 256);

    // loops through all characters and adds them to render list
    for(int i = 0; i < 254;i++){

        if (i == 13 || i == 160) {
            c->next = malloc(sizeof(struct single_tex_data_s)); memset(c->next , 0 , sizeof(struct single_tex_data_s));
            c = c->next;
            continue;
        }

        //character information
        uchar *bitmap = 0;
        int advance = 0;
        int yoffset = 0;
        int xoffset = 0;
        int width = 0;
        int height = 0;

        // Loads character info
        bitmap = text_load_char(ttf_buffer, &width , &height , i , 256 , &advance, &yoffset , &xoffset , colour);

        // Sets render character info
        text_info[text_info_index].advance[i] = advance;
        text_info[text_info_index].yoffset[i] = yoffset;
        text_info[text_info_index].xoffset[i] = xoffset;
        text_info[text_info_index].height[i] = height;

        // Sets texture info
        c->data = bitmap;
        c->height = height;
        c->width = width;

        // Advances tex ptr
        c->next = malloc(sizeof(struct single_tex_data_s)); memset(c->next , 0 , sizeof(struct single_tex_data_s));
        c = c->next;
    }
    text_info_index++;
    free(ttf_buffer);
}

// Add any textures you want to add inside of this
void tex_data_setup(){

    double start = glfwGetTime();

    uchar colour[3] = {0 , 0 , 0};
    text_add("AtkinsonHyperlegible-Regular.ttf" , colour); // BLACK

    colour[0] = 200; colour[1] = 200; colour[2] = 200;
    text_add("AtkinsonHyperlegible-Regular.ttf" , colour); // WHITE

    colour[0] = 247; colour[1] = 84; colour[2] = 50;
    text_add("AtkinsonHyperlegible-Regular.ttf" , colour); // Menu RED

    colour[0] = 117; colour[1] = 207; colour[2] = 47;
    text_add("AtkinsonHyperlegible-Regular.ttf" , colour); // Menu GREEN

    colour[0] = 120; colour[1] = 81; colour[2] = 175;
    text_add("AtkinsonHyperlegible-Regular.ttf" , colour); // Menu BLUE

    colour[0] = 190; colour[1] = 190; colour[2] = 190;
    text_add("AtkinsonHyperlegible-Regular.ttf" , colour); // Grey

    colour[0] = 254; colour[1] = 160; colour[2] = 96;
    text_add("AtkinsonHyperlegible-Regular.ttf" , colour); // Menu Orange


    double end = glfwGetTime();

    printf("%f\n" , end - start);

}

void tex_data_kernel_setup(){

    // Get full texture data
    struct full_tex_data_s *c = tex_data_combine(global_tex_data_list);

    // Tex Data arg
    cl_mem tex_data = cl_buffer_create(CL_MEM_READ_WRITE, c->total_size);
    cl_buffer_write(&tex_data , c->data , c->total_size);
    cl_kernel_set_arg(render_kernel , 0 , sizeof(cl_mem) , &tex_data);

    // Tex Offset Arg
    cl_mem tex_offset = cl_buffer_create(CL_MEM_READ_WRITE, c->total_textures * sizeof(uint));
    cl_buffer_write(&tex_offset , c->offsets , c->total_textures * sizeof(uint));
    cl_kernel_set_arg(render_kernel , 1 , sizeof(cl_mem) , &tex_offset);

    // width
    cl_mem tex_width = cl_buffer_create(CL_MEM_READ_WRITE, c->total_textures * sizeof(uint));
    cl_buffer_write(&tex_width , c->width , c->total_textures * sizeof(uint));
    cl_kernel_set_arg(render_kernel , 2 , sizeof(cl_mem) , &tex_width);

    // height
    cl_mem tex_height = cl_buffer_create(CL_MEM_READ_WRITE, c->total_textures * sizeof(uint));
    cl_buffer_write(&tex_height , c->height , c->total_textures * sizeof(uint));
    cl_kernel_set_arg(render_kernel , 3 , sizeof(cl_mem) , &tex_height);

}

int solid_colour_add(uchar r, uchar g, uchar b){

    char expected = 0;
    while(!atomic_compare_exchange_strong(&sprite_buffer.lock , &expected , 1)) expected = 0;

    // Checking size 
    if (solid_colour_buffer.buffer_index + 1 > solid_colour_buffer.buffer_max){
        solid_colour_buffer.buffer_max = (solid_colour_buffer.buffer_max) ? solid_colour_buffer.buffer_max << 1 : 1024;
        if (solid_colour_buffer.buffer){
            solid_colour_buffer.buffer = realloc(solid_colour_buffer.buffer , solid_colour_buffer.buffer_max * sizeof(uchar) * 3);
            if (!solid_colour_buffer.buffer){
                printf("Fatal Error!  Realloc failed!!!\n");
                atomic_store(&sprite_buffer.lock , 0);
                return -1;
            }
        }
        else{
            solid_colour_buffer.buffer = malloc(solid_colour_buffer.buffer_max * sizeof(uchar) * 3);
        }
    }

    // copy data
    int index = (solid_colour_buffer.buffer_index * 3);
    solid_colour_buffer.buffer[index + 0] = r;
    solid_colour_buffer.buffer[index + 1] = g;
    solid_colour_buffer.buffer[index + 2] = b;

    solid_colour_buffer_change = 1;

    solid_colour_buffer.buffer_index ++;
    atomic_store(&sprite_buffer.lock , 0);
    return index / 3;
}

void solid_colour_get_rgb(int id, unsigned char *r, unsigned char *g, unsigned char *b){
    if (id < 0 || id >= (int)solid_colour_buffer.buffer_index){ *r = *g = *b = 0; return; }
    *r = solid_colour_buffer.buffer[id * 3 + 0];
    *g = solid_colour_buffer.buffer[id * 3 + 1];
    *b = solid_colour_buffer.buffer[id * 3 + 2];
}

int tex_index_translation_add(int sprite_index){
    
    // if theres an empty slot in translation buffer its stored in stack
    int translation_stack_index = sprite_buffer.buffer_sprite_translation_stack_end_index;

    int translation_index = 0;
    
    // if empty slock in stack its used otherwise translation stack grows 
    if (translation_stack_index > 0){
        translation_index = sprite_buffer.buffer_sprite_translation_stack[translation_stack_index-1];
        sprite_buffer.buffer_sprite_translation_stack[translation_stack_index-1] = -1;
        sprite_buffer.buffer_sprite_translation_stack_end_index--;
    }
    else{
        translation_index = sprite_buffer.buffer_sprite_translation_end_index;
        sprite_buffer.buffer_sprite_translation_end_index++;
    }

    // add the sprite index to translation buffer and return the index to the translation buffer
    sprite_buffer.buffer_sprite_translation[translation_index] = sprite_index;
    return translation_index;
}

int tex_index_translation_remove(int translation_index){

    // Get sprite index and null tranlation buffer of translation index
    int sprite_index = sprite_buffer.buffer_sprite_translation[translation_index];
    sprite_buffer.buffer_sprite_translation[translation_index] = -1;
    
    // Add this index onto the empty translation slot stack
    sprite_buffer.buffer_sprite_translation_stack[sprite_buffer.buffer_sprite_translation_stack_end_index++] = translation_index;

    // return the true sprite address
    return sprite_index;
}

void tex_remove(int translation_index){
    char expected = 0;
    while(!atomic_compare_exchange_strong(&sprite_buffer.lock , &expected , 1)) expected = 0;

    // get the sprite index of the sprite you want to remove
    int sprite_index = tex_index_translation_remove(translation_index);

    if (sprite_buffer.buffer_index - 1 != sprite_index){

        // replace with last sprite
        sprite_buffer.buffer[sprite_index] = sprite_buffer.buffer[sprite_buffer.buffer_index - 1];

        // get last sprite translation index
        int translation_index_end = sprite_buffer.sprite_buffer_translation[sprite_buffer.buffer_index - 1];

        // update the translation buffer of moved sprite to new position
        sprite_buffer.buffer_sprite_translation[translation_index_end] = sprite_index;
    
        // update its sprite
        sprite_buffer.sprite_buffer_translation[sprite_index] = translation_index_end;

    }
    sprite_buffer.buffer_index--;

    atomic_store(&sprite_buffer.lock , 0);
}

int tex_add(int x, int y, ushort texture_id, float scale, uchar depth, uchar flags){

    char expected = 0;
    while(!atomic_compare_exchange_strong(&sprite_buffer.lock , &expected , 1)) expected = 0;

    // Checking size
    if (sprite_buffer.buffer_index + 1 > sprite_buffer.buffer_max){
        sprite_buffer.buffer_max = (sprite_buffer.buffer_max) ? sprite_buffer.buffer_max << 1 : 1024;
        if (sprite_buffer.buffer) {
            sprite_buffer.buffer = realloc(sprite_buffer.buffer , sprite_buffer.buffer_max * sizeof(struct sprite_s));
            sprite_buffer.buffer_sprite_translation = realloc(sprite_buffer.buffer_sprite_translation, sprite_buffer.buffer_max * sizeof(int));
            sprite_buffer.buffer_sprite_translation_stack = realloc(sprite_buffer.buffer_sprite_translation_stack, sprite_buffer.buffer_max * sizeof(int));
            sprite_buffer.sprite_buffer_translation = realloc(sprite_buffer.sprite_buffer_translation , sizeof(int) * sprite_buffer.buffer_max);
            if (!sprite_buffer.buffer){
                printf("fatal error!!! REALLOC failed!!!\n");
                atomic_store(&sprite_buffer.lock , 0);
                return -1;
            }
        }
        else{
            sprite_buffer.buffer = malloc(sizeof(struct sprite_s) * sprite_buffer.buffer_max);
            sprite_buffer.buffer_sprite_translation = malloc(sizeof(int) * sprite_buffer.buffer_max);
            sprite_buffer.buffer_sprite_translation_stack = malloc(sizeof(int) * sprite_buffer.buffer_max);
            sprite_buffer.sprite_buffer_translation = malloc(sizeof(int) * sprite_buffer.buffer_max);
        }
    }

    // copying data
    int index = sprite_buffer.buffer_index;
    sprite_buffer.buffer[index].x = x;
    sprite_buffer.buffer[index].y = y;
    sprite_buffer.buffer[index].textureID = texture_id;
    sprite_buffer.buffer[index].scale = scale;
    sprite_buffer.buffer[index].depth = depth;
    sprite_buffer.buffer[index].flags = flags;
    sprite_buffer.buffer_index++;

    atomic_store(&sprite_buffer.lock , 0);

    // add this sprite to translation buffer
    int translation_index = tex_index_translation_add(index);

    // Add the translation index to a buffer of the current sprite index
    sprite_buffer.sprite_buffer_translation[index] = translation_index;

    // return translation index
    return translation_index;
}

int solid_colour_tex_add(int x, int y, int width, int height, ushort texture_id, float scale, uchar depth, uchar flags){
    char expected = 0;
    while(!atomic_compare_exchange_strong(&sprite_buffer.lock , &expected , 1)) expected = 0;

    // Checking size
    if (sprite_buffer.buffer_index + 1 > sprite_buffer.buffer_max){
        sprite_buffer.buffer_max = (sprite_buffer.buffer_max) ? sprite_buffer.buffer_max << 1 : 1024;
        if (sprite_buffer.buffer) {
            sprite_buffer.buffer = realloc(sprite_buffer.buffer , sprite_buffer.buffer_max * sizeof(struct sprite_s));
            sprite_buffer.buffer_sprite_translation = realloc(sprite_buffer.buffer_sprite_translation, sprite_buffer.buffer_max * sizeof(int));
            sprite_buffer.buffer_sprite_translation_stack = realloc(sprite_buffer.buffer_sprite_translation_stack, sprite_buffer.buffer_max * sizeof(int));
            sprite_buffer.sprite_buffer_translation = realloc(sprite_buffer.sprite_buffer_translation , sizeof(int) * sprite_buffer.buffer_max);
            if (!sprite_buffer.buffer){
                printf("fatal error!!! REALLOC failed!!!\n");
                atomic_store(&sprite_buffer.lock , 0);
                return -1;
            }
        }
        else{
            sprite_buffer.buffer = malloc(sizeof(struct sprite_s) * sprite_buffer.buffer_max);
            sprite_buffer.buffer_sprite_translation = malloc(sizeof(int) * sprite_buffer.buffer_max);
            sprite_buffer.buffer_sprite_translation_stack = malloc(sizeof(int) * sprite_buffer.buffer_max);
            sprite_buffer.sprite_buffer_translation = malloc(sizeof(int) * sprite_buffer.buffer_max);
        }
    }

    // copying data
    int index = sprite_buffer.buffer_index;
    sprite_buffer.buffer[index].x = x;
    sprite_buffer.buffer[index].y = y;
    sprite_buffer.buffer[index].textureID = texture_id;
    sprite_buffer.buffer[index].scale = scale;
    sprite_buffer.buffer[index].depth = depth;
    sprite_buffer.buffer[index].flags = flags | (solid_texture);
    sprite_buffer.buffer[index].solid_width = width;
    sprite_buffer.buffer[index].solid_height = height;
    sprite_buffer.buffer_index++;

    atomic_store(&sprite_buffer.lock , 0);

    // add this sprite to translation buffer
    int translation_index = tex_index_translation_add(index);

    // Add the translation index to a buffer of the current sprite index
    sprite_buffer.sprite_buffer_translation[index] = translation_index;

    // return translation index
    return translation_index;
}

void kernel_write_render_arrays(){

    char expected = 0;
    while(!atomic_compare_exchange_strong(&sprite_buffer.lock , &expected , 1)) expected = 0;


    //  Checks if sprite buffer needs creating or re sizing
    if (sprite_buffer.buffer_index >= kernel_sprite_buffer.max_allocated){
        if (kernel_sprite_buffer.buffer) clReleaseMemObject(kernel_sprite_buffer.buffer);
        kernel_sprite_buffer.max_allocated = sprite_buffer.buffer_index + 1024;
        kernel_sprite_buffer.buffer = cl_buffer_create(CL_MEM_READ_WRITE , sizeof(struct sprite_s) * kernel_sprite_buffer.max_allocated);
        cl_kernel_set_arg(render_kernel , 4 , sizeof(cl_mem) , &kernel_sprite_buffer.buffer);
    }

    // Checks if the sprite data exists
    if (!sprite_buffer.buffer) {
        atomic_store(&sprite_buffer.lock , 0);
        return;
    }

    // writes sprite data
    cl_event sprite_buffer_event;
    clEnqueueWriteBuffer(opencl_context.queue, kernel_sprite_buffer.buffer, 0 , 0 , sizeof(struct sprite_s) * sprite_buffer.buffer_index, sprite_buffer.buffer, 0, 0, &sprite_buffer_event);

    // Checks if solid colour buffer needs creating or resizing
    if (solid_colour_buffer.buffer_index > kernel_solid_colour_buffer.max_allocated || kernel_solid_colour_buffer.max_allocated == 0){
        if (kernel_solid_colour_buffer.buffer) clReleaseMemObject(kernel_solid_colour_buffer.buffer);
        kernel_solid_colour_buffer.max_allocated = solid_colour_buffer.buffer_index + 1024;
        kernel_solid_colour_buffer.buffer = cl_buffer_create(CL_MEM_READ_WRITE , sizeof(uchar) * 3 * kernel_solid_colour_buffer.max_allocated);
        cl_kernel_set_arg(render_kernel , 7 , sizeof(cl_mem) , &kernel_solid_colour_buffer.buffer);
    }   

    // Checks if solid colour data exists
    if (!solid_colour_buffer.buffer) {
        clWaitForEvents(1, &sprite_buffer_event);
        clReleaseEvent(sprite_buffer_event);
        atomic_store(&sprite_buffer.lock , 0);   
        return;
    }

    // writes solid colour data
    if (solid_colour_buffer_change) {
        clEnqueueWriteBuffer(opencl_context.queue , kernel_solid_colour_buffer.buffer , CL_FALSE , 0 , sizeof(uchar) * 3 * solid_colour_buffer.buffer_index , solid_colour_buffer.buffer , 0 , 0 , 0);
    }
    solid_colour_buffer_change = 0;

    clWaitForEvents(1, &sprite_buffer_event);
    clReleaseEvent(sprite_buffer_event);
    atomic_store(&sprite_buffer.lock , 0);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    user_inputs.scroll_wheel = yoffset;
}

void input_get(){

    for (int i = 0; i < 4; i++){
        user_inputs.mouse_buttons[i] = glfwGetMouseButton(window , i);
    }

    double x = 0; double y = 0;
    glfwGetCursorPos(window , &x , &y);

    user_inputs.mouse_x = (ceil(x));
    user_inputs.mouse_y = (window_height)  - (ceil(y));

    for (int i = 0; i < 512; i++){
        user_inputs.keys[i] = glfwGetKey(window , i);
    }
}

void window_resize_callback(GLFWwindow *window, int width, int height){

    // Error check if width or height is 0.
    if (width == 0 || height == 0){
        return;
    }

    // Set global new screen width and height
    atomic_store(&window_width_new , width);
    atomic_store(&window_height_new , height);
    atomic_store(&window_change, 1);
}

void window_resize(){

    clFinish(opencl_context.queue);
    glFinish();

    // Resize frame textures
    frame_resize();

    // Sets new backbuffer width and height
    cl_kernel_set_arg(render_kernel, 10 , sizeof(unsigned int) , &window_width);
    cl_kernel_set_arg(render_kernel, 11 , sizeof(unsigned int) , &window_height);

    // Sets new Depth buffer
    if (opencl_depth_buffer) clReleaseMemObject(opencl_depth_buffer);
    opencl_depth_buffer = cl_buffer_create(CL_MEM_READ_WRITE , window_width * window_height * sizeof(uint));
    cl_kernel_set_arg(render_kernel , 8 , sizeof(cl_mem) , &opencl_depth_buffer);

    clFinish(opencl_context.queue);
    glFinish();
}

char is_fullscreen;
int windowed_x; int windowed_y;
int windowed_width; int windowed_height;
void toggle_fullscreen()
{
    is_fullscreen = !is_fullscreen;

    if (is_fullscreen)
    {
        // Save windowed position/size
        glfwGetWindowPos(window, &windowed_x, &windowed_y);
        glfwGetWindowSize(window, &windowed_width, &windowed_height);
        
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);

        glfwSetWindowMonitor(
            window,
            monitor,
            0, 0,
            mode->width,
            mode->height,
            mode->refreshRate
        );
    }
    else
    {
        // Restore windowed mode
        glfwSetWindowMonitor(
            window,
            NULL,
            windowed_x,
            windowed_y,
            windowed_width,
            windowed_height + 37,  //CHECK IF THIS IS THE CASE ON ALL OPERATING SYSTEMS
            0
        );
    }
}

int rendering_fps = 0;
void render_loop(){

    // The index of the frame buffer
    int frame_buffer_index = 0;

    // OpenGL Window Init
    window = opengl_init(1000 , 1000, "Window");
    window_width = 1000;
    window_height = 1000;

    glfwSetWindowSizeCallback(window , window_resize_callback);
    glfwMakeContextCurrent(window);

    // Opencl Init
    opencl_init();

    // Create OpenGL frame textures
    opengl_frame_texture_create();
    
    // Create OpenCL frame textures
    opencl_frame_texture_create();

    // Create render kernel
    render_kernel = cl_kernel_create_from_file("render_kernel.cl" , "render_textures");
    resolve_kernel = cl_kernel_create_from_file("render_kernel.cl" , "resolve_depth");

    // Get all texture data
    tex_data_setup();

    // Combine texture data
    tex_data_kernel_setup();

    // Set the Kernel args for render
    opencl_kernel_arg_set_init(render_kernel);

    // Target FPS
    target_fps = 30;
    int frame_number = 0;
    int last_frame_number = 0;
    double last_time = glfwGetTime();
    double time_adjustment = 0.0f;

    // Tell opengl to use current framebuffer texture because during resize the old one was destroyed
    char use_current_tex = 0;

    // tells the logic thread when rendering starts
    rendering_init = 1;

    // set scroll callback
    glfwSetScrollCallback(window , scroll_callback);

    // Render Loop
    while(!glfwWindowShouldClose(window)){

        double start = glfwGetTime();

        if (atomic_load(&window_change) == 1){
            // Load new window dim
            window_width = atomic_load(&window_width_new);
            window_height = atomic_load(&window_height_new);
            
            // Resize window backbuffer
            window_resize();
            
            // reset atomic
            atomic_store(&window_change , 0);

            // Refresh OpenGL viewport
            glViewport(0, 0, window_width, window_height);

            // use current opencl texture in opengl
            use_current_tex = 1;
        }

        // Acquire shared object
        opencl_frame_buffer_acquire(frame_buffer_index);

        // Fill frame buffer to replace old frame and depth buffer
        opencl_frame_buffer_fill(frame_buffer_index);

        // Write Rendering arrays to GPU
        kernel_write_render_arrays(); 

        // Set openCL kernel args
        opencl_kernel_arg_set(frame_buffer_index);

        // Run opencl Kernel if total_textures not 0
        if (sprite_buffer.buffer_index != 0) opencl_kernel_run(render_kernel , sprite_buffer.buffer_index);

        // Copy packed depth/colour buffer into the frame texture
        opencl_resolve_run(frame_buffer_index);

        // Release shared object
        opencl_frame_buffer_release(frame_buffer_index);

        // Render backbuffer to screen
        buffer_swap(window , opengl_frame_texture[(!use_current_tex) ? frame_buffer_index ^ 1 : frame_buffer_index]);

        // get user inputs if logic thread is ready
        if (user_inputs.update){
            input_get();
            user_inputs.update = 0;
        }

        // reset use_current_tex
        use_current_tex = 0;

        // Swap Buffer being drawn and read from
        frame_buffer_index ^= 1;

        double end = glfwGetTime();

        // Measures FPS
        if (end - last_time >= 1.0){
            //rintf("FPS: %d , objects: %d\n" , frame_number - last_frame_number , sprite_buffer.buffer_index);

            rendering_fps = frame_number - last_frame_number;

            int missing_fps = (target_fps - (frame_number - last_frame_number)) + 2;
            time_adjustment = 0.000002777778f * (double)missing_fps;

            if (missing_fps < 0) {
                time_adjustment = (double)0.0;
            }
            last_frame_number = frame_number;
            last_time = end;
        }

        // Sleeps for each frame
        double sleep_target = (double)1.0 / (double)target_fps;
        double sleep_remaining = sleep_target - (end - start) - time_adjustment;
        if (sleep_remaining > 0.001){
            struct timespec ts = {.tv_sec = 0, .tv_nsec = (long)((sleep_remaining) * 1e9)};
            nanosleep(&ts, 0);
        }

        if (past_fullscreen != fullscreen){
            toggle_fullscreen();
        }

        past_fullscreen = fullscreen;

        frame_number++;
    }

    glfwDestroyWindow(window);
    window_active = 0;
}

// Creates Render thread
void render_init(){
    window_active = 1;
    pthread_t thread;
    pthread_create(&thread , NULL , render_loop , NULL);
}