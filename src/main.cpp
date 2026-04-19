#include <SFSE/SFSE.h>
#include <RE/Starfield.h>
#include <windows.h>

#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <memory>

static constexpr DWORD kWaitForMenuTimeoutMs = 1800000;
static constexpr DWORD kPostMainMenuTimeoutMs = 30000;
static constexpr DWORD kMainMenuPollMs = 250;
static constexpr DWORD kPostMainMenuRetryMs = 500;
static constexpr DWORD kTaskPollMs = 50;
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

static void write_log_linef(const char* fmt, ...)
{
    char buffer[512]{};

    va_list args;
    va_start(args, fmt);
    const int n = std::vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    if (n <= 0) {
        return;
    }

    buffer[sizeof(buffer) - 1] = '\0';
    write_log_line(buffer);
}

static DWORD remaining_timeout_since(DWORD startTick, DWORD totalTimeoutMs)
{
    const DWORD elapsed = GetTickCount() - startTick;
    return elapsed >= totalTimeoutMs ? 0 : (totalTimeoutMs - elapsed);
}

static bool load_most_recent_save_game_thread()
{
    write_log_line("AutoStartSFSE: game-thread load callback started");

    auto* saveLoad = RE::BGSSaveLoadManager::GetSingleton();
    if (saveLoad == nullptr) {
        write_log_line("AutoStartSFSE: BGSSaveLoadManager unavailable");
        return false;
    }

    write_log_linef("AutoStartSFSE: save list state built=%s count=%u arraySize=%u",
                    saveLoad->saveGameListBuilt ? "true" : "false",
                    saveLoad->saveGameCount,
                    saveLoad->saveGameList.size());

    if (!saveLoad->saveGameListBuilt) {
        write_log_line("AutoStartSFSE: save list not built yet");
        return false;
    }

    if (saveLoad->saveGameCount == 0 || saveLoad->saveGameList.empty()) {
        write_log_line("AutoStartSFSE: no save entries available yet");
        return false;
    }

    auto* entry = saveLoad->saveGameList.back();
    if (entry == nullptr) {
        write_log_line("AutoStartSFSE: most recent save entry is null");
        return false;
    }

    saveLoad->QueueLoadGame(entry);
    write_log_linef("AutoStartSFSE: queued QueueLoadGame for '%s'",
                    entry->GetFileName() ? entry->GetFileName() : "<null>");

    return true;
}

static bool load_most_recent_save(DWORD timeoutMs)
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

    write_log_linef("AutoStartSFSE: scheduling load task on game thread (timeout=%lu ms)", timeoutMs);

    taskInterface->AddTask([state]() {
        state->success = load_most_recent_save_game_thread();
        state->done.store(true, std::memory_order_release);
    });

    const DWORD start = GetTickCount();
    while (!state->done.load(std::memory_order_acquire) && (GetTickCount() - start) < timeoutMs) {
        Sleep(kTaskPollMs);
    }

    if (!state->done.load(std::memory_order_acquire)) {
        write_log_line("AutoStartSFSE: timed out waiting for game-thread load call");
        return false;
    }

    write_log_linef("AutoStartSFSE: game-thread load task completed in %lu ms", GetTickCount() - start);

    return state->success;
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
    const DWORD workerStartedAt = GetTickCount();
    write_log_line("AutoStartSFSE: worker thread started");

    const DWORD mainMenuWaitStartedAt = GetTickCount();
    write_log_linef("AutoStartSFSE: waiting for MainMenu open (timeout=%lu ms)", kWaitForMenuTimeoutMs);

    if (!wait_for_main_menu_open(kWaitForMenuTimeoutMs)) {
        write_log_line("AutoStartSFSE: timed out waiting for MainMenu open");
        return 0;
    }

    const DWORD mainMenuDetectedAt = GetTickCount();
    DWORD       attempt = 0;
    write_log_linef("AutoStartSFSE: MainMenu detected after %lu ms", mainMenuDetectedAt - mainMenuWaitStartedAt);
    write_log_linef("AutoStartSFSE: attempting most recent save load for up to %lu ms after MainMenu", kPostMainMenuTimeoutMs);

    while (true) {
        const DWORD remainingMs = remaining_timeout_since(mainMenuDetectedAt, kPostMainMenuTimeoutMs);
        if (remainingMs == 0) {
            write_log_line("AutoStartSFSE: timed out requesting most recent save load after MainMenu");
            return 0;
        }

        ++attempt;
        write_log_linef("AutoStartSFSE: load attempt %lu (remaining=%lu ms after MainMenu)", attempt, remainingMs);

        const DWORD taskTimeoutMs = remainingMs < kTaskTimeoutMs ? remainingMs : kTaskTimeoutMs;
        if (load_most_recent_save(taskTimeoutMs)) {
            write_log_linef("AutoStartSFSE: autoload request completed after %lu ms since MainMenu and %lu ms since worker start",
                            GetTickCount() - mainMenuDetectedAt,
                            GetTickCount() - workerStartedAt);
            return 0;
        }

        const DWORD retryDelayMs = remainingMs < kPostMainMenuRetryMs ? remainingMs : kPostMainMenuRetryMs;
        write_log_linef("AutoStartSFSE: load attempt %lu failed, retrying in %lu ms", attempt, retryDelayMs);
        Sleep(retryDelayMs);
    }
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
    v.PluginVersion({ 1, 0, 2, 0 });
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
    write_log_line("AutoStartSFSE: SFSE_PLUGIN_LOAD");
    start_autoload_worker_once();
    return true;
}

SFSE_PLUGIN_PRELOAD(const SFSE::PreLoadInterface* sfse)
{
    SFSE::Init(sfse);
    write_log_line("AutoStartSFSE: SFSE_PLUGIN_PRELOAD");
    return true;
}
