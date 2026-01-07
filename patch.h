#ifndef PATCH_H
#define PATCH_H

#include <windows.h>

// Declare the embedded gain variable (defined in patch.cpp)
extern float g_embedded_gain;

// Functions that will be injected into Discord
extern "C" void __cdecl hp_cutoff(
    const float* input, int cutoff_freq, float* output,
    int* state_mem, int sample_count, int channel_count,
    int sample_rate, int architecture);

extern "C" void __cdecl dc_reject(
    const float* src, float* dst, int* filter_state,
    int frame_size, int num_channels, int samplerate);

// Patching functions
bool ApplyAllPatches(float gain_multiplier);
bool UpdateGainValue(float new_gain);

// Store Discord process handle globally
extern HANDLE g_discord_process;
extern uintptr_t g_discord_base;
extern uintptr_t g_gain_address_hp;
extern uintptr_t g_gain_address_dc;

#endif