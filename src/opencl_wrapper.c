#include "opencl_wrapper.h"


char platformName[256];

struct cl_main_context_s opencl_context;

void opencl_init()
{
    cl_int err;

    cl_uint num_platforms = 0;
    err = clGetPlatformIDs(0, NULL, &num_platforms);
    if (err != CL_SUCCESS || num_platforms == 0) {
        printf("No OpenCL platforms found\n");
        return;
    }

    cl_platform_id *platforms = (cl_platform_id*)mem_malloc(sizeof(cl_platform_id) * num_platforms);

    clGetPlatformIDs(num_platforms, platforms, NULL);

    cl_platform_id platform = NULL;

    for (cl_uint i = 0; i < num_platforms; i++) {

        size_t ext_size = 0;
        clGetPlatformInfo(platforms[i],CL_PLATFORM_EXTENSIONS,0, NULL, &ext_size);

        char *ext = (char*)mem_malloc(ext_size);
        clGetPlatformInfo(platforms[i],CL_PLATFORM_EXTENSIONS,ext_size, ext, NULL);

        if (strstr(ext, "cl_khr_gl_sharing")) {platform = platforms[i];
            free(ext);
            break;
        }

        free(ext);
    }

    free(platforms);

    if (!platform) {
        printf("No OpenCL platform with cl_khr_gl_sharing found\n");
        return;
    }

    cl_device_id device;
    err = clGetDeviceIDs(platform,CL_DEVICE_TYPE_GPU,1,&device,NULL);

    if (err != CL_SUCCESS) {
        printf("Failed to get GPU device\n");
        return;
    }

    GLXContext gl_ctx = glXGetCurrentContext();
    Display *gl_display = glXGetCurrentDisplay();

    if (!gl_ctx || !gl_display) {
        printf("ERROR: No current OpenGL context. Did you call glfwMakeContextCurrent(window)?\n");
        return;
    }

    cl_context_properties props[] = {
        CL_GL_CONTEXT_KHR, (cl_context_properties)glXGetCurrentContext(),
        CL_GLX_DISPLAY_KHR, (cl_context_properties)glXGetCurrentDisplay(),
        CL_CONTEXT_PLATFORM,  (cl_context_properties)platform,
        0
    };

    opencl_context.context =
        clCreateContext(props,1,&device,NULL,NULL,&err);

    if (err != CL_SUCCESS || !opencl_context.context) {
        printf("clCreateContext failed: %d\n", err);
        return;
    }

    opencl_context.platform = platform;
    opencl_context.device = device;

    opencl_context.queue =
        clCreateCommandQueueWithProperties(opencl_context.context,device,0,&err);

    if (err != CL_SUCCESS) {
        printf("clCreateCommandQueue failed: %d\n", err);
        return;
    }

    char name[256] = {0};
    clGetPlatformInfo(platform,CL_PLATFORM_NAME,sizeof(name),name,NULL);

    printf("OpenCL Platform: %s\n", name);
}

// Function to check for errors and print a message if there is one
void checkError(cl_int err, const char* msg) {
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Error: %s (%d)\n", msg, err);
        //exit(EXIT_FAILURE);
    }
}

// Function to create a buffer with error checking
cl_mem cl_buffer_create(cl_mem_flags flags, size_t size) {
    cl_int err;
    cl_mem buffer = clCreateBuffer(opencl_context.context, flags, size, 0, &err);
    checkError(err, "Failed to create buffer");
    return buffer;
}

// Function to create a program with error checking
cl_program cl_program_create( const char* source) {
    cl_int err;
    cl_program program = clCreateProgramWithSource(opencl_context.context, 1, (const char**)&source, NULL, &err);
    checkError(err, "Failed to create program");
    return program;
}

// Function to build a program with error checking
void cl_program_build(cl_program program, cl_device_id device) {
    cl_int err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    checkError(err, "Failed to build program");

    size_t log_size;
    clGetProgramBuildInfo(program,
                        opencl_context.device,
                        CL_PROGRAM_BUILD_LOG,
                        0,
                        NULL,
                        &log_size);

    char *log = mem_malloc(log_size + 1);

    clGetProgramBuildInfo(program,
                        opencl_context.device,
                        CL_PROGRAM_BUILD_LOG,
                        log_size,
                        log,
                        NULL);

    log[log_size] = '\0';
    if (log_size > 5){
        printf("log: %s\n", log);
    }
    free(log);

}

// Function to create a kernel with error checking
cl_kernel cl_kernel_create(cl_program program, const char* kernel_name) {
    cl_int err;
    cl_kernel kernel = clCreateKernel(program, kernel_name, &err);
    checkError(err, "Failed to create kernel");

    return kernel;
}

// Function to set the arguments for a kernel with error checking
void cl_kernel_set_arg(cl_kernel kernel, cl_uint arg_index, size_t arg_size, const void* arg_value) {
    cl_int err = clSetKernelArg(kernel, arg_index, arg_size, arg_value);
    checkError(err, "Failed to set kernel argument\n");
    //printf("%d %d %d %p\n" , kernel, arg_index , arg_size , arg_value);
}

// Function to enqueue a kernel with error checking
void opencl_kernel_run(cl_kernel kernel ,size_t size) {
    size_t local[2] = {1 , 32};
    size_t work_dim = 2;
    size_t global_work[2] = {size , 32};
    cl_int err = clEnqueueNDRangeKernel(opencl_context.queue, kernel, 2, NULL, global_work, local, 0, NULL, NULL);
    checkError(err, "Failed to enqueue kernel");
}

// Function to read buffer data with error checking
void* cl_buffer_read(cl_mem buffer, size_t size) {
    void* host_ptr = mem_malloc(size);
    memset(host_ptr , 0 , size);
    cl_int err = clEnqueueReadBuffer(opencl_context.queue, buffer, CL_TRUE, 0, size, host_ptr, 0, NULL, NULL);
    checkError(err, "Failed to read buffer");
    return host_ptr;
}

// Function to write data to a buffer with error checking
void cl_buffer_write(cl_mem *buffer, const void* host_ptr, size_t write_size) {
    cl_int err = clEnqueueWriteBuffer(opencl_context.queue, *buffer, CL_FALSE, 0, write_size, host_ptr, 0, NULL, NULL);
    checkError(err, "Failed to write to buffer");
}

// Function to resize a buffer with error checking
cl_mem cl_buffer_resize(cl_mem_flags flags,cl_mem original_buffer, size_t original_size , size_t new_size) {
    // Read the data from the original buffer into a temporary host pointer
    void* temp_host_ptr = mem_malloc(original_size);
    clEnqueueReadBuffer(NULL, original_buffer, CL_FALSE, 0, original_size, temp_host_ptr, 0, NULL, NULL);

    // Create a new buffer with the updated size
    cl_mem new_buffer = clCreateBuffer(opencl_context.context, flags, new_size, NULL, NULL);

    // Write the data from the temporary host pointer to the new buffer
    clEnqueueWriteBuffer(NULL, new_buffer, CL_FALSE, 0, new_size, temp_host_ptr, 0, NULL, NULL);

    // Free the temporary host pointer (it's no longer needed)
    free(temp_host_ptr);

    clReleaseMemObject(original_buffer);

    return new_buffer;
}



// create kernel from .cl file:
cl_kernel cl_kernel_create_from_file (char *file_name , char*kernel_name){
    int len = 0;
    char *kern_buf = file_load(file_name , &len);
    cl_program program = cl_program_create(kern_buf);
    cl_program_build(program , opencl_context.device);

    cl_kernel kern = cl_kernel_create(program , kernel_name);

    //printf("kernel: %s : %d\n" , kernel_name , kern);

    return kern;
}