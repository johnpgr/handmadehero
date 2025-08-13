#pragma once
#include "SDL3/SDL_audio.h"
#include "SDL3/SDL_gamepad.h"
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_video.h"
#include "core.h"

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
