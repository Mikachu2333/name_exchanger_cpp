#pragma once

#include <string>
#include <windows.h>

inline constexpr wchar_t PROCESS_MUTEX_GUID[] = L"Local\\CFFD3CF9A003453C9893A8CD49EF7ED5";

// Strict conversions. Invalid input or an oversized string returns an empty string.
std::string Utf16ToUtf8(const std::wstring& wstr);
std::wstring Utf8ToUtf16(const std::string& str);

// Return the complete executable path, or an empty path on failure/truncation.
std::wstring GetExecutablePath();

// Check if the current process is running as administrator
bool IsRunAsAdmin();

// Relaunch the current process with admin privileges (UAC prompt)
bool RunAsAdmin(bool privilege);
