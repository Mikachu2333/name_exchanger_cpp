#include "src/app.hpp"
#include "src/utils.hpp"

#include <shellapi.h>
#include <windows.h>

#include <cwchar>

namespace {
class LocalArgv final {
  public:
    LocalArgv() : data_(CommandLineToArgvW(GetCommandLineW(), &count_)) {}
    ~LocalArgv() {
        if (data_) LocalFree(data_);
    }
    LocalArgv(const LocalArgv&) = delete;
    LocalArgv& operator=(const LocalArgv&) = delete;

    [[nodiscard]] bool valid() const noexcept { return data_ != nullptr; }
    [[nodiscard]] int count() const noexcept { return count_; }
    [[nodiscard]] wchar_t** data() const noexcept { return data_; }

  private:
    int count_ = 0;
    wchar_t** data_ = nullptr;
};

class UniqueHandle final {
  public:
    explicit UniqueHandle(HANDLE value = nullptr) noexcept : value_(value) {}
    ~UniqueHandle() {
        if (value_) {
            if (ownsMutex_) ReleaseMutex(value_);
            CloseHandle(value_);
        }
    }
    UniqueHandle(const UniqueHandle&) = delete;
    UniqueHandle& operator=(const UniqueHandle&) = delete;
    [[nodiscard]] HANDLE get() const noexcept { return value_; }
    void setOwnsMutex(bool value) noexcept { ownsMutex_ = value; }

  private:
    HANDLE value_;
    bool ownsMutex_ = false;
};
}

int APIENTRY WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int) {
    LocalArgv arguments;
    if (!arguments.valid()) return 255;

    const bool guiRelaunch = arguments.count() == 2 && arguments.data()[1] &&
                             std::wcscmp(arguments.data()[1], L"--gui-relaunch") == 0;
    // CLI invocations must not be blocked by an already running GUI instance or inherit its
    // remembered elevation preference.
    if (arguments.count() > 1 && !guiRelaunch) return App::RunCommandLine(arguments.count(), arguments.data());

    // Extension case is the persistent mode flag. Enforce it before acquiring the single-instance
    // mutex so the replacement process does not have to wait for a half-initialized instance.
    if (!guiRelaunch) {
        const bool preferredAdminMode = IsAdminModePreferred();
        if (preferredAdminMode != IsRunAsAdmin()) return RunAsAdmin(preferredAdminMode) ? 0 : 1;
    }

    UniqueHandle instanceMutex(CreateMutexW(nullptr, TRUE, PROCESS_MUTEX_GUID));
    if (!instanceMutex.get()) return 1;
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        if (guiRelaunch && WaitForSingleObject(instanceMutex.get(), 5000) == WAIT_OBJECT_0) {
            instanceMutex.setOwnsMutex(true);
        } else {
            if (HWND existing = FindWindowW(L"NameExchangerClass", L"FilenameExchanger")) {
                ShowWindow(existing, SW_RESTORE);
                SetForegroundWindow(existing);
            }
            return 0;
        }
    } else {
        instanceMutex.setOwnsMutex(true);
    }

    App& app = GetApp();
    if (!app.Init(instance)) return 1;

    const int result = app.Run();
    app.Shutdown();
    return result;
}
