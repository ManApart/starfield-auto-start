#include <windows.h>
#include <cstdint>

// Minimal SFSE plugin ABI (enough to be loadable without SFSE headers)
using PluginHandle = std::uint32_t;

static constexpr std::uint32_t MAKE_VERSION(std::uint32_t major, std::uint32_t minor, std::uint32_t build)
{
    return (((major & 0xFFu) << 24) | ((minor & 0xFFu) << 16) | ((build & 0xFFFu) << 4));
}

struct SFSEPluginVersionData
{
    std::uint32_t dataVersion;
    std::uint32_t pluginVersion;
    char name[256];
    char author[256];
    std::uint32_t addressIndependence;
    std::uint32_t structureIndependence;
    std::uint32_t compatibleVersions[16];
    std::uint32_t seVersionRequired;
    std::uint32_t reservedNonBreaking;
    std::uint32_t reservedBreaking;
};

struct SFSEInterface
{
    std::uint32_t sfseVersion;
    std::uint32_t runtimeVersion;
    std::uint32_t interfaceVersion;
    void* (*QueryInterface)(std::uint32_t id);
    PluginHandle (*GetPluginHandle)();
    void* (*GetPluginInfo)(const char* name);
};

static void write_log_line(const char* line)
{
    // Compute "<this dll folder>\\AutoStartSFSE.log"
    HMODULE self = nullptr;
    if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            reinterpret_cast<LPCSTR>(&write_log_line),
                            &self)) {
        return;
    }

    char dllPath[MAX_PATH]{};
    const DWORD n = GetModuleFileNameA(self, dllPath, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) {
        return;
    }

    // Strip filename
    for (int i = static_cast<int>(n) - 1; i >= 0; --i) {
        if (dllPath[i] == '\\' || dllPath[i] == '/') {
            dllPath[i + 1] = '\0';
            break;
        }
    }

    char logPath[MAX_PATH]{};
    lstrcpynA(logPath, dllPath, MAX_PATH);
    lstrcatA(logPath, "AutoStartSFSE.log");

    HANDLE h = CreateFileA(
        logPath,
        FILE_APPEND_DATA,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);

    if (h == INVALID_HANDLE_VALUE) {
        return;
    }

    DWORD written = 0;
    WriteFile(h, line, static_cast<DWORD>(lstrlenA(line)), &written, nullptr);
    WriteFile(h, "\r\n", 2, &written, nullptr);
    CloseHandle(h);
}

extern "C" __declspec(dllexport) SFSEPluginVersionData SFSEPlugin_Version = {
    1,  // SFSE version data format
    1,  // plugin version
    "AutoStartSFSE",
    "you",
    1,  // AddressIndependence::Signatures
    1,  // StructureIndependence::NoStructs
    {0},  // compatibleVersions (unused for version-independent plugins)
    0,  // seVersionRequired
    0,  // reservedNonBreaking
    0   // reservedBreaking
};

// Called by SFSE during the normal load phase (after preload, if present).
extern "C" __declspec(dllexport) bool SFSEPlugin_Load(const SFSEInterface* /*sfse*/)
{
    write_log_line("AutoStartSFSE: hello world (SFSEPlugin_Load)");
    return true;
}

// Optional: also support preload phase; SFSE will call this if present.
extern "C" __declspec(dllexport) bool SFSEPlugin_Preload(const SFSEInterface* /*sfse*/)
{
    write_log_line("AutoStartSFSE: hello world (SFSEPlugin_Preload)");
    return true;
}

