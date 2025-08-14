#include "core.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

constexpr f32 SAMPLE_RATE = 48000.0;
constexpr i16 TRIGGER_THRESHOLD = 4000;
constexpr i16 DEADZONE = 8000;

struct GameInput {
    SDL_Gamepad* gamepad = nullptr;
    bool controller_connected = false;
};

struct GameSound {
    SDL_AudioStream* audio_stream = nullptr;
    f32 tone_volume = 0.1;
    f32 wave_period = 0.0;
};

struct Game {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;
    GameInput input = {};
    GameSound sound = {};
    i32 win_width = 1280;
    i32 win_height = 720;
    bool win_focused = true;
    bool running = true;
};

struct GameState {
    i32 blue_offset = 0;
    i32 green_offset = 0;
    f32 tone_hz = 440.0;
};

static Game game = {};

bool resize_texture(i32 width, i32 height) {
    if (game.texture) {
        SDL_DestroyTexture(game.texture);
        game.texture = nullptr;
    }

    game.texture = SDL_CreateTexture(
        game.renderer,
        SDL_PIXELFORMAT_BGRX32,
        SDL_TEXTUREACCESS_STREAMING,
        width,
        height
    );

    if (!game.texture) {
        SDL_Log("Your GPU is probably older than my grandmother's dentures.");
        return false;
    }

    SDL_SetTextureScaleMode(game.texture, SDL_SCALEMODE_NEAREST);

    return true;
}

void render_weird_gradient(GameState* state) {
    if (!game.texture) return;

    void* pixels = nullptr;
    i32 pitch = 0;

    if (!SDL_LockTexture(game.texture, nullptr, &pixels, &pitch)) {
        SDL_Log("You are a failure. %s", SDL_GetError());
        return;
    }

    f32 texture_width, texture_height;
    if (!SDL_GetTextureSize(game.texture, &texture_width, &texture_height)) {
        SDL_UnlockTexture(game.texture);
        return;
    }

    u8* row = (u8*)pixels;

    for (i32 y = 0; y < (i32)texture_height; ++y) {
        u32* pixel = (u32*)row;

        for (i32 x = 0; x < (i32)texture_width; ++x) {
            u8 blue = (x + state->blue_offset);
            u8 green = (y + state->green_offset);

            *pixel++ = ((green << 8) | blue);
        }

        row += pitch;
    }

    SDL_UnlockTexture(game.texture);
}

void handle_keyboard_input(GameState* state) {
    const bool* keyboard_state = SDL_GetKeyboardState(nullptr);

    if (keyboard_state[SDL_SCANCODE_ESCAPE]) {
        game.running = false;
    }

    if (keyboard_state[SDL_SCANCODE_W]) {
        state->tone_hz += 10.0f;
        if (state->tone_hz > 2000.0f) state->tone_hz = 2000.0f;
    }

    if (keyboard_state[SDL_SCANCODE_A]) {
    }

    if (keyboard_state[SDL_SCANCODE_S]) {
        state->tone_hz -= 10.0f;
        if (state->tone_hz < 100.0f) state->tone_hz = 100.0f;
    }

    if (keyboard_state[SDL_SCANCODE_D]) {
    }

    if (keyboard_state[SDL_SCANCODE_SPACE]) {
    }

    if (keyboard_state[SDL_SCANCODE_1]) {
        state->tone_hz = 261.63f; // C4
    }

    if (keyboard_state[SDL_SCANCODE_2]) {
        state->tone_hz = 293.66f; // D4
    }

    if (keyboard_state[SDL_SCANCODE_3]) {
        state->tone_hz = 329.63f; // E4
    }

    if (keyboard_state[SDL_SCANCODE_4]) {
        state->tone_hz = 349.23f; // F4
    }

    if (keyboard_state[SDL_SCANCODE_5]) {
        state->tone_hz = 392.00f; // G4
    }

    if (keyboard_state[SDL_SCANCODE_6]) {
        state->tone_hz = 440.00f; // A4
    }

    if (keyboard_state[SDL_SCANCODE_7]) {
        state->tone_hz = 493.88f; // B4
    }

    if (keyboard_state[SDL_SCANCODE_M]) {
        game.sound.tone_volume = 0.0;
    }
    if (keyboard_state[SDL_SCANCODE_U]) {
        game.sound.tone_volume = 0.1;
    }
}

void handle_controller_input([[maybe_unused]] GameState* state) {
    if (!game.input.controller_connected) return;

    // Buttons (current state, not just pressed this frame)
    if (SDL_GetGamepadButton(game.input.gamepad, SDL_GAMEPAD_BUTTON_SOUTH)) {
        // SDL_Log("A button is being held down");
    }

    if (SDL_GetGamepadButton(game.input.gamepad, SDL_GAMEPAD_BUTTON_EAST)) {
        // SDL_Log("B button is being held down");
    }

    if (SDL_GetGamepadButton(game.input.gamepad, SDL_GAMEPAD_BUTTON_START)) {
        game.running = false;
    }

    // D-pad
    if (SDL_GetGamepadButton(game.input.gamepad, SDL_GAMEPAD_BUTTON_DPAD_UP)) {
        // SDL_Log("D-pad up");
    }

    if (SDL_GetGamepadButton(
            game.input.gamepad,
            SDL_GAMEPAD_BUTTON_DPAD_DOWN
        )) {
        // SDL_Log("D-pad down");
    }

    if (SDL_GetGamepadButton(
            game.input.gamepad,
            SDL_GAMEPAD_BUTTON_DPAD_LEFT
        )) {
        // SDL_Log("D-pad left");
    }

    if (SDL_GetGamepadButton(
            game.input.gamepad,
            SDL_GAMEPAD_BUTTON_DPAD_RIGHT
        )) {
        // SDL_Log("D-pad right");
    }

    // Shoulder buttons
    if (SDL_GetGamepadButton(
            game.input.gamepad,
            SDL_GAMEPAD_BUTTON_LEFT_SHOULDER
        )) {
        // SDL_Log("Left bumper");
    }

    if (SDL_GetGamepadButton(
            game.input.gamepad,
            SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER
        )) {
        // SDL_Log("Right bumper");
    }

    // Left stick
    i16 left_x = SDL_GetGamepadAxis(game.input.gamepad, SDL_GAMEPAD_AXIS_LEFTX);
    i16 left_y = SDL_GetGamepadAxis(game.input.gamepad, SDL_GAMEPAD_AXIS_LEFTY);

    if (abs(left_x) > DEADZONE || abs(left_y) > DEADZONE) {
        f32 norm_left_x = left_x / 32767.0f;
        f32 norm_left_y = left_y / 32767.0f;

        state->blue_offset +=
            (i32)(norm_left_x * 5.0f); // X controls blue offset
        state->green_offset +=
            (i32)(norm_left_y * 5.0f); // Y controls green offset

        // Use normalized values for player movement, camera, etc.
        // SDL_Log("Left stick: (%.2f, %.2f)", norm_left_x, norm_left_y);
    }

    // Right stick
    i16 right_x =
        SDL_GetGamepadAxis(game.input.gamepad, SDL_GAMEPAD_AXIS_RIGHTX);
    i16 right_y =
        SDL_GetGamepadAxis(game.input.gamepad, SDL_GAMEPAD_AXIS_RIGHTY);

    if (abs(right_x) > DEADZONE || abs(right_y) > DEADZONE) {
        [[maybe_unused]] f32 norm_right_x = right_x / 32767.0f;
        f32 norm_right_y = right_y / 32767.0f;

        state->tone_hz -= norm_right_y * 20.0f;
        if (state->tone_hz < 100.0f) state->tone_hz = 100.0f;
        if (state->tone_hz > 2000.0f) state->tone_hz = 2000.0f;

        // Use for camera control, aiming, etc.
        // SDL_Log("Right stick: (%.2f, %.2f)", norm_right_x, norm_right_y);
    }

    // Triggers (0 to 32767)
    i16 left_trigger =
        SDL_GetGamepadAxis(game.input.gamepad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER);
    i16 right_trigger =
        SDL_GetGamepadAxis(game.input.gamepad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER);

    if (left_trigger > TRIGGER_THRESHOLD) {
        [[maybe_unused]] f32 norm_left_trigger = left_trigger / 32767.0f;
        // Use for analog actions like acceleration, zoom, etc.
    }

    if (right_trigger > TRIGGER_THRESHOLD) {
        [[maybe_unused]] f32 norm_right_trigger = right_trigger / 32767.0f;
        // Use for analog actions
    }
}

void handle_window_events([[maybe_unused]] GameState* state) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_EVENT_QUIT: {
                game.running = false;
                break;
            }

            case SDL_EVENT_WINDOW_RESIZED: {
                game.win_width = event.window.data1;
                game.win_height = event.window.data2;

                resize_texture(game.win_width, game.win_height);

                break;
            }

            case SDL_EVENT_WINDOW_FOCUS_LOST: {
                game.win_focused = false;
                break;
            }

            case SDL_EVENT_WINDOW_FOCUS_GAINED: {
                game.win_focused = true;
                break;
            }

            case SDL_EVENT_GAMEPAD_ADDED: {
                if (!game.input.controller_connected) {
                    game.input.gamepad = SDL_OpenGamepad(event.gdevice.which);
                    if (game.input.gamepad) {
                        game.input.controller_connected = true;
                        const char* name =
                            SDL_GetGamepadName(game.input.gamepad);
                        SDL_Log(
                            "Controller connected: %s",
                            name ? name : "Unknown"
                        );
                    }
                }
                break;
            }

            case SDL_EVENT_GAMEPAD_REMOVED: {
                if (game.input.gamepad &&
                    event.gdevice.which ==
                        SDL_GetGamepadID(game.input.gamepad)) {
                    SDL_CloseGamepad(game.input.gamepad);
                    game.input.gamepad = nullptr;
                    game.input.controller_connected = false;
                    SDL_Log("Controller disconnected");
                }
                break;
            }
        }
    }
}

void handle_audio_stream(GameState* state) {
    if (!game.sound.audio_stream) return;

    // Target ~100 ms of queued audio to avoid underruns.
    constexpr f32 TARGET_SECONDS = 0.10;
    i32 target_bytes = (i32)(SAMPLE_RATE * sizeof(f32) * TARGET_SECONDS);

    f32 samples[512];
    u32 sample_count = SDL_arraysize(samples);

    // See if we need to feed the audio stream more data yet.
    // We're being lazy here, but if there's less than half a second queued,
    // generate more. A sine wave is unchanging audio--easy to stream--but for
    // video games, you'll want to generate significantly _less_ audio ahead of
    // time!
    if (SDL_GetAudioStreamQueued(game.sound.audio_stream) < target_bytes) {
        // Generate a 440hz pure tone
        for (u32 i = 0; i < sample_count; i++) {
            f32 sine_value = SDL_sinf(game.sound.wave_period * 2.0f * SDL_PI_F);
            samples[i] = sine_value * game.sound.tone_volume;

            // Advance the wave period by the frequency step
            // This is the key insight from Casey's explanation:
            // Each sample advances the phase by (frequency / sample_rate)
            game.sound.wave_period += state->tone_hz / SAMPLE_RATE;

            // Keep wave_period in a reasonable range to avoid floating point
            // errors Since sine is periodic, we can wrap around at 1.0
            if (game.sound.wave_period >= 1.0f) {
                game.sound.wave_period -= 1.0f;
            }
        }

        SDL_PutAudioStreamData(
            game.sound.audio_stream,
            samples,
            sizeof(samples)
        );
    }
}

void render(GameState* state) {
    if (!game.win_focused) return;

    render_weird_gradient(state);

    SDL_SetRenderDrawColor(game.renderer, 0, 0, 0, 255);
    SDL_RenderClear(game.renderer);
    if (game.texture) {
        SDL_RenderTexture(game.renderer, game.texture, nullptr, nullptr);
    }
    SDL_RenderPresent(game.renderer);
}

void initialize_gamepad() {
    i32 n_joysticks;
    SDL_JoystickID* joysticks = SDL_GetJoysticks(&n_joysticks);

    if (n_joysticks > 0) {
        for (i32 i = 0; i < n_joysticks; i++) {
            if (SDL_IsGamepad(joysticks[i])) {
                game.input.gamepad = SDL_OpenGamepad(joysticks[i]);
                if (game.input.gamepad) {
                    game.input.controller_connected = true;
                    const char* name = SDL_GetGamepadName(game.input.gamepad);
                    SDL_Log(
                        "Controller connected: %s",
                        name ? name : "Unknown"
                    );
                    break;
                }
            }
        }
        SDL_free(joysticks);
    }

    if (!game.input.controller_connected) {
        SDL_Log("No controller detected");
    }
}

bool initialize_audio() {
    SDL_AudioSpec spec = {};
    spec.format = SDL_AUDIO_F32;
    spec.channels = 1;
    spec.freq = SAMPLE_RATE;

    game.sound.audio_stream = SDL_OpenAudioDeviceStream(
        SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
        &spec,
        nullptr,
        nullptr
    );
    if (!game.sound.audio_stream) {
        SDL_Log("You will die in misery");
        return false;
    }

    SDL_ResumeAudioStreamDevice(game.sound.audio_stream);

    return true;
}

bool initialize() {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD)) {
        SDL_Log("You've failed as a human being.");
        return false;
    }

    SDL_CreateWindowAndRenderer(
        "Handmade hero SDL3",
        game.win_width,
        game.win_height,
        SDL_WINDOW_RESIZABLE,
        &game.window,
        &game.renderer
    );

    if (!game.window || !game.renderer) {
        SDL_Log("Maybe you should buy a new computer.");
        return false;
    }

    if (!SDL_SetRenderVSync(game.renderer, 1)) {
        SDL_Log("Warning: Unable to enable VSync: %s", SDL_GetError());
    }

    if (!resize_texture(game.win_width, game.win_height)) {
        return false;
    }

    initialize_audio();
    initialize_gamepad();

    return true;
}

void shutdown() {
    SDL_Log("Running shutdown logic");

    if (game.sound.audio_stream) {
        SDL_DestroyAudioStream(game.sound.audio_stream);
    }
    if (game.input.gamepad) {
        SDL_CloseGamepad(game.input.gamepad);
    }
    if (game.texture) {
        SDL_DestroyTexture(game.texture);
    }
    if (game.window) {
        SDL_DestroyWindow(game.window);
    }
    if (game.renderer) {
        SDL_DestroyRenderer(game.renderer);
    }

    SDL_Quit();
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
    if (!initialize()) return -1;
    defer { shutdown(); };

    auto persistent_storage = FixedBufferAllocator::create(MB(64));
    defer { persistent_storage.destroy(); };

    auto transient_storage = FixedBufferAllocator::create(MB(64));
    defer { transient_storage.destroy(); };

    auto state = persistent_storage.alloc_initialized<GameState>();

    while (game.running) {
        u64 frame_start_ns = SDL_GetTicksNS();

        handle_window_events(state);
        handle_keyboard_input(state);
        handle_controller_input(state);
        handle_audio_stream(state);
        render(state);

        u64 frame_end_ns = SDL_GetTicksNS();
        [[maybe_unused]] u64 frame_ns = frame_end_ns - frame_start_ns;
    }

    return 0;
}
