#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <iostream>
#include <vector>
#include <string>

#pragma comment(lib, "psapi.lib")

extern "C" void hp_cutoff(const float* in, int cutoff_Hz, float* out, int* hp_mem, int len, int channels, int Fs, int arch);
extern "C" void dc_reject(const float* in, float* out, int* hp_mem, int len, int channels, int Fs);

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

    writer.WriteByte(Addr(Offsets9219::DisableDownmixRoutine), 0xC3);

    writer.Write(Addr(Offsets9219::Force48kHz), "\x90\x90\x90", 3);

    const uint8_t always_ok[] = { 0x48,0xC7,0xC0,0x01,0x00,0x00,0x00,0xC3 };
    writer.Write(Addr(Offsets9219::BypassOpusConfig), always_ok, sizeof(always_ok));

    const uint8_t max_br[] = { 0x30,0xC8,0x07,0x00,0x00 };
    writer.Write(Addr(Offsets9219::MaxBitrateValue), max_br, sizeof(max_br));
    writer.Write(Addr(Offsets9219::NopBitrateOr), "\x90\x90\x90", 3);

    writer.WriteByte(Addr(Offsets9219::DisableErrorThrow), 0xC3);
}

int main() {
    auto [process, base] = FindTarget();
    if (!process || !base) {
        std::cout << "Discord.exe not running.\n";
        system("pause");
        return 1;
    }

    ApplyRuntimePatches(process, base);
    CloseHandle(process);

    std::cout << "Successfully applied\n";
    system("pause");
    return 0;
}
