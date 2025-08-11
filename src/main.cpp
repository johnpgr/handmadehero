#include "glad/glad.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdint.h>

// Useful redefines
#define fn static
#define global static

// Useful macros
#define NANOS_TO_SECONDS(ns) ns / 1000000000.0f

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
char vs_source[] = {
#embed "shader/vertex.glsl"
};

char fs_source[] = {
#embed "shader/frag.glsl"
};

// Main application state
global bool running = true;
global SDL_Window* window = nullptr;
global SDL_GLContextState* gl_context = nullptr;
global SDL_AudioStream* audio_stream = nullptr;
global SDL_Gamepad* gamepad = nullptr;
global i32 current_sine_sample = 0;
global i32 win_width = 1280;
global i32 win_height = 720;
global bool win_focused = true;
global bool controller_connected = false;

// OpenGL objects
global u32 shader_program = 0;
global u32 vao = 0;
global u32 vbo = 0;
global u32 ebo = 0;
global i32 time_uniform = -1;
global i32 resolution_uniform = -1;

fn u32 compile_shader(GLchar* source, GLint len, GLenum shader_type) {
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

fn void initialize_quad() {
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

fn void handle_keyboard_input() {
    const bool* keyboard_state = SDL_GetKeyboardState(nullptr);
    
    if (keyboard_state[SDL_SCANCODE_ESCAPE]) {
        running = false;
    }
    
    // Example of other keyboard polling
    if (keyboard_state[SDL_SCANCODE_W]) {
        // Move forward or whatever
    }
    
    if (keyboard_state[SDL_SCANCODE_A]) {
        // Move left
    }
    
    if (keyboard_state[SDL_SCANCODE_S]) {
        // Move backward  
    }
    
    if (keyboard_state[SDL_SCANCODE_D]) {
        // Move right
    }
    
    if (keyboard_state[SDL_SCANCODE_SPACE]) {
        // Jump or action
    }
}

fn void handle_controller_input() {
    if (!controller_connected) return;
    
    // Buttons (current state, not just pressed this frame)
    if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_SOUTH)) {
        // SDL_Log("A button is being held down");
    }
    
    if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_EAST)) {
        // SDL_Log("B button is being held down");
    }
    
    if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_START)) {
        running = false;
    }
    
    // D-pad
    if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_UP)) {
        // SDL_Log("D-pad up");
    }
    
    if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_DOWN)) {
        // SDL_Log("D-pad down");
    }
    
    if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_LEFT)) {
        // SDL_Log("D-pad left");
    }
    
    if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_RIGHT)) {
        // SDL_Log("D-pad right");
    }
    
    // Shoulder buttons
    if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER)) {
        // SDL_Log("Left bumper");
    }
    
    if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER)) {
        // SDL_Log("Right bumper");
    }
    
    // Analog sticks with deadzone
    const i16 DEADZONE = 8000;
    
    // Left stick
    i16 left_x = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTX);
    i16 left_y = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTY);
    
    if (abs(left_x) > DEADZONE || abs(left_y) > DEADZONE) {
        f32 norm_left_x = left_x / 32767.0f;
        f32 norm_left_y = left_y / 32767.0f;
        
        // Use normalized values for player movement, camera, etc.
        SDL_Log("Left stick: (%.2f, %.2f)", norm_left_x, norm_left_y);
    }
    
    // Right stick
    i16 right_x = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHTX);
    i16 right_y = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHTY);
    
    if (abs(right_x) > DEADZONE || abs(right_y) > DEADZONE) {
        f32 norm_right_x = right_x / 32767.0f;
        f32 norm_right_y = right_y / 32767.0f;
        
        // Use for camera control, aiming, etc.
        SDL_Log("Right stick: (%.2f, %.2f)", norm_right_x, norm_right_y);
    }
    
    // Triggers (0 to 32767)
    i16 left_trigger = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER);
    i16 right_trigger = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER);
    
    const i16 TRIGGER_THRESHOLD = 4000;
    
    if (left_trigger > TRIGGER_THRESHOLD) {
        [[maybe_unused]] f32 norm_left_trigger = left_trigger / 32767.0f;
        // Use for analog actions like acceleration, zoom, etc.
    }
    
    if (right_trigger > TRIGGER_THRESHOLD) {
        [[maybe_unused]] f32 norm_right_trigger = right_trigger / 32767.0f;
        // Use for analog actions
    }
}

fn void handle_window_events([[maybe_unused]] u64 current_time_ns) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_EVENT_QUIT: {
                running = false;
                break;
            }

            case SDL_EVENT_WINDOW_RESIZED: {
                win_width = event.window.data1;
                win_height = event.window.data2;
                glViewport(0, 0, win_width, win_height);
                break;
            }

            case SDL_EVENT_WINDOW_FOCUS_LOST: {
                win_focused = false;
                break;
            }

            case SDL_EVENT_WINDOW_FOCUS_GAINED: {
                win_focused = true;
                break;
            }

            case SDL_EVENT_GAMEPAD_ADDED: {
                if (!controller_connected) {
                    gamepad = SDL_OpenGamepad(event.gdevice.which);
                    if (gamepad) {
                        controller_connected = true;
                        const char* name = SDL_GetGamepadName(gamepad);
                        SDL_Log("Controller connected: %s", name ? name : "Unknown");
                    }
                }
                break;
            }

            case SDL_EVENT_GAMEPAD_REMOVED: {
                if (gamepad && event.gdevice.which == SDL_GetGamepadID(gamepad)) {
                    SDL_CloseGamepad(gamepad);
                    gamepad = nullptr;
                    controller_connected = false;
                    SDL_Log("Controller disconnected");
                }
                break;
            }
        }
    }
}

fn void handle_audio_stream() {
    // 8000 float samples per second. Half of that.
    int min_audio = (8000 * sizeof(f32)) / 2;

    // See if we need to feed the audio stream more data yet.
    // We're being lazy here, but if there's less than half a second queued, generate more.
    // A sine wave is unchanging audio--easy to stream--but for video games, you'll want
    // to generate significantly _less_ audio ahead of time!
    if(SDL_GetAudioStreamQueued(audio_stream) < min_audio) {
        static f32 samples[512];

        // Generate a 440hz pure tone
        for (i32 i = 0; i < SDL_arraysize(samples); i++) {
            int freq = 440;
            f32 phase = current_sine_sample * freq / 8000.0f;
            samples[i] = SDL_sinf(phase * 2 * SDL_PI_F);
            current_sine_sample++;
        }

        // Wrapping around to avoid floating-point errors
        current_sine_sample %= 8000;
        
        SDL_PutAudioStreamData(audio_stream, samples, sizeof(samples));
    }
}

fn void render(u64 current_time_ns) {
    if(!win_focused) return;

    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(shader_program);

    f32 time = NANOS_TO_SECONDS(current_time_ns);
    glUniform1f(time_uniform, time);
    glUniform2f(resolution_uniform, (f32)win_width, (f32)win_height);

    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    SDL_GL_SwapWindow(window);
}

fn void initialize_gamepad() {
    i32 n_joysticks;
    SDL_JoystickID* joysticks = SDL_GetJoysticks(&n_joysticks);

    if (n_joysticks > 0) {
        for (i32 i = 0; i < n_joysticks; i++) {
            if (SDL_IsGamepad(joysticks[i])) {
                gamepad = SDL_OpenGamepad(joysticks[i]);
                if (gamepad) {
                    controller_connected = true;
                    const char* name = SDL_GetGamepadName(gamepad);
                    SDL_Log("Controller connected: %s", name ? name : "Unknown");
                    break;
                }
            }
        }
        SDL_free(joysticks);
    }

    if (!controller_connected) {
        SDL_Log("No controller detected");
    }
}

fn bool initialize_audio() {
    SDL_AudioSpec spec;
    spec.channels = 1;
    spec.format = SDL_AUDIO_F32;
    spec.freq = 8000;

    audio_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
    if(!audio_stream) {
        SDL_Log("You will die in misery");
        return false;
    }

    SDL_ResumeAudioStreamDevice(audio_stream);

    return true;
}

fn bool initialize() {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD)) {
        SDL_Log("You've failed as a human being.");
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    window = SDL_CreateWindow("Handmade hero SDL3", win_width, win_height, SDL_WINDOW_OPENGL);
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

    if (!SDL_GL_SetSwapInterval(1)) {
        SDL_Log("Couldn't enable VSync");
    }

    glViewport(0, 0, win_width, win_height);

    shader_program = create_shader_program();
    if (shader_program == 0) {
        SDL_Log("Get mental help.");
        return false;
    }

    time_uniform = glGetUniformLocation(shader_program, "uTime");
    resolution_uniform = glGetUniformLocation(shader_program, "uResolution");

    initialize_quad();
    initialize_audio();
    initialize_gamepad();

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    return true;
}

fn void shutdown() {
    if (gamepad) {
        SDL_CloseGamepad(gamepad);
    }
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
        handle_keyboard_input();
        handle_controller_input();
        handle_audio_stream();
        render(tick);
    }

    shutdown();

    return 0;
}
