#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <print>
#include <stdint.h>

#define fn static
#define global static

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

using std::println;

global bool running;
global SDL_Window* window;

fn void handle_window_events() {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_EVENT_QUIT:
            running = false;
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

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        println("You've failed as a human being");
        return -1;
    }

    window = SDL_CreateWindow("Handmade hero SDL3", 1280, 720, 0);

    running = true;

    while (running) {
        handle_window_events();
    }

    return 0;
}
