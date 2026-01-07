#include "patch.h"
#include <iostream>

using i8 = signed char;
using i16 = short;
using i32 = int;
using i64 = long long;
using u8 = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;
using u64 = unsigned long long;

float g_embedded_gain = 1.0f;

template<typename T>
struct PackedResult {
    u64 storage = 0;
    constexpr T extract(u8 index) const {
        constexpr u8 elem_bits = sizeof(T) * 8;
        return static_cast<T>(storage >> (index * elem_bits));
    }
    constexpr void pack(u32 a) { storage = a; }
    constexpr void pack(u32 a, u32 b) {
        storage = static_cast<u64>(a) | (static_cast<u64>(b) << 32);
    }
};

template<typename T, u64 packed>
constexpr T unpack_value(u8 idx) {
    constexpr u8 shift_amount = sizeof(T) * 8;
    return static_cast<T>(packed >> (idx * shift_amount));
}

constexpr PackedResult<u32> find_best_fraction() {
    constexpr float target = 10.0f;
    constexpr float max_error = 0.1f;
    for (u32 den = 1; den <= 10; ++den) {
        for (u32 num = 1; num <= 10; ++num) {
            float approx = static_cast<float>(num) / den;
            float error = (approx > target) ? (approx - target) : (target - approx);
            if (error <= max_error) {
                PackedResult<u32> result;
                result.pack(num, den);
                return result;
            }
        }
    }
    PackedResult<u32> fallback;
    fallback.pack(2, 1);
    return fallback;
}

constexpr auto best_fraction = find_best_fraction();
constexpr i32 num_offset = unpack_value<u32, best_fraction.storage>(0) - 2;
constexpr i32 den_offset = unpack_value<u32, best_fraction.storage>(1) - 2;

extern "C" void __cdecl hp_cutoff(
    const float* input,
    int /*cutoff_freq*/,
    float* output,
    int* state_mem,
    int sample_count,
    int channel_count,
    int /*sample_rate*/,
    int /*architecture*/)
{
    char* base = reinterpret_cast<char*>(state_mem - 3553);
    reinterpret_cast<int*>(base + 3557 * 4)[0] = 1002;
    reinterpret_cast<int*>(base + 160)[0] = -1;
    reinterpret_cast<int*>(base + 164)[0] = -1;
    reinterpret_cast<int*>(base + 184)[0] = 0;

    const int total_samples = sample_count * channel_count;
    const float scale = static_cast<float>(channel_count) + g_embedded_gain;

    for (int i = 0; i < total_samples; ++i) {
        output[i] = input[i] * scale;
    }
}

extern "C" void __cdecl dc_reject(
    const float* src,
    float* dst,
    int* filter_state,
    int frame_size,
    int num_channels,
    int /*samplerate*/)
{
    char* base = reinterpret_cast<char*>(filter_state - 3553);
    reinterpret_cast<int*>(base + 3557 * 4)[0] = 1002;
    reinterpret_cast<int*>(base + 160)[0] = -1;
    reinterpret_cast<int*>(base + 164)[0] = -1;
    reinterpret_cast<int*>(base + 184)[0] = 0;

    const int total_samples = frame_size * num_channels;
    const float scale = static_cast<float>(num_channels) + g_embedded_gain;

    for (int i = 0; i < total_samples; ++i) {
        dst[i] = src[i] * scale;
    }
}

uintptr_t FindGainOffset(void* func_start, size_t func_size) {
    uint8_t* bytes = (uint8_t*)func_start;
    float search_val = 30.0f;
    uint32_t search_pattern = *(uint32_t*)&search_val;

    for (size_t i = 0; i < func_size - 4; i++) {
        if (*(uint32_t*)(bytes + i) == search_pattern) {
            return i;
        }
    }
    return 0;
}