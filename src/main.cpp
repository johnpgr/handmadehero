#include "SDL3/SDL_timer.h"
#include "SDL3/SDL_video.h"
#include "glad/glad.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdint.h>

// Useful redefines
#define fn static
#define global static

// Useful macros
#define nanos_to_second(_ns) _ns / 1000000000.0f

// Useful typedefs
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef float f32;
typedef double f64;
typedef size_t usize;

// Compile time variables
constexpr char vs_source[] = {
#embed "shader/vertex.glsl"
};

constexpr char fs_source[] = {
#embed "shader/frag.glsl"
};

// Main application state
global bool running = true;
global SDL_Window* window = nullptr;
global SDL_GLContext gl_context = nullptr;
global i32 w_width = 1280;
global i32 w_height = 720;

// OpenGL objects
global u32 shader_program = 0;
global u32 vao = 0;
global u32 vbo = 0;
global i32 time_uniform = -1;
global i32 resolution_uniform = -1;

fn u32 compile_shader(const GLchar* source, const GLint len, GLenum shader_type) {
    u32 shader = glCreateShader(shader_type);
    glShaderSource(shader, 1, &source, &len);
    glCompileShader(shader);

    i32 success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char info_log[512];
        glGetShaderInfoLog(shader, 512, nullptr, info_log);
        SDL_Log("This shader code is an aberration.\n%s", info_log);
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

fn u32 create_shader_program() {
    u32 vertex_shader = compile_shader(vs_source, sizeof(vs_source), GL_VERTEX_SHADER);
    u32 fragment_shader = compile_shader(fs_source, sizeof(fs_source), GL_FRAGMENT_SHADER);

    if (vertex_shader == 0 || fragment_shader == 0) {
        return 0;
    }

    u32 program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);

    i32 success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char info_log[512];
        glGetProgramInfoLog(program, sizeof(info_log), nullptr, info_log);
        SDL_Log("You are incompetent.\n%s", info_log);
        glDeleteProgram(program);
        program = 0;
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    return program;
}

fn void setup_quad() {
    f32 vertices[] = {
        -1.0f, -1.0f, // bottom left
        1.0f,  -1.0f, // bottom right
        1.0f,  1.0f,  // top right
        -1.0f, 1.0f   // top left
    };

    u32 indices[] = {
        0, 1, 2, // first triangle
        2, 3, 0  // second triangle
    };

    u32 ebo;

    glCreateVertexArrays(1, &vao);
    glCreateBuffers(1, &vbo);
    glCreateBuffers(1, &ebo);

    // Upload vertex data
    glNamedBufferData(vbo, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glNamedBufferData(ebo, sizeof(indices), indices, GL_STATIC_DRAW);

    // Bind buffers to VAO
    glVertexArrayVertexBuffer(vao, 0, vbo, 0, 2 * sizeof(f32));
    glVertexArrayElementBuffer(vao, ebo);

    // Setup vertex attribute
    glEnableVertexArrayAttrib(vao, 0);
    glVertexArrayAttribFormat(vao, 0, 2, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(vao, 0, 0);
}

fn void handle_window_events([[maybe_unused]] u64 current_time_ns) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_EVENT_QUIT:
            running = false;
            break;

        case SDL_EVENT_WINDOW_RESIZED:
            w_width = event.window.data1;
            w_height = event.window.data2;
            glViewport(0, 0, w_width, w_height);
            break;

        case SDL_EVENT_KEY_DOWN:
            switch (event.key.key) {
            case SDLK_ESCAPE:
                running = false;
                break;
            default:
                break;
            }
            break;

        default:
            break;
        }
    }
}

fn void render(u64 current_time_ns) {
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(shader_program);

    f32 time = nanos_to_second(current_time_ns);
    glUniform1f(time_uniform, time);
    glUniform2f(resolution_uniform, (f32)w_width, (f32)w_height);

    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    SDL_GL_SwapWindow(window);
}

fn bool initialize() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("You've failed as a human being.");
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    window = SDL_CreateWindow("Handmade hero SDL3", w_width, w_height, SDL_WINDOW_OPENGL);
    if (!window) {
        SDL_Log("Get a life.");
        return false;
    }

    gl_context = SDL_GL_CreateContext(window);
    if (!gl_context) {
        SDL_Log("You disgust me.");
        return false;
    }

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        SDL_Log("Unlucky, be better.");
        return false;
    }

    // Print OpenGL info for debugging
    SDL_Log("OpenGL Version: %s", (char*)glGetString(GL_VERSION));
    SDL_Log("OpenGL Vendor: %s", (char*)glGetString(GL_VENDOR));
    SDL_Log("OpenGL Renderer: %s", (char*)glGetString(GL_RENDERER));

    if(!SDL_GL_SetSwapInterval(1)) {
        SDL_Log("Couldn't enable VSync");
    }

    glViewport(0, 0, w_width, w_height);

    shader_program = create_shader_program();
    if (shader_program == 0) {
        SDL_Log("Get mental help.");
        return false;
    }

    time_uniform = glGetUniformLocation(shader_program, "uTime");
    resolution_uniform = glGetUniformLocation(shader_program, "uResolution");

    setup_quad();

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    return true;
}

fn void shutdown() {
    if (vao) {
        glDeleteVertexArrays(1, &vao);
    }

    if (vbo) {
        glDeleteBuffers(1, &vbo);
    }

    if (shader_program) {
        glDeleteProgram(shader_program);
    }

    if (window) {
        SDL_DestroyWindow(window);
    }

    if (gl_context) {
        SDL_GL_DestroyContext(gl_context);
    }

    SDL_Quit();
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
    if (!initialize()) {
        return -1;
    }

    while (running) {
        u64 tick = SDL_GetTicksNS();
        handle_window_events(tick);
        render(tick);
    }

    shutdown();

    return 0;
}
