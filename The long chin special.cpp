#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <iostream>
#include "patch.h"

#pragma comment(lib, "psapi.lib")

HANDLE g_discord_process = NULL;
uintptr_t g_discord_base = 0;
uintptr_t g_gain_address_hp = 0;
uintptr_t g_gain_address_dc = 0;

namespace Offsets9219 {
    constexpr uintptr_t StereoSuccess1 = 0x520CFB;
    constexpr uintptr_t StereoSuccess2 = 0x520D07;
    constexpr uintptr_t CreateStereoFrame = 0x116C91;
    constexpr uintptr_t SetOpusChannels = 0x3A0B64;
    constexpr uintptr_t DisableMonoDownmix = 0xD6319;
    constexpr uintptr_t BypassHighPass = 0x52CF70;
    constexpr uintptr_t InjectHighpass = 0x8D64B0;
    constexpr uintptr_t InjectDcReject = 0x8D6690;
    constexpr uintptr_t DisableDownmixRoutine = 0x8D2820;
    constexpr uintptr_t Force48kHz = 0x520E63;
    constexpr uintptr_t BypassOpusConfig = 0x3A0E00;
    constexpr uintptr_t MaxBitrateValue = 0x522F81;
    constexpr uintptr_t NopBitrateOr = 0x522F89;
    constexpr uintptr_t DisableErrorThrow = 0x2B3340;
    constexpr uintptr_t ForceHighBitrate = 0x52115A;
    constexpr uintptr_t headroom1 = 0x058DE0;
    constexpr uintptr_t headroom2 = 0x6B3DA0;
    constexpr uintptr_t headroom3 = 0x52B3A0;
    constexpr uintptr_t sidechain1 = 0x012EE0;
    constexpr uintptr_t sidechain2 = 0x0132D0;
    constexpr uintptr_t sidechain3 = 0x118CB0;
    constexpr uintptr_t sidechain4 = 0x118C20;
    constexpr uintptr_t splitting_filter = 0x3A2AE0;
    constexpr uintptr_t matched_filter = 0x920480;
    constexpr uintptr_t voicefilters = 0x005510;
    constexpr uintptr_t clipping1 = 0x52EC30;
    constexpr uintptr_t srytjsrt1 = 0x3D85E0;
    constexpr uintptr_t srytjsrt2 = 0x52B3A0;
    constexpr uintptr_t processing1 = 0x6FE140;
    constexpr uintptr_t processing2 = 0x92DA50;
    constexpr uintptr_t processing3 = 0x91DA20;
    constexpr uintptr_t processing4 = 0x92FA00;
    constexpr uintptr_t processing5 = 0x404750;
    constexpr uintptr_t processing6 = 0x3BD7A0;
    constexpr uintptr_t processing7 = 0x404750;
    constexpr uintptr_t processing8 = 0x930340;
    constexpr uintptr_t processing9 = 0x6D8FC0;
    constexpr uintptr_t processing10 = 0xB26010;
    constexpr uintptr_t processing11 = 0x92FA00;
    constexpr uintptr_t processing12 = 0xB26300;
    constexpr uintptr_t agc1 = 0x52E300;
    constexpr uintptr_t agc2 = 0x52D390;
    constexpr uintptr_t agc3 = 0x52E0C0;
    constexpr uintptr_t agc4 = 0x8EBD30;
    constexpr uintptr_t vad1 = 0x8EB900;
    constexpr uintptr_t vad2 = 0x6DD580;
}

class MemWriter {
    HANDLE proc;
public:
    explicit MemWriter(HANDLE h) : proc(h) {}
    bool Write(void* addr, const void* data, size_t sz) {
        DWORD old;
        if (!VirtualProtectEx(proc, addr, sz, PAGE_EXECUTE_READWRITE, &old)) return false;
        bool ok = WriteProcessMemory(proc, addr, data, sz, nullptr);
        DWORD junk;
        VirtualProtectEx(proc, addr, sz, old, &junk);
        return ok;
    }
    bool WriteByte(void* addr, uint8_t b) { return Write(addr, &b, 1); }
};

std::pair<HANDLE, uintptr_t> FindTarget() {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return { nullptr, 0 };

    PROCESSENTRY32 pe{ sizeof(pe) };
    while (Process32Next(snap, &pe)) {
        if (_stricmp(pe.szExeFile, "Discord.exe") == 0) {
            HANDLE p = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe.th32ProcessID);
            if (!p) continue;

            HMODULE mods[1024];
            DWORD needed;
            if (EnumProcessModules(p, mods, sizeof(mods), &needed)) {
                for (unsigned i = 0; i < needed / sizeof(HMODULE); ++i) {
                    char name[MAX_PATH]{};
                    if (GetModuleBaseNameA(p, mods[i], name, MAX_PATH) &&
                        _stricmp(name, "Discord_voice.node") == 0) {
                        CloseHandle(snap);
                        return { p, reinterpret_cast<uintptr_t>(mods[i]) };
                    }
                }
            }
            CloseHandle(p);
        }
    }
    CloseHandle(snap);
    return { nullptr, 0 };
}

void ApplyRuntimePatches(HANDLE proc, uintptr_t base) {
    MemWriter writer(proc);
    auto Addr = [base](uintptr_t off) { return reinterpret_cast<void*>(base + off); };

    writer.WriteByte(Addr(Offsets9219::StereoSuccess1), 0x02);
    writer.WriteByte(Addr(Offsets9219::StereoSuccess2), 0xEB);

    const uint8_t stereo[] = { 0x49, 0x89, 0xC5, 0x90 };
    writer.Write(Addr(Offsets9219::CreateStereoFrame), stereo, sizeof(stereo));

    writer.WriteByte(Addr(Offsets9219::SetOpusChannels), 0x02);

    const uint8_t long_nop[] = { 0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0xE9 };
    writer.Write(Addr(Offsets9219::DisableMonoDownmix), long_nop, sizeof(long_nop));

    const uint8_t high_br[] = { 0x00, 0xEE, 0x02 };
    writer.Write(Addr(Offsets9219::ForceHighBitrate), high_br, sizeof(high_br));

    const uint8_t jmp_shell[] = { 0x48,0xB8,0x10,0x9E,0xD8,0xCF,0x08,0x02,0x00,0x00,0xC3 };
    writer.Write(Addr(Offsets9219::BypassHighPass), jmp_shell, sizeof(jmp_shell));

    writer.Write(Addr(Offsets9219::InjectHighpass), hp_cutoff, 0x100);
    writer.Write(Addr(Offsets9219::InjectDcReject), dc_reject, 0x1B6);

    g_gain_address_hp = base + Offsets9219::InjectHighpass +
        reinterpret_cast<uintptr_t>(&g_embedded_gain) -
        reinterpret_cast<uintptr_t>(hp_cutoff);

    g_gain_address_dc = base + Offsets9219::InjectDcReject +
        reinterpret_cast<uintptr_t>(&g_embedded_gain) -
        reinterpret_cast<uintptr_t>(dc_reject);

    writer.WriteByte(Addr(Offsets9219::DisableDownmixRoutine), 0xC3);
    writer.Write(Addr(Offsets9219::Force48kHz), "\x90\x90\x90", 3);

    const uint8_t always_ok[] = { 0x48,0xC7,0xC0,0x01,0x00,0x00,0x00,0xC3 };
    writer.Write(Addr(Offsets9219::BypassOpusConfig), always_ok, sizeof(always_ok));

    const uint8_t max_br[] = { 0x30,0xC8,0x07,0x00,0x00 };
    writer.Write(Addr(Offsets9219::MaxBitrateValue), max_br, sizeof(max_br));
    writer.Write(Addr(Offsets9219::NopBitrateOr), "\x90\x90\x90", 3);

    writer.WriteByte(Addr(Offsets9219::DisableErrorThrow), 0xC3);

    writer.WriteByte(Addr(Offsets9219::headroom1), 0xC3);
    writer.WriteByte(Addr(Offsets9219::headroom2), 0xC3);
    writer.WriteByte(Addr(Offsets9219::headroom3), 0xC3);

    writer.WriteByte(Addr(Offsets9219::sidechain1), 0xC3);
    writer.WriteByte(Addr(Offsets9219::sidechain2), 0xC3);
    writer.WriteByte(Addr(Offsets9219::sidechain3), 0xC3);
    writer.WriteByte(Addr(Offsets9219::sidechain4), 0xC3);

    writer.WriteByte(Addr(Offsets9219::splitting_filter), 0xC3);
    writer.WriteByte(Addr(Offsets9219::matched_filter), 0xC3);
    writer.WriteByte(Addr(Offsets9219::voicefilters), 0xC3);

    writer.WriteByte(Addr(Offsets9219::clipping1), 0xC3);

    writer.WriteByte(Addr(Offsets9219::srytjsrt1), 0xC3);
    writer.WriteByte(Addr(Offsets9219::srytjsrt2), 0xC3);

    writer.WriteByte(Addr(Offsets9219::processing1), 0xC3);


    //          semi unstable (still decent)
    writer.WriteByte(Addr(Offsets9219::processing7), 0xC3);
    writer.WriteByte(Addr(Offsets9219::processing8), 0xC3);
    writer.WriteByte(Addr(Offsets9219::processing9), 0xC3);

    //         !!!!!!!!!!!!UNSTABLE!!!!!!!!!!!!
    //writer.WriteByte(Addr(Offsets9219::processing2), 0xC3);
    //writer.WriteByte(Addr(Offsets9219::processing3), 0xC3);
    //writer.WriteByte(Addr(Offsets9219::processing4), 0xC3);
    //writer.WriteByte(Addr(Offsets9219::processing5), 0xC3);
    //writer.WriteByte(Addr(Offsets9219::processing6), 0xC3);
    //writer.WriteByte(Addr(Offsets9219::processing10), 0xC3);
    //writer.WriteByte(Addr(Offsets9219::processing11), 0xC3);
    //writer.WriteByte(Addr(Offsets9219::processing12), 0xC3);

    writer.WriteByte(Addr(Offsets9219::agc1), 0xC3);
    writer.WriteByte(Addr(Offsets9219::agc2), 0xC3);
    writer.WriteByte(Addr(Offsets9219::agc3), 0xC3);
    writer.WriteByte(Addr(Offsets9219::agc4), 0xC3);

    writer.WriteByte(Addr(Offsets9219::vad1), 0xC3);
    writer.WriteByte(Addr(Offsets9219::vad2), 0xC3);
}

bool ApplyAllPatches(float gain_multiplier) {
    std::cout << "[Patcher] Applying patches with gain " << gain_multiplier << "x...\n";

    auto [process, base] = FindTarget();
    if (!process || !base) {
        std::cout << "Discord.exe not running or discord_voice.node not found.\n";
        return false;
    }

    g_discord_process = process;
    g_discord_base = base;

    ApplyRuntimePatches(process, base);

    UpdateGainValue(gain_multiplier);

    std::cout << "Successfully applied all patches!\n";
    return true;
}

bool UpdateGainValue(float new_gain) {
    if (!g_discord_process) {
        std::cerr << "No Discord process handle - apply patches first!\n";
        return false;
    }

    MemWriter writer(g_discord_process);

    bool ok1 = writer.Write((void*)g_gain_address_hp, &new_gain, sizeof(float));
    bool ok2 = writer.Write((void*)g_gain_address_dc, &new_gain, sizeof(float));

    if (ok1 && ok2) {
        std::cout << "Updated gain to " << new_gain << "\n";
        return true;
    }

    std::cerr << "Failed to update gain value!\n";
    return false;
}