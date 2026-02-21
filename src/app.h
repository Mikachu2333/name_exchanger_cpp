#pragma once

#include "d3d_helpers.h"
#include "imgui.h"
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
    ImFont* fontLabel = nullptr;
    ImFont* fontInput = nullptr;
    ImFont* fontStartBtn = nullptr;

    // Theme colors (sync with Windows app theme)
    ImVec4 clearColor = ImVec4(0.94f, 0.94f, 0.94f, 1.0f);
    ImVec4 topBarBgColor = ImVec4(0.00f, 0.47f, 0.84f, 1.0f);
    ImVec4 topBarTextColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    ImVec4 topBarButtonHoveredColor = ImVec4(1.0f, 1.0f, 1.0f, 0.15f);
    ImVec4 topBarButtonActiveColor = ImVec4(1.0f, 1.0f, 1.0f, 0.25f);
    ImVec4 tooltipBgColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    ImVec4 tooltipTextColor = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);

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

    // Apply colors from current Windows theme settings
    void ApplySystemTheme();

    // Load MSYH font with the given size
    ImFont* LoadMsyhFont(ImGuiIO& io, float size);
};

// Global app instance (needed for WndProc callback)
App& GetApp();
