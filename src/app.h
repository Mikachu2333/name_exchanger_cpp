#pragma once

#include "d3d_helpers.h"
#include <string>
#include <windows.h>

// Forward declare ImFont
struct ImFont;

// Application state and UI
struct App {
    std::string path1 = "";
    std::string path2 = "";

    bool isTopmost = true;
    bool showWindow = true;
    bool done = false;

    HWND hwnd = nullptr;
    D3DState d3d = {};

    // DPI scaling
    float dpiScale = 1.0f;

    // Fonts
    ImFont* fontIcon = nullptr;

    // Initialize the application (create window, D3D, ImGui, tray)
    bool Init(HINSTANCE hInstance, int argc, wchar_t** argv);

    // Run the main loop
    int Run();

    // Cleanup all resources
    void Shutdown();

    // Render one frame of the UI
    void RenderUI();

    // Create or remove the "Send To" shortcut
    void CreateSendToShortcut(bool remove);

    // Handle the WndProc callback
    LRESULT HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    // Get DPI scale factor for the window
    void UpdateDpiScale();
};

// Global app instance (needed for WndProc callback)
App& GetApp();
