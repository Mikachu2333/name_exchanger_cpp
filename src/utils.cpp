#include "utils.hpp"

#include <shellapi.h>

#include <limits>
#include <vector>

namespace {
[[nodiscard]] bool FitsInt(size_t size) noexcept {
    return size <= static_cast<size_t>((std::numeric_limits<int>::max)());
}
}

std::string Utf16ToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return {};
    if (!FitsInt(wstr.size())) return {};

    const int sourceSize = static_cast<int>(wstr.size());
    const int sizeNeeded = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wstr.data(), sourceSize, nullptr, 0,
                                               nullptr, nullptr);
    if (sizeNeeded <= 0) return {};

    std::string result(static_cast<size_t>(sizeNeeded), '\0');
    if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wstr.data(), sourceSize, result.data(), sizeNeeded, nullptr,
                            nullptr) != sizeNeeded) {
        return {};
    }
    return result;
}

std::wstring Utf8ToUtf16(const std::string& str) {
    if (str.empty()) return {};
    if (!FitsInt(str.size())) return {};

    const int sourceSize = static_cast<int>(str.size());
    const int sizeNeeded = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str.data(), sourceSize, nullptr, 0);
    if (sizeNeeded <= 0) return {};

    std::wstring result(static_cast<size_t>(sizeNeeded), L'\0');
    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str.data(), sourceSize, result.data(), sizeNeeded) !=
        sizeNeeded) {
        return {};
    }
    return result;
}

std::wstring GetExecutablePath() {
    std::vector<wchar_t> buffer(512);
    for (;;) {
        SetLastError(ERROR_SUCCESS);
        const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (length == 0) return {};
        if (length < buffer.size() - 1 || (length < buffer.size() && GetLastError() != ERROR_INSUFFICIENT_BUFFER)) {
            return std::wstring(buffer.data(), length);
        }
        if (buffer.size() >= 32768) return {};
        buffer.resize((std::min)(buffer.size() * 2, static_cast<size_t>(32768)));
    }
}

bool IsRunAsAdmin() {
    BOOL isAdmin = FALSE;
    PSID adminGroup = nullptr;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;

    if (AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0,
                                 0, &adminGroup)) {
        if (!CheckTokenMembership(nullptr, adminGroup, &isAdmin)) isAdmin = FALSE;
        FreeSid(adminGroup);
    }
    return isAdmin != FALSE;
}

namespace {
bool LaunchUnelevatedViaExplorer(const std::wstring& exePath) {
    const HWND shellWindow = GetShellWindow();
    if (!shellWindow) return false;

    DWORD explorerPid = 0;
    GetWindowThreadProcessId(shellWindow, &explorerPid);
    if (!explorerPid) return false;

    HANDLE explorer = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, explorerPid);
    if (!explorer) return false;

    HANDLE explorerToken = nullptr;
    BOOL ok = OpenProcessToken(explorer, TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY | TOKEN_QUERY, &explorerToken);
    CloseHandle(explorer);
    if (!ok || !explorerToken) return false;

    HANDLE primaryToken = nullptr;
    ok = DuplicateTokenEx(explorerToken,
                          TOKEN_ASSIGN_PRIMARY | TOKEN_DUPLICATE | TOKEN_QUERY | TOKEN_ADJUST_DEFAULT |
                              TOKEN_ADJUST_SESSIONID,
                          nullptr, SecurityImpersonation, TokenPrimary, &primaryToken);
    CloseHandle(explorerToken);
    if (!ok || !primaryToken) return false;

    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    std::wstring commandLine = L"\"" + exePath + L"\" --gui-relaunch";
    ok = CreateProcessWithTokenW(primaryToken, 0, exePath.c_str(), commandLine.data(), 0, nullptr, nullptr, &startup,
                                 &process);
    CloseHandle(primaryToken);

    if (!ok) return false;
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return true;
}
}

bool RunAsAdmin(bool privilege) {
    const std::wstring executable = GetExecutablePath();
    if (executable.empty()) return false;

    if (privilege) {
        SHELLEXECUTEINFOW execute{};
        execute.cbSize = sizeof(execute);
        execute.fMask = SEE_MASK_NOCLOSEPROCESS;
        execute.lpVerb = L"runas";
        execute.lpFile = executable.c_str();
        execute.lpParameters = L"--gui-relaunch";
        execute.nShow = SW_NORMAL;
        if (!ShellExecuteExW(&execute)) return false;
        if (execute.hProcess) CloseHandle(execute.hProcess);
    } else if (!LaunchUnelevatedViaExplorer(executable)) {
        return false;
    }

    // Let the normal main loop perform orderly cleanup. The caller closes the old instance.
    return true;
}
