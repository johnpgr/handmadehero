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

// Constants
constexpr f32 SAMPLE_RATE = 44100.0;

// Main application state
global SDL_Window* window = nullptr;
global SDL_Renderer* renderer = nullptr;
global SDL_Texture* texture = nullptr;
global SDL_Gamepad* gamepad = nullptr;
global SDL_AudioStream* audio_stream = nullptr;
global f32 tone_hz = 440.0;
global f32 tone_volume = 0.1;
global f32 wave_period = 0.0;
global i32 win_width = 1280;
global i32 win_height = 720;
global i32 texture_width = 0;
global i32 texture_height = 0;
global bool running = true;
global bool win_focused = true;
global bool controller_connected = false;

fn bool resize_texture(i32 width, i32 height) {
    if (texture) {
        SDL_DestroyTexture(texture);
        texture = nullptr;
    }

    texture_width = width;
    texture_height = height;

    texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGBA32,
        SDL_TEXTUREACCESS_STREAMING,
        texture_width,
        texture_height
    );

    if (!texture) {
        SDL_Log("Your GPU is probably older than my grandmother's dentures.");
        return false;
    }

    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);

    return true;
}

fn void render_weird_gradient(u32 blue_offset, u32 green_offset) {
    if (!texture)
        return;

    void* pixels = nullptr;
    i32 pitch = 0;

    if (!SDL_LockTexture(texture, nullptr, &pixels, &pitch)) {
        SDL_Log("You are a failure. %s", SDL_GetError());
        return;
    }

    i32 bytes_per_pixel = 4;
    auto base = (u8*)pixels;

    for (i32 y = 0; y < texture_height; ++y) {
        u8* row = base + y * pitch;

        for (i32 x = 0; x < texture_width; ++x) {
            u8* pixel = row + x * bytes_per_pixel;

            *pixel++ = 0;
            *pixel++ = (u8)((y + green_offset) & 0xFF);
            *pixel++ = (u8)((x + blue_offset) & 0xFF);
            *pixel++ = 255;
        }
    }

    SDL_UnlockTexture(texture);
}

fn void handle_keyboard_input([[maybe_unused]] u64 current_time_ns) {
    const bool* keyboard_state = SDL_GetKeyboardState(nullptr);

    if (keyboard_state[SDL_SCANCODE_ESCAPE]) {
        running = false;
    }

    if (keyboard_state[SDL_SCANCODE_W]) {
        tone_hz += 10.0f;
        if (tone_hz > 2000.0f)
            tone_hz = 2000.0f;
    }

    if (keyboard_state[SDL_SCANCODE_A]) {
    }

    if (keyboard_state[SDL_SCANCODE_S]) {
        tone_hz -= 10.0f;
        if (tone_hz < 100.0f)
            tone_hz = 100.0f;
    }


    if (keyboard_state[SDL_SCANCODE_D]) {
    }

    if (keyboard_state[SDL_SCANCODE_SPACE]) {
    }

    if(keyboard_state[SDL_SCANCODE_1]) {
        tone_hz = 261.63f; // C4
    }

    if(keyboard_state[SDL_SCANCODE_2]) {
        tone_hz = 293.66f; // D4
    }

    if(keyboard_state[SDL_SCANCODE_3]) {
        tone_hz = 329.63f; // E4
    }

    if(keyboard_state[SDL_SCANCODE_4]) {
        tone_hz = 349.23f; // F4
    }

    if(keyboard_state[SDL_SCANCODE_5]) {
        tone_hz = 392.00f; // G4
    }

    if(keyboard_state[SDL_SCANCODE_6]) {
        tone_hz = 440.00f; // A4
    }

    if(keyboard_state[SDL_SCANCODE_7]) {
        tone_hz = 493.88f; // B4
    }

    if(keyboard_state[SDL_SCANCODE_M]) {
        tone_volume = (tone_volume > 0.0f) ? 0.0f : 0.1f;
    }
}

fn void handle_controller_input([[maybe_unused]] u64 current_time_ns) {
    if (!controller_connected)
        return;

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
    i16 left_trigger =
        SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER);
    i16 right_trigger =
        SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER);

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

                resize_texture(win_width, win_height);

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
                        SDL_Log(
                            "Controller connected: %s", name ? name : "Unknown"
                        );
                    }
                }
                break;
            }

            case SDL_EVENT_GAMEPAD_REMOVED: {
                if (gamepad &&
                    event.gdevice.which == SDL_GetGamepadID(gamepad)) {
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

fn void handle_audio_stream([[maybe_unused]] u64 current_time_ns) {
    // 8000 float samples per second. Half of that.
    i32 min_audio = (SAMPLE_RATE * sizeof(f32)) / 50;

    // See if we need to feed the audio stream more data yet.
    // We're being lazy here, but if there's less than half a second queued,
    // generate more. A sine wave is unchanging audio--easy to stream--but for
    // video games, you'll want to generate significantly _less_ audio ahead of
    // time!
    if (SDL_GetAudioStreamQueued(audio_stream) < min_audio) {
        static f32 samples[128];

        u32 sample_count = SDL_arraysize(samples);

        // Generate a 440hz pure tone
        for (u64 i = 0; i < sample_count; i++) {
            f32 sine_value = SDL_sinf(wave_period * 2.0f * SDL_PI_F);
            samples[i] = sine_value * tone_volume;

            // Advance the wave period by the frequency step
            // This is the key insight from Casey's explanation:
            // Each sample advances the phase by (frequency / sample_rate)
            wave_period += tone_hz / SAMPLE_RATE;

            // Keep wave_period in a reasonable range to avoid floating point
            // errors Since sine is periodic, we can wrap around at 1.0
            if (wave_period >= 1.0f) {
                wave_period -= 1.0f;
            }
        }

        SDL_PutAudioStreamData(audio_stream, samples, sizeof(samples));
    }
}

fn void render([[maybe_unused]] u64 current_time_ns) {
    if (!win_focused)
        return;

    static u32 blue_offset = 0;
    static u32 green_offset = 0;

    blue_offset += 1;
    green_offset += 1;

    render_weird_gradient(blue_offset, green_offset);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    if (texture) {
        SDL_RenderTexture(renderer, texture, nullptr, nullptr);
    }
    SDL_RenderPresent(renderer);
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
                    SDL_Log(
                        "Controller connected: %s", name ? name : "Unknown"
                    );
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

    audio_stream = SDL_OpenAudioDeviceStream(
        SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr
    );
    if (!audio_stream) {
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

    window = SDL_CreateWindow(
        "Handmade hero SDL3", win_width, win_height, SDL_WINDOW_RESIZABLE
    );
    if (!window) {
        SDL_Log("Get a life.");
        return false;
    }

    renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer) {
        SDL_Log("It's over for you");
        return false;
    }

    if (!resize_texture(win_width, win_height)) {
        return false;
    }

    initialize_audio();
    initialize_gamepad();

    return true;
}

fn void shutdown() {
    if (audio_stream) {
        SDL_DestroyAudioStream(audio_stream);
    }
    if (gamepad) {
        SDL_CloseGamepad(gamepad);
    }
    if (texture) {
        SDL_DestroyTexture(texture);
    }
    if (window) {
        SDL_DestroyWindow(window);
    }
    if (renderer) {
        SDL_DestroyRenderer(renderer);
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
        handle_keyboard_input(tick);
        handle_controller_input(tick);
        handle_audio_stream(tick);
        render(tick);
    }

    shutdown();

    return 0;
}
