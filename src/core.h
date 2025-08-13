#pragma once

#include "SDL3/SDL_assert.h"
#include "SDL3/SDL_error.h"
#include "SDL3/SDL_iostream.h"
#include "SDL3/SDL_log.h"
#include <cassert>
#include <chrono>
#include <expected>
#include <filesystem>
#include <system_error>
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

using std::error_code;
using std::expected;
using std::unexpected;
using std::chrono::duration_cast;
using std::chrono::seconds;
using std::filesystem::file_time_type;
using std::filesystem::last_write_time;

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

        if (aligned_used + size > capacity) {
            SDL_Log("How could you fumble your memory this bad?");
            SDL_assert(false);
            return nullptr;
        }

        void* result = memory + aligned_used;
        used = aligned_used + size;

        return result;
    }
};

struct FileData {
    u8* data;
    usize size;
    i64 last_modified;
};

enum FileDataError {
    InvalidFile,
    StatReadFailed,
    SizeReadFailed,
    AllocationFailed,
    ReadFailed
};

static expected<FileData, FileDataError>
read_entire_file(const char* filename, FixedBufferAllocator& allocator) {
    SDL_IOStream* file = SDL_IOFromFile(filename, "rb");
    if (!file) {
        SDL_Log("Failed to open file %s: %s", filename, SDL_GetError());
        return unexpected(InvalidFile);
    }
    defer { SDL_CloseIO(file); };

    error_code ec;
    file_time_type mod_time = last_write_time(filename, ec);

    if (ec) {
        SDL_Log(
            "Failed to get the file modification time: %s",
            ec.message().c_str()
        );
        return unexpected(StatReadFailed);
    }

    auto unix_time =
        duration_cast<seconds>(mod_time.time_since_epoch()).count();

    i64 file_size = SDL_GetIOSize(file);
    if (file_size < 0) {
        SDL_Log("Failed to get file size: %s", SDL_GetError());
        return unexpected(SizeReadFailed);
    }

    auto data = (u8*)allocator.alloc_bytes(file_size);
    if (!data) {
        SDL_Log("Failed to allocate %lld bytes for file", file_size);
        return unexpected(AllocationFailed);
    }

    usize bytes_read = SDL_ReadIO(file, data, file_size);
    if (bytes_read != (usize)file_size) {
        SDL_Log("Failed to read entire file");
        return unexpected(ReadFailed);
    }

    return FileData{
        .data = data,
        .size = (usize)file_size,
        .last_modified = unix_time,
    };
}

static bool write_file(const char* filename, const void* data, size_t size) {
    SDL_IOStream* file = SDL_IOFromFile(filename, "wb");
    if (!file) {
        SDL_Log("Failed to create file %s: %s", filename, SDL_GetError());
        return false;
    }
    defer { SDL_CloseIO(file); };

    size_t written = SDL_WriteIO(file, data, size);
    if (written != size) {
        SDL_Log("Failed to write complete data to file");
        return false;
    }

    return true;
}
