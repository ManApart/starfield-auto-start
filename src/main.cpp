#include <SFSE/SFSE.h>
#include <RE/Starfield.h>
#include <windows.h>

#include <atomic>
#include <memory>

static constexpr DWORD kForegroundPollMs = 250;
static constexpr DWORD kWaitForMenuTimeoutMs = 180000;
static constexpr DWORD kMainMenuPollMs = 250;
static constexpr DWORD kMainMenuSettleDelayMs = 200;
static constexpr DWORD kTaskPollMs = 10;
static constexpr DWORD kTaskTimeoutMs = 5000;

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

static bool load_most_recent_save_game_thread()
{
    auto* saveLoad = RE::BGSSaveLoadManager::GetSingleton();
    if (saveLoad == nullptr) {
        write_log_line("AutoStartSFSE: BGSSaveLoadManager unavailable");
        return false;
    }

    saveLoad->QueueLoadMostRecentSaveGame();

    return true;
}

static bool load_most_recent_save()
{
    const auto* taskInterface = SFSE::GetTaskInterface();
    if (taskInterface == nullptr) {
        write_log_line("AutoStartSFSE: TaskInterface unavailable");
        return false;
    }

    struct TaskState
    {
        std::atomic<bool> done{ false };
        bool              success{ false };
    };

    auto state = std::make_shared<TaskState>();

    taskInterface->AddTask([state]() {
        state->success = load_most_recent_save_game_thread();
        state->done.store(true, std::memory_order_release);
    });

    const DWORD start = GetTickCount();
    while (!state->done.load(std::memory_order_acquire) && (GetTickCount() - start) < kTaskTimeoutMs) {
        Sleep(kTaskPollMs);
    }

    if (!state->done.load(std::memory_order_acquire)) {
        write_log_line("AutoStartSFSE: timed out waiting for game-thread load call");
        return false;
    }

    return state->success;
}

static bool starfield_window_is_foreground()
{
    const HWND foreground = GetForegroundWindow();
    if (foreground == nullptr || !IsWindowVisible(foreground)) {
        return false;
    }

    DWORD foregroundPid = 0;
    GetWindowThreadProcessId(foreground, &foregroundPid);
    return foregroundPid == GetCurrentProcessId();
}

static bool wait_for_starfield_foreground_window(DWORD timeoutMs)
{
    const DWORD start = GetTickCount();
    while ((GetTickCount() - start) < timeoutMs) {
        if (starfield_window_is_foreground()) {
            return true;
        }
        Sleep(kForegroundPollMs);
    }
    return false;
}

static bool wait_for_main_menu_open(DWORD timeoutMs)
{
    const DWORD start = GetTickCount();
    const RE::BSFixedString mainMenuName{ "MainMenu" };
    while ((GetTickCount() - start) < timeoutMs) {
        auto* ui = RE::UI::GetSingleton();
        if (ui != nullptr && ui->IsMenuOpen(mainMenuName)) {
            return true;
        }
        Sleep(kMainMenuPollMs);
    }
    return false;
}

static DWORD WINAPI autoload_worker_thread(LPVOID /*unused*/)
{
    if (!wait_for_starfield_foreground_window(kWaitForMenuTimeoutMs)) {
        write_log_line("AutoStartSFSE: timed out waiting for Starfield foreground window");
        return 0;
    }

    if (!wait_for_main_menu_open(kWaitForMenuTimeoutMs)) {
        write_log_line("AutoStartSFSE: timed out waiting for MainMenu open");
        return 0;
    }

    Sleep(kMainMenuSettleDelayMs);
    if (!load_most_recent_save()) {
        write_log_line("AutoStartSFSE: failed to queue most recent save load");
        return 0;
    }

    return 0;
}

static volatile LONG g_workerStarted = 0;

static void start_autoload_worker_once()
{
    if (InterlockedCompareExchange(&g_workerStarted, 1, 0) != 0) {
        return;
    }

    HANDLE worker = CreateThread(nullptr, 0, autoload_worker_thread, nullptr, 0, nullptr);
    if (worker == nullptr) {
        write_log_line("AutoStartSFSE: failed to create worker thread");
        InterlockedExchange(&g_workerStarted, 0);
        return;
    }

    CloseHandle(worker);
}

SFSE_PLUGIN_VERSION = []() noexcept {
    SFSE::PluginVersionData v{};
    v.PluginVersion({ 1, 0, 0, 0 });
    v.PluginName("AutoStartSFSE");
    v.AuthorName("you");
    v.UsesAddressLibrary(true);
    v.HasNoStructUse(false);
    v.IsLayoutDependent(true);
    return v;
}();

SFSE_PLUGIN_LOAD(const SFSE::LoadInterface* sfse)
{
    SFSE::Init(sfse);
    start_autoload_worker_once();
    return true;
}

SFSE_PLUGIN_PRELOAD(const SFSE::PreLoadInterface* sfse)
{
    SFSE::Init(sfse);
    start_autoload_worker_once();
    return true;
}
