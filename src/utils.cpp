#include "utils.h"

#include <shellapi.h>

std::string Utf16ToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) {
        return {};
    }
    int sizeNeeded =
        WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), nullptr, 0, nullptr, nullptr);
    if (sizeNeeded <= 0) {
        return {};
    }
    std::string result(static_cast<size_t>(sizeNeeded), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), result.data(), sizeNeeded, nullptr,
                        nullptr);
    return result;
}

std::wstring Utf8ToUtf16(const std::string& str) {
    if (str.empty()) {
        return {};
    }
    int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), nullptr, 0);
    if (sizeNeeded <= 0) {
        return {};
    }
    std::wstring result(static_cast<size_t>(sizeNeeded), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), result.data(), sizeNeeded);
    return result;
}

bool IsRunAsAdmin() {
    BOOL isAdmin = FALSE;
    PSID adminGroup = nullptr;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;

    if (AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0,
                                 0, &adminGroup)) {
        CheckTokenMembership(nullptr, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }
    return isAdmin != FALSE;
}

static bool LaunchUnelevatedViaExplorer(const wchar_t* exePath) {
    HWND shellWnd = GetShellWindow();
    if (!shellWnd) return false;

    DWORD explorerPid = 0;
    GetWindowThreadProcessId(shellWnd, &explorerPid);
    if (!explorerPid) return false;

    HANDLE hExplorer = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, explorerPid);
    if (!hExplorer) return false;

    HANDLE hExplorerToken = nullptr;
    BOOL ok = OpenProcessToken(hExplorer, TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY | TOKEN_QUERY, &hExplorerToken);
    CloseHandle(hExplorer);
    if (!ok || !hExplorerToken) return false;

    HANDLE hPrimaryToken = nullptr;
    ok = DuplicateTokenEx(
        hExplorerToken,
        TOKEN_ASSIGN_PRIMARY | TOKEN_DUPLICATE | TOKEN_QUERY | TOKEN_ADJUST_DEFAULT | TOKEN_ADJUST_SESSIONID, nullptr,
        SecurityImpersonation, TokenPrimary, &hPrimaryToken);
    CloseHandle(hExplorerToken);
    if (!ok || !hPrimaryToken) return false;

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};

    ok = CreateProcessWithTokenW(hPrimaryToken, 0, exePath, nullptr, 0, nullptr, nullptr, &si, &pi);

    CloseHandle(hPrimaryToken);

    if (ok) {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        return true;
    }
    return false;
}

extern HANDLE g_hMutex;

bool RunAsAdmin(bool privilege) {
    wchar_t szPath[1024];
    if (!GetModuleFileNameW(nullptr, szPath, ARRAYSIZE(szPath))) {
        return false;
    }

    // Release the mutex so the new elevated instance can start
    if (g_hMutex) {
        ReleaseMutex(g_hMutex);
        CloseHandle(g_hMutex);
        g_hMutex = nullptr;
    }

    if (privilege) {
        SHELLEXECUTEINFOW sei = {};
        sei.cbSize = sizeof(sei);
        sei.lpVerb = L"runas";
        sei.lpFile = szPath;
        sei.hwnd = nullptr;
        sei.nShow = SW_NORMAL;
        if (ShellExecuteExW(&sei)) {
            ExitProcess(0);
        }
    } else {
        if (LaunchUnelevatedViaExplorer(szPath)) {
            ExitProcess(0);
        }
    }
    // If ShellExecuteExW failed, user refused elevation — re-acquire the mutex
    g_hMutex = CreateMutexW(nullptr, TRUE, PROCESS_MUTEX_GUID);
    return false;
}
