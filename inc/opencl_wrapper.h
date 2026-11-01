#ifndef cl_func_h
#define cl_func_h

#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include "CL/cl_gl.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <GL/glx.h>
#include <X11/Xlib.h>
#include "file_loader.h"

typedef struct cl_main_context_s {

    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;

} cl_main_context_t;

// Function prototypes for error checking and buffer operations
void checkError(cl_int err, const char* msg);
cl_mem cl_buffer_create(cl_mem_flags flags, size_t size);
cl_program cl_program_create(const char* source);
void cl_program_build(cl_program program, cl_device_id device);
cl_kernel cl_kernel_create(cl_program program, const char* kernel_name);
void cl_kernel_set_arg(cl_kernel kernel, cl_uint arg_index, size_t arg_size, const void* arg_value);
void opencl_kernel_run(cl_kernel kernel ,size_t size);
void* cl_buffer_read(cl_mem buffer, size_t size);
void cl_buffer_write(cl_mem *buffer, const void* host_ptr, size_t write_size);
cl_mem cl_buffer_resize(cl_mem_flags flags,cl_mem original_buffer, size_t original_size , size_t new_size);
void opencl_init();
cl_kernel cl_kernel_create_from_file (char *file_name , char*kernel_name);

#endif