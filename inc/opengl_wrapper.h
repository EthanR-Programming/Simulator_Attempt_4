#ifndef gl_func_h
#define gl_func_h

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "file_loader.h"

GLFWwindow* opengl_init(int width, int height, char *window_name);
void buffer_swap(GLFWwindow *window , GLuint frame_texture);
GLuint opengl_pbo_create(int width, int height);
GLuint opengl_texture_create(int width, int height);

#endif