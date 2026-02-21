#pragma once

#include <string>
#include <windows.h>

// Convert UTF-16 (wchar_t) to UTF-8 (std::string)
std::string Utf16ToUtf8(const std::wstring &wstr);

// Convert UTF-8 (std::string) to UTF-16 (std::wstring)
std::wstring Utf8ToUtf16(const std::string &str);

// Check if the current process is running as administrator
bool IsRunAsAdmin();

// Relaunch the current process with admin privileges (UAC prompt)
void RunAsAdmin();
