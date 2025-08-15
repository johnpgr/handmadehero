#include "core.h"

constexpr f32 STEP_SIZE = 1.0f;
constexpr f32 SAMPLE_RATE = 48000.0;
constexpr i16 DEADZONE = 8000;
constexpr u8 TONES_LEN = 7;
constexpr f32 TONES[TONES_LEN] = {
    261.63f,
    293.66f,
    329.63f,
    349.23f,
    392.00f,
    440.00f,
    493.88f,
};

struct ButtonState {
    bool ended_down = false;
    bool half_transition_count = false;

    ButtonState() = default;

    fn was_pressed() -> bool { return ended_down && half_transition_count; }

    fn was_released() -> bool { return !ended_down && half_transition_count; }

    fn process_button_state(ButtonState* prev_state, bool is_down) -> void {
        ended_down = is_down;
        half_transition_count =
            (prev_state->ended_down != ended_down) ? true : false;
    }
};

struct GameControllerInput {
    bool is_connected = false;
    bool is_analog = false;

    f32 stick_average_x = 0.0f;
    f32 stick_average_y = 0.0f;

    union {
        ButtonState buttons[12] = {};
        struct {
            ButtonState move_up;
            ButtonState move_down;
            ButtonState move_left;
            ButtonState move_right;

            ButtonState action_up;
            ButtonState action_down;
            ButtonState action_left;
            ButtonState action_right;

            ButtonState left_shoulder;
            ButtonState right_shoulder;

            ButtonState back;
            ButtonState start;
        };
    };
};

struct GameInput {
    SDL_Gamepad* gamepad = nullptr;
    bool controller_connected = false;

    f32 dt_for_frame = 0.0f;

    union {
        GameControllerInput controllers[5] = {};

        struct {
            GameControllerInput keyboard_input;
            GameControllerInput controller_input0;
            GameControllerInput controller_input1;
            GameControllerInput controller_input2;
            GameControllerInput controller_input3;
        };
    };
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
    f32 tone_hz = 440.0f;
    u8 preset_tones_idx = 5; // 440.0f;
};

static Game game = {};

fn resize_texture(i32 width, i32 height) -> bool {
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

fn render_weird_gradient(GameState* state) -> void {
    if (!game.texture)
        return;

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

fn handle_input(
    GameInput* prev_input,
    GameInput* curr_input,
    [[maybe_unused]] GameState* state
) -> void {
    // Copy old input
    *curr_input = *prev_input;

    // Clear transition counts for new frame
    for (i32 controller_index = 0; controller_index < 5; ++controller_index) {
        GameControllerInput* controller =
            &curr_input->controllers[controller_index];
        for (i32 button_index = 0; button_index < 12; ++button_index) {
            controller->buttons[button_index].half_transition_count = 0;
        }
    }

    GameControllerInput* keyboard_input_prev = &prev_input->keyboard_input;
    GameControllerInput* keyboard_input = &curr_input->keyboard_input;
    keyboard_input->is_connected = true;
    keyboard_input->is_analog = false;

    const bool* keyboard_state = SDL_GetKeyboardState(nullptr);

    keyboard_input->move_up.process_button_state(
        &keyboard_input_prev->move_up,
        keyboard_state[SDL_SCANCODE_W]
    );

    keyboard_input->move_down.process_button_state(
        &keyboard_input_prev->move_down,
        keyboard_state[SDL_SCANCODE_S]
    );

    keyboard_input->move_left.process_button_state(
        &keyboard_input_prev->move_left,
        keyboard_state[SDL_SCANCODE_A]
    );

    keyboard_input->move_right.process_button_state(
        &keyboard_input_prev->move_right,
        keyboard_state[SDL_SCANCODE_D]
    );

    keyboard_input->action_up.process_button_state(
        &keyboard_input_prev->action_up,
        keyboard_state[SDL_SCANCODE_UP]
    );

    keyboard_input->action_down.process_button_state(
        &keyboard_input_prev->action_down,
        keyboard_state[SDL_SCANCODE_DOWN]
    );

    keyboard_input->action_left.process_button_state(
        &keyboard_input_prev->action_left,
        keyboard_state[SDL_SCANCODE_LEFT]
    );

    keyboard_input->action_right.process_button_state(
        &keyboard_input_prev->action_right,
        keyboard_state[SDL_SCANCODE_RIGHT]
    );

    keyboard_input->start.process_button_state(
        &keyboard_input_prev->start,
        keyboard_state[SDL_SCANCODE_ESCAPE]
    );

    keyboard_input->start.process_button_state(
        &keyboard_input_prev->start,
        keyboard_state[SDL_SCANCODE_ESCAPE]
    );

    // TODO: Handle the other controllers
    if (game.input.controller_connected && game.input.gamepad) {
        GameControllerInput* gamepad_controller_prev =
            &prev_input->controller_input0;
        GameControllerInput* gamepad_controller =
            &curr_input->controller_input0;
        gamepad_controller->is_connected = true;
        gamepad_controller->is_analog = true;

        gamepad_controller->move_up.process_button_state(
            &gamepad_controller_prev->move_up,
            SDL_GetGamepadButton(game.input.gamepad, SDL_GAMEPAD_BUTTON_DPAD_UP)
        );

        gamepad_controller->move_down.process_button_state(
            &gamepad_controller_prev->move_down,
            SDL_GetGamepadButton(
                game.input.gamepad,
                SDL_GAMEPAD_BUTTON_DPAD_DOWN
            )
        );

        gamepad_controller->move_left.process_button_state(
            &gamepad_controller_prev->move_left,
            SDL_GetGamepadButton(
                game.input.gamepad,
                SDL_GAMEPAD_BUTTON_DPAD_LEFT
            )
        );

        gamepad_controller->move_right.process_button_state(
            &gamepad_controller_prev->move_right,
            SDL_GetGamepadButton(
                game.input.gamepad,
                SDL_GAMEPAD_BUTTON_DPAD_RIGHT
            )
        );

        gamepad_controller->action_up.process_button_state(
            &gamepad_controller_prev->action_up,
            SDL_GetGamepadButton(game.input.gamepad, SDL_GAMEPAD_BUTTON_NORTH)
        );

        gamepad_controller->action_down.process_button_state(
            &gamepad_controller_prev->action_down,
            SDL_GetGamepadButton(game.input.gamepad, SDL_GAMEPAD_BUTTON_SOUTH)
        );

        gamepad_controller->action_left.process_button_state(
            &gamepad_controller_prev->action_left,
            SDL_GetGamepadButton(game.input.gamepad, SDL_GAMEPAD_BUTTON_WEST)
        );

        gamepad_controller->action_right.process_button_state(
            &gamepad_controller_prev->action_right,
            SDL_GetGamepadButton(game.input.gamepad, SDL_GAMEPAD_BUTTON_EAST)
        );

        gamepad_controller->left_shoulder.process_button_state(
            &gamepad_controller_prev->left_shoulder,
            SDL_GetGamepadButton(
                game.input.gamepad,
                SDL_GAMEPAD_BUTTON_LEFT_SHOULDER
            )
        );

        gamepad_controller->right_shoulder.process_button_state(
            &gamepad_controller_prev->right_shoulder,
            SDL_GetGamepadButton(
                game.input.gamepad,
                SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER
            )
        );

        gamepad_controller->start.process_button_state(
            &gamepad_controller_prev->start,
            SDL_GetGamepadButton(game.input.gamepad, SDL_GAMEPAD_BUTTON_START)
        );

        i16 left_x =
            SDL_GetGamepadAxis(game.input.gamepad, SDL_GAMEPAD_AXIS_LEFTX);
        i16 left_y =
            SDL_GetGamepadAxis(game.input.gamepad, SDL_GAMEPAD_AXIS_LEFTY);
        [[maybe_unused]] i16 right_x =
            SDL_GetGamepadAxis(game.input.gamepad, SDL_GAMEPAD_AXIS_RIGHTX);
        [[maybe_unused]] i16 right_y =
            SDL_GetGamepadAxis(game.input.gamepad, SDL_GAMEPAD_AXIS_RIGHTY);

        if (abs(left_x) > DEADZONE || abs(left_y) > DEADZONE) {
            gamepad_controller->stick_average_x = left_x / 32767.0f;
            gamepad_controller->stick_average_y = left_y / 32767.0f;
        } else {
            gamepad_controller->stick_average_x = 0.0f;
            gamepad_controller->stick_average_y = 0.0f;
        }
    }
}

fn update(GameInput* input, GameState* state) -> void {
    for (i32 controller_index = 0; controller_index < 5; ++controller_index) {
        GameControllerInput* controller = &input->controllers[controller_index];

        if (!controller->is_connected)
            continue;

        f32 move_x = 0.0f;
        f32 move_y = 0.0f;

        if (controller->is_analog) {
            move_x = controller->stick_average_x;
            move_y = controller->stick_average_y;
        }

        if (controller->move_left.ended_down)
            move_x = -1.0f;
        if (controller->move_right.ended_down)
            move_x = 1.0f;
        if (controller->move_up.ended_down)
            move_y = -1.0f;
        if (controller->move_down.ended_down)
            move_y = 1.0f;

        state->blue_offset += (i32)(move_x * STEP_SIZE * 5.0f);
        state->green_offset += (i32)(move_y * STEP_SIZE * 5.0f);

        if (controller->action_up.ended_down) {
            state->tone_hz += 10.0f;
            if (state->tone_hz > 2000.0f)
                state->tone_hz = 2000.0f;
        }

        if (controller->action_down.ended_down) {
            state->tone_hz -= 10.0f;
            if (state->tone_hz < 100.0f)
                state->tone_hz = 100.0f;
        }

        if (controller->start.was_pressed()) {
            game.running = false;
        }

        if (controller->action_right.was_pressed()) {
            if (state->preset_tones_idx + 1 >= TONES_LEN) {
                state->preset_tones_idx = 0;
            } else {
                state->preset_tones_idx += 1;
            }

            state->tone_hz = TONES[state->preset_tones_idx];
        }

        if (controller->action_left.was_pressed()) {
            if (state->preset_tones_idx == 0) {
                state->preset_tones_idx = TONES_LEN - 1;
            } else {
                state->preset_tones_idx -= 1;
            }

            state->tone_hz = TONES[state->preset_tones_idx];
        }

        const bool* keyboard_state = SDL_GetKeyboardState(nullptr);
        if (keyboard_state[SDL_SCANCODE_M]) {
            game.sound.tone_volume = 0.0;
        }

        if (keyboard_state[SDL_SCANCODE_U]) {
            game.sound.tone_volume = 0.1;
        }
    }
}

fn handle_window_events([[maybe_unused]] GameState* state) -> void {
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

fn handle_audio_stream(GameState* state) -> void {
    if (!game.sound.audio_stream)
        return;

    // Target ~100 ms of queued audio to avoid underruns.
    constexpr f32 TARGET_SECONDS = 0.10;
    constexpr usize SAMPLE_COUNT = 512;

    f32 samples[SAMPLE_COUNT];
    i32 target_bytes = (i32)(SAMPLE_RATE * sizeof(f32) * TARGET_SECONDS);

    // See if we need to feed the audio stream more data yet.
    // We're being lazy here, but if there's less than half a second queued,
    // generate more. A sine wave is unchanging audio--easy to stream--but for
    // video games, you'll want to generate significantly _less_ audio ahead of
    // time!
    if (SDL_GetAudioStreamQueued(game.sound.audio_stream) < target_bytes) {
        // Generate a 440hz pure tone
        for (u32 i = 0; i < SAMPLE_COUNT; i++) {
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

fn render(GameState* state) -> void {
    if (!game.win_focused)
        return;

    render_weird_gradient(state);

    SDL_SetRenderDrawColor(game.renderer, 0, 0, 0, 255);
    SDL_RenderClear(game.renderer);
    if (game.texture) {
        SDL_RenderTexture(game.renderer, game.texture, nullptr, nullptr);
    }
    SDL_RenderPresent(game.renderer);
}

fn initialize_gamepad() -> void {
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

fn initialize_audio() -> bool {
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

fn initialize() -> bool {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD)) {
        SDL_Log("You've failed as a human being.");
        return false;
    }

    SDL_CreateWindowAndRenderer(
        "Handmade hero SDL3",
        game.win_width,
        game.win_height,
        0,
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

fn shutdown() -> void {
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
    if (!initialize())
        return -1;
    defer { shutdown(); };

    let persistent_storage = FixedBufferAllocator::create(MB(64));
    defer { persistent_storage.destroy(); };

    let transient_storage = FixedBufferAllocator::create(MB(64));
    defer { transient_storage.destroy(); };

    let state = persistent_storage.alloc_initialized<GameState>();

    let prev_input = transient_storage.alloc_initialized<GameInput>();
    let curr_input = transient_storage.alloc_initialized<GameInput>();

    while (game.running) {
        u64 frame_start_ns = SDL_GetTicksNS();

        handle_window_events(state);
        handle_input(prev_input, curr_input, state);
        update(curr_input, state);
        handle_audio_stream(state);
        render(state);

        GameInput* temp = prev_input;
        prev_input = curr_input;
        curr_input = temp;

        u64 frame_end_ns = SDL_GetTicksNS();
        [[maybe_unused]] u64 frame_ns = frame_end_ns - frame_start_ns;
    }

    return 0;
}
