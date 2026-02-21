#pragma once

#include <windows.h>

// Setup the system tray icon for the given window.
// Uses the application's own icon from resources.
void SetupTrayIcon(HWND hwnd);

// Remove the system tray icon.
void RemoveTrayIcon();
