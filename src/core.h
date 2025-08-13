#pragma once

#include "SDL3/SDL_assert.h"
#include "SDL3/SDL_log.h"
#include <cassert>
#include <utility>

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

// Useful macros
#define BIT(x) 1 << (x)
#define KB(x) ((usize)1024 * x)
#define MB(x) ((usize)1024 * KB(x))
#define GB(x) ((usize)1024 * MB(x))
#define NANOS_TO_SECONDS(ns) ns / 1000000000.0f

template <typename F> struct Defer {
    Defer(F f) : f(f) {}
    ~Defer() { f(); }
    F f;
};

template <typename F> Defer<F> makeDefer(F f) { return Defer<F>(f); };

#define __defer(line) defer_##line
#define _defer(line) __defer(line)

struct defer_dummy {};
template <typename F> Defer<F> operator+(defer_dummy, F&& f) {
    return makeDefer<F>(std::forward<F>(f));
}

#define defer auto _defer(__LINE__) = defer_dummy() + [&]()

struct FixedBufferAllocator {
    usize capacity;
    usize used;
    u8* memory;

    static FixedBufferAllocator create(usize size) {
        FixedBufferAllocator fba = {};
        fba.capacity = size;
        fba.used = 0;
        fba.memory = (u8*)SDL_malloc(size);

        if (!fba.memory) {
            SDL_Log("Buy more RAM lol!");
            SDL_assert(false);
        }

        memset(fba.memory, 0, size);

        return fba;
    }

    void destroy() {
        capacity = 0;
        used = 0;
        SDL_free(memory);
    }

    template <typename T> T* alloc(usize count = 1) {
        usize size = sizeof(T) * count;

        usize alignment = alignof(T);
        usize aligned_used = (used + alignment - 1) & ~(alignment - 1);

        if (aligned_used + size > capacity) {
            SDL_Log("How could you fumble your memory this bad?");
            SDL_assert(false);
            return nullptr;
        }

        T* result = (T*)(memory + aligned_used);
        used = aligned_used + size;

        return result;
    }

    template <typename T, typename... Args>
    T* alloc_initialized(Args&&... args) {
        T* ptr = alloc<T>();

        if (ptr) {
            new (ptr) T(std::forward<Args>(args)...);
        }

        return ptr;
    }

    void* alloc_bytes(usize size, usize alignment = sizeof(void*)) {
        usize aligned_used = (used + alignment - 1) & ~(alignment - 1);

        if(aligned_used + size > capacity) {
            SDL_Log("How could you fumble your memory this bad?");
            SDL_assert(false);
            return nullptr;
        }

        void* result = memory + aligned_used;
        used = aligned_used + size;

        return result;
    }
};
