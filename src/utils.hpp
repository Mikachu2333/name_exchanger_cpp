#pragma once

#include <string>
#include <windows.h>

inline constexpr wchar_t PROCESS_MUTEX_GUID[] = L"Local\\CFFD3CF9A003453C9893A8CD49EF7ED5";

// Strict conversions. Invalid input or an oversized string returns an empty string.
std::string Utf16ToUtf8(const std::wstring& wstr);
std::wstring Utf8ToUtf16(const std::string& str);

// Return the complete executable path, or an empty path on failure/truncation.
std::wstring GetExecutablePath();

// Check if the current process is running as administrator.
bool IsRunAsAdmin();

// The executable extension stores the preferred mode: .EXE = administrator, .exe = standard.
// Returns the current preference, or false when the executable does not have an exe extension.
bool IsAdminModePreferred();

// Persist the requested mode by changing the executable extension case, then relaunch.
// A failed launch attempts to restore the original executable name.
bool RunAsAdmin(bool privilege);
