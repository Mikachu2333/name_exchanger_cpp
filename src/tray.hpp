#pragma once

#include <windows.h>

inline constexpr UINT WM_TRAYICON = WM_APP + 1;

// Setup the system tray icon for the given window.
// Uses the application's own icon from resources.
[[nodiscard]] bool SetupTrayIcon(HWND hwnd);

// Remove the system tray icon.
void RemoveTrayIcon();
