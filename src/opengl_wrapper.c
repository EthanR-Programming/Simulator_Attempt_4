#include "opengl_wrapper.h"

extern int back_buffer_id;

extern int back_buffer_width;
extern int back_buffer_height;

GLuint global_program;
GLuint global_vao;

//shaders
GLuint CompileShader(GLenum type, const char* source)
{

    GLuint shader = glCreateShader(type);

    glShaderSource(shader, 1, &source, 0);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success)
    {
        char log[1024];
        glGetShaderInfoLog(shader, sizeof(log), 0, log);
        printf("Shader error:\n%s\n", log);
    }

    return shader;
}

//program
GLuint CreateProgram(const char* vs, const char* fs)
{
    GLuint vert = CompileShader(GL_VERTEX_SHADER, vs);
    GLuint frag = CompileShader(GL_FRAGMENT_SHADER, fs);

    GLuint program = glCreateProgram();

    glAttachShader(program, vert);
    glAttachShader(program, frag);

    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (!success)
    {
        char log[1024];
        glGetProgramInfoLog(program, sizeof(log), 0, log);
        printf("Link error:\n%s\n", log);
    }

    glDeleteShader(vert);
    glDeleteShader(frag);

    return program;
}

GLFWwindow* window_create(int width, int height, char *name){

    GLFWwindow* window = glfwCreateWindow(width, height, name, NULL, NULL);
    glfwMakeContextCurrent(window);

    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    if(!window){
        printf("Window not created...\n");
        exit(1);
    }

    return window;
}

GLint loc;

// Initializing opengl
GLFWwindow* opengl_init(int width, int height, char *window_name){

    glfwInit();

    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_SAMPLES, 4); // Enable 4x multisampling
    glfwWindowHint(GLFW_AUTO_ICONIFY, GLFW_FALSE); // Stay visible in fullscreen when focus moves to another monitor

    GLFWwindow * window = window_create(width , height , window_name);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        printf("Failed to load GLAD\n");
        exit(1);
    }
    else{
        printf("Version: %s\n", glGetString(GL_VERSION));
        printf("Vendor: %s\n", glGetString(GL_VENDOR));
        printf("Renderer: %s\n", glGetString(GL_RENDERER));
    }


    glViewport(0, 0, width, height);

    int len = 0;

    char *FS = file_load("fragment" , &len);
    char *VS = file_load("vertex" , &len);

    if (!FS || !VS){
        printf("Fragment shader or Vertex shader not found...\n");
        exit(1);
    }

    GLuint program = CreateProgram(VS, FS);

    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    global_program = program;
    global_vao = vao;

    glfwSwapInterval(0);

    loc = glGetUniformLocation(global_program, "screenTex");

    return window;

}

GLuint opengl_pbo_create(int width, int height) {
    GLuint pbo;
    glGenBuffers(1, &pbo);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, width * height * 4, NULL, GL_STREAM_DRAW);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

    return pbo;
}

GLuint opengl_texture_create(int width, int height) {
    // Create OpenGL texture
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    return tex;
}

char init_finished = 0;

void buffer_swap(GLFWwindow *window , GLuint frame_texture)
{
    if (!init_finished)
    {
        glDisable(GL_BLEND);
        glUseProgram(global_program);
        glUniform1i(loc, 0);

        glActiveTexture(GL_TEXTURE0);
        glBindVertexArray(global_vao);

        init_finished = 1;
    }

    // Display the texture that OpenCL finished writing to.
    glBindTexture(GL_TEXTURE_2D,frame_texture);

    glDrawArrays(GL_TRIANGLES, 0, 3);

    glfwSwapBuffers(window);
    glfwPollEvents();

    // Swap for the next frame.
}