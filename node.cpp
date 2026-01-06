#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <iostream>
#include <string>
#include <vector>

#pragma comment(lib, "psapi.lib")

// change this if you ever need the old version again
#define CURRENT_VERSION 9219

// set to 1 if you want to patch the .node file on disk instead of live process
#define PATCH_FILE 0

extern "C" void hp_cutoff(const float* in, int cutoff_Hz, float* out, int* hp_mem, int len, int channels, int Fs, int arch);
extern "C" void dc_reject(const float* in, float* out, int* hp_mem, int len, int channels, int Fs);

// offsets for 9186
namespace old {
    constexpr uintptr_t create_stereo = 0xAD794;
    constexpr uintptr_t set_channels = 0x302EA8;
    constexpr uintptr_t mono_downmix = 0x95B23;
    constexpr uintptr_t hp_process = 0x4A5022;
    constexpr uintptr_t stereo_success = 0x497504;
    constexpr uintptr_t high_bitrate = 0x497762;
    constexpr uintptr_t force_48khz = 0x49761B;
    constexpr uintptr_t inject_hp = 0x8B4370;
    constexpr uintptr_t inject_dc = 0x8B4550;
    constexpr uintptr_t kill_downmix = 0x8B0BB0;
    constexpr uintptr_t opus_ok = 0x30310C;
}

// offsets for current version 9219
namespace cur {
    constexpr uintptr_t create_stereo = 0x116C91;
    constexpr uintptr_t set_channels = 0x3A0B64;
    constexpr uintptr_t mono_downmix = 0xD6319;
    constexpr uintptr_t hp_process = 0x52CF70;
    constexpr uintptr_t stereo_flag1 = 0x520CFB;
    constexpr uintptr_t stereo_flag2 = 0x520D07;
    constexpr uintptr_t high_bitrate = 0x52115A;
    constexpr uintptr_t force_48khz = 0x520E63;
    constexpr uintptr_t inject_hp = 0x8D64B0;
    constexpr uintptr_t inject_dc = 0x8D6690;
    constexpr uintptr_t kill_downmix = 0x8D2820;
    constexpr uintptr_t opus_ok = 0x3A0E00;
    constexpr uintptr_t max_bitrate_val = 0x522F81;
    constexpr uintptr_t nop_bitrate_or = 0x522F89;
    constexpr uintptr_t kill_error_throw = 0x2B3340;

    // all the extra crap we just want to nop entirely
    const uintptr_t kill_list[] = {
        0x059619, 0x012568, 0x0593E1, 0x05886F,
        0x3D85E0, 0x3A31B0, 0x92DA50, 0x91DA20,
        0x584210, 0x7024E0, 0x5850A0, 0x584100,
        0x9205D0, 0x6B3DA0, 0x3A56A0, 0x52EC30,
        0x6DD580, 0x70EE40, 0x005510, 0x178490,
        0x3A2AE0, 0x920480, 0x118CB0, 0x0132D0,
        0x012EE0, 0x118C20, 0x52B3A0, 0x530680,
        0x5271A0, 0x6B43E0, 0x8EBD30, 0x52E0C0
    };
}

// simple wrapper
bool write_mem(HANDLE proc, void* addr, const void* data, size_t size)
{
    DWORD old_prot;
    if (!VirtualProtectEx(proc, addr, size, PAGE_EXECUTE_READWRITE, &old_prot))
        return false;

    bool ok = WriteProcessMemory(proc, addr, data, size, nullptr);

    DWORD dummy;
    VirtualProtectEx(proc, addr, size, old_prot, &dummy);
    return ok;
}

bool write_byte(HANDLE proc, void* addr, uint8_t byte)
{
    return write_mem(proc, addr, &byte, 1);
}

// find Discord.exe and the voice node module
bool find_process(HANDLE& process, uintptr_t& base)
{
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return false;

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(pe);

    while (Process32Next(snap, &pe))
    {
        if (_stricmp(pe.szExeFile, "Discord.exe") == 0)
        {
            process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe.th32ProcessID);
            if (!process) continue;

            HMODULE modules[1024];
            DWORD needed;

            if (EnumProcessModules(process, modules, sizeof(modules), &needed))
            {
                for (unsigned int i = 0; i < needed / sizeof(HMODULE); i++)
                {
                    char modname[MAX_PATH] = { 0 };
                    if (GetModuleBaseNameA(process, modules[i], modname, sizeof(modname)))
                    {
                        if (_stricmp(modname, "Discord_voice.node") == 0)
                        {
                            base = (uintptr_t)modules[i];
                            CloseHandle(snap);
                            return true;
                        }
                    }
                }
            }
            CloseHandle(process);
            process = nullptr;
        }
    }

    CloseHandle(snap);
    return false;
}

// main patching routine
void do_patches(HANDLE proc, uintptr_t base)
{
    auto addr = [&](uintptr_t off) { return (void*)(base + off); };

#if CURRENT_VERSION == 9186
    // old version patches
    uint8_t stereo_inst[] = { 0x4D, 0x89, 0xC5, 0x90 };
    write_mem(proc, addr(old::create_stereo), stereo_inst, sizeof(stereo_inst));

    write_byte(proc, addr(old::set_channels), 0x02);

#else
    // 9219 patches
    write_byte(proc, addr(cur::stereo_flag1), 0x02);
    write_byte(proc, addr(cur::stereo_flag2), 0xEB);

    uint8_t stereo_inst[] = { 0x49, 0x89, 0xC5, 0x90 };
    write_mem(proc, addr(cur::create_stereo), stereo_inst, 4);

    write_byte(proc, addr(cur::set_channels), 0x02);

    uint8_t nops[] = { 0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0xE9 };
    write_mem(proc, addr(cur::mono_downmix), nops, sizeof(nops));

    uint8_t high_br[] = { 0x00, 0xEE, 0x02 };
    write_mem(proc, addr(cur::high_bitrate), high_br, sizeof(high_br));

    uint8_t jump_to_shell[] = { 0x48,0xB8,0x10,0x9E,0xD8,0xCF,0x08,0x02,0x00,0x00,0xC3 };
    write_mem(proc, addr(cur::hp_process), jump_to_shell, sizeof(jump_to_shell));

    write_mem(proc, addr(cur::inject_hp), hp_cutoff, 0x100);
    write_mem(proc, addr(cur::inject_dc), dc_reject, 0x1B6);

    write_byte(proc, addr(cur::kill_downmix), 0xC3);

    write_mem(proc, addr(cur::force_48khz), "\x90\x90\x90", 3);

    uint8_t always_ok[] = { 0x48,0xC7,0xC0,0x01,0x00,0x00,0x00,0xC3 };
    write_mem(proc, addr(cur::opus_ok), always_ok, sizeof(always_ok));

    uint8_t max_br[] = { 0x30, 0xC8, 0x07, 0x00, 0x00 };
    write_mem(proc, addr(cur::max_bitrate_val), max_br, sizeof(max_br));
    write_mem(proc, addr(cur::nop_bitrate_or), "\x90\x90\x90", 3);

    write_byte(proc, addr(cur::kill_error_throw), 0xC3);

    // kill all the extra processing functions
    for (size_t i = 0; i < sizeof(cur::kill_list) / sizeof(cur::kill_list[0]); i++)
    {
        write_byte(proc, addr(cur::kill_list[i]), 0xC3);
    }
#endif

    std::cout << "All patches applied for version " << CURRENT_VERSION << "\n";
}

int main()
{
    HANDLE process = nullptr;
    uintptr_t module_base = 0;

    if (!find_process(process, module_base))
    {
        std::cout << "Make sure it's running and try as admin\n";
        system("pause");
        return 1;
    }

#if PATCH_FILE
    std::cout << "File patching is disabled in this build\n";
#else
    do_patches(process, module_base);
    CloseHandle(process);
#endif

    std::cout << "Successfully applied\n";
    system("pause");
    return 0;
}