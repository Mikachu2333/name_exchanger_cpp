#include "src/app.h"

#include <shellapi.h>
#include <windows.h>

#ifdef __GNUC__
extern "C" {
void __chkstk() { asm("call ___chkstk_ms"); }
void* __cxa_allocate_exception(size_t thrown_size) { return malloc(thrown_size); }
[[noreturn]] void __cxa_throw(void* /*thrown_exception*/, void* /*tinfo*/, void (* /*dest*/)(void*)) {
    TerminateProcess(GetCurrentProcess(), 1);
    while (true) {
    }
}
void* __gxx_personality_v0 = nullptr;
void* _CxxThrowException = nullptr;
void* __CxxFrameHandler3 = nullptr;
}
// Dummy for MSVC RTTI
asm(".global \"??_7type_info@@6B@\"\n\"??_7type_info@@6B@\": .long 0");
#endif

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPSTR /*lpCmdLine*/, int /*nCmdShow*/) {
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    // Mutex to prevent multiple instances
    HANDLE hMutex = CreateMutexW(nullptr, TRUE, L"D79BA905DBC642759A74162A2FAABC28");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        HWND existingApp = FindWindowW(L"NameExchangerCppClass", L"FilenameExchanger");
        if (existingApp) {
            ShowWindow(existingApp, SW_RESTORE);
            SetForegroundWindow(existingApp);
        }
        LocalFree(argv);
        if (hMutex) {
            CloseHandle(hMutex);
        }
        return 0;
    }

    App& app = GetApp();

    if (!app.Init(hInstance, argc, argv)) {
        LocalFree(argv);
        if (hMutex) {
            ReleaseMutex(hMutex);
            CloseHandle(hMutex);
        }
        return 0;
    }
    LocalFree(argv);

    int result = app.Run();
    app.Shutdown();

    if (hMutex) {
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
    }

    return result;
}
