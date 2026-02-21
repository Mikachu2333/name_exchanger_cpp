#pragma once

#include <string>
#include <windows.h>

const wchar_t PROCESS_MUTEX_GUID[] = L"CFFD3CF9A003453C9893A8CD49EF7ED5";

// Convert UTF-16 (wchar_t) to UTF-8 (std::string)
std::string Utf16ToUtf8(const std::wstring &wstr);

// Convert UTF-8 (std::string) to UTF-16 (std::wstring)
std::wstring Utf8ToUtf16(const std::string &str);

// Check if the current process is running as administrator
bool IsRunAsAdmin();

// Relaunch the current process with admin privileges (UAC prompt)
bool RunAsAdmin(bool privilege);
