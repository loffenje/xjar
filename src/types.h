#pragma once

#include <stdint.h>
#include <stddef.h>

#define OpenGL 1
#define Vulkan 2

#define MAX_FRAMES_IN_FLIGHT 2

#define INVALID_TEXTURE 0xffffff

using i16 = int16_t;
using i32 = int32_t;
using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using f32 = float;
using f64 = double;
using b32 = int32_t;

struct gpuvec4 {
    f32 x, y, z, w;

    gpuvec4() = default;

    explicit gpuvec4(f32 v):
        x(v), y(v), z(v), w(v) {
    }

    gpuvec4(f32 a, f32 b, f32 c, f32 d):
        x(a), y(b), z(c), w(d) {
    }
};