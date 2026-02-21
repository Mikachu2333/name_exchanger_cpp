#include "app.h"

#include "d3d_helpers.h"
#include "font_data.h"
#include "i18n.h"
#include "tray.h"
#include "utils.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_stdlib.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

#include <minwindef.h>
#include <objbase.h>
#include <shellapi.h>
#include <shlobj.h>
#include <windows.h>
#include <filesystem>
#include <string>

// External function from the Rust library
extern "C" int exchange(const char* path1, const char* path2);

// Forward declaration for ImGui Win32 handler
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static App g_app;

App& GetApp() { return g_app; }

static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) {
        return true;
    }
    return GetApp().HandleMessage(hWnd, msg, wParam, lParam);
}

void App::UpdateDpiScale() {
    // Use GetDpiForWindow (Windows 10 1607+)
    using GetDpiForWindowFn = UINT(WINAPI*)(HWND);
    static auto pGetDpiForWindow =
        reinterpret_cast<GetDpiForWindowFn>(GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetDpiForWindow"));

    UINT dpi = 96;
    if (pGetDpiForWindow && hwnd) {
        dpi = pGetDpiForWindow(hwnd);
    } else {
        // Fallback: use DC DPI
        HDC hdc = GetDC(nullptr);
        if (hdc) {
            dpi = static_cast<UINT>(GetDeviceCaps(hdc, LOGPIXELSX));
            ReleaseDC(nullptr, hdc);
        }
    }
    dpiScale = static_cast<float>(dpi) / 96.0f;
}

bool App::Init(HINSTANCE hInstance, int argc, wchar_t** argv) {
    // Command line mode: 2 args → exchange and exit
    if (argc == 3) {
        std::string p1 = Utf16ToUtf8(argv[1]);
        std::string p2 = Utf16ToUtf8(argv[2]);
        exchange(p1.c_str(), p2.c_str());
        return false;  // Signal to exit
    }

    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    // Create application window
    ImGui_ImplWin32_EnableDpiAwareness();

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_CLASSDC;
    wc.lpfnWndProc = ::WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"NameExchangerClass";
    RegisterClassExW(&wc);

    // Get primary monitor DPI for initial window sizing
    {
        HDC hdc = GetDC(nullptr);
        if (hdc) {
            dpiScale = static_cast<float>(GetDeviceCaps(hdc, LOGPIXELSX)) / 96.0f;
            ReleaseDC(nullptr, hdc);
        }
    }

    int winW = static_cast<int>(362 * dpiScale);
    int winH = static_cast<int>(242 * dpiScale);

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int posX = (screenW - winW) / 2;
    int posY = (screenH - winH) / 2;

    hwnd = CreateWindowExW(WS_EX_TOOLWINDOW, wc.lpszClassName, L"FilenameExchanger", WS_POPUP, posX, posY, winW, winH,
                           nullptr, nullptr, hInstance, nullptr);

    if (!hwnd) {
        return false;
    }

    // Update DPI from actual window
    UpdateDpiScale();
    winW = static_cast<int>(364 * dpiScale);
    winH = static_cast<int>(240 * dpiScale);
    SetWindowPos(hwnd, nullptr, 0, 0, winW, winH, SWP_NOMOVE | SWP_NOZORDER);

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd, d3d)) {
        CleanupDeviceD3D(d3d);
        UnregisterClassW(wc.lpszClassName, hInstance);
        return false;
    }

    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);
    SetForegroundWindow(hwnd);

    if (isTopmost) {
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }

    // Setup tray icon
    SetupTrayIcon(hwnd);

    // Enable drag and drop
    DragAcceptFiles(hwnd, TRUE);

    // Setup ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;  // Disable ini file

    // Custom style (no default theme)
    {
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowBorderSize = 0.0f;
        style.WindowRounding = 0.0f;
        style.FrameRounding = 0.0f;
        style.FrameBorderSize = 0.0f;
        style.Colors[ImGuiCol_WindowBg] = ImVec4(240 / 255.0f, 240 / 255.0f, 240 / 255.0f, 1.0f);
        style.Colors[ImGuiCol_ChildBg] = ImVec4(0, 0, 0, 0);
        style.Colors[ImGuiCol_Text] = ImVec4(0, 0, 0, 1);
        style.Colors[ImGuiCol_Border] = ImVec4(0.70f, 0.70f, 0.70f, 1.0f);
        style.Colors[ImGuiCol_FrameBg] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
        style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.96f, 0.96f, 0.96f, 1.0f);
        style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.92f, 0.92f, 0.92f, 1.0f);
        style.Colors[ImGuiCol_Button] = ImVec4(0.85f, 0.85f, 0.85f, 1.0f);
        style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.78f, 0.78f, 0.78f, 1.0f);
        style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.70f, 0.70f, 0.70f, 1.0f);
        style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.95f, 0.95f, 0.95f, 1.0f);
        style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.70f, 0.70f, 0.70f, 1.0f);
        style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.55f, 0.55f, 0.55f, 1.0f);
        style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.40f, 0.40f, 0.40f, 1.0f);
    }

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(d3d.device, d3d.deviceContext);

    // Load fonts with Chinese support

    // Default font
    LoadMsyhFont(io, 16.0f * dpiScale);

    // Label font (16pt)
    fontLabel = LoadMsyhFont(io, 16.0f * dpiScale);

    // Input font (15pt)
    fontInput = LoadMsyhFont(io, 15.0f * dpiScale);

    // Start button font (24pt)
    fontStartBtn = LoadMsyhFont(io, 24.0f * dpiScale);

    // Icon font (15pt for title bar buttons)
    {
        ImFontConfig cfg;
        cfg.FontDataOwnedByAtlas = false;
        fontIcon = io.Fonts->AddFontFromMemoryTTF(const_cast<unsigned char*>(kIconFontData),
                                                  static_cast<int>(kIconFontDataSize), 15.0f * dpiScale, &cfg);
    }

    return true;
}

int App::Run() {
    while (!done) {
        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            if (msg.message == WM_QUIT) {
                done = true;
            }
        }
        if (done) {
            break;
        }

        if (!showWindow) {
            Sleep(10);
            continue;
        }

        // Handle swap chain occlusion
        if (d3d.swapChainOccluded && d3d.swapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED) {
            Sleep(10);
            continue;
        }
        d3d.swapChainOccluded = false;

        // Handle resize
        if (d3d.resizeWidth != 0 && d3d.resizeHeight != 0) {
            CleanupRenderTarget(d3d);
            d3d.swapChain->ResizeBuffers(0, d3d.resizeWidth, d3d.resizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            d3d.resizeWidth = d3d.resizeHeight = 0;
            CreateRenderTarget(d3d);
        }

        // Start ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        RenderUI();

        // Rendering
        ImGui::Render();
        const float clearColor[4] = {240 / 255.0f, 240 / 255.0f, 240 / 255.0f, 1.0f};
        d3d.deviceContext->OMSetRenderTargets(1, &d3d.renderTargetView, nullptr);
        d3d.deviceContext->ClearRenderTargetView(d3d.renderTargetView, clearColor);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        HRESULT hr = d3d.swapChain->Present(1, 0);  // Present with vsync
        d3d.swapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
    }

    return 0;
}

void App::Shutdown() {
    RemoveTrayIcon();
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDeviceD3D(d3d);

    if (hwnd) {
        DestroyWindow(hwnd);
        hwnd = nullptr;
    }
    UnregisterClassW(L"NameExchangerClass", GetModuleHandleW(nullptr));
    CoUninitialize();
}

void App::RenderUI() {
    const auto& L = GetCurrentLocale();
    const float s = dpiScale;

    float winW = 364 * s;
    float winH = 240 * s;
    float barH = 32 * s;
    float btnSize = 29 * s;
    float btnY = (barH - btnSize) / 2.0f;
    float contentX = 11 * s;
    float inputWidth = winW - contentX * 2;
    float each_width = 6 * s + btnSize;

    // Clear input focus when window is inactive (requirement 9)
    if (GetForegroundWindow() != hwnd) {
        ImGui::ClearActiveID();
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(winW, winH));
    ImGui::Begin("Main", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings);

    // === Top Bar ===
    ImGui::SetCursorPos(ImVec2(0, 0));

    ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(0, 120, 215, 255));
    ImGui::BeginChild("TopBar", ImVec2(winW, barH), false);
    ImGui::PopStyleColor();

    if (fontIcon) {
        ImGui::PushFont(fontIcon);
    }

    // Transparent button background
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 1, 1, 0.15f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1, 1, 1, 0.25f));

    // Pin toggle button - yellow text
    ImGui::SetCursorPos(ImVec2(6 * s, btnY));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
    if (ImGui::Button(isTopmost ? "B" : "A", ImVec2(btnSize, btnSize))) {
        isTopmost = !isTopmost;
        SetWindowPos(hwnd, isTopmost ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }
    ImGui::PopStyleColor();

    // About button - red text
    ImGui::SetCursorPos(ImVec2(winW - each_width * 5, btnY));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
    if (ImGui::Button("C", ImVec2(btnSize, btnSize))) {
        MessageBoxW(hwnd, L.aboutMessageW, L.warningTitle, MB_OK);
    }
    ImGui::PopStyleColor();

    // Remaining 4 buttons - white text
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

    // Admin button
    ImGui::SetCursorPos(ImVec2(winW - each_width * 4, btnY));
    bool isAdmin = IsRunAsAdmin();
    if (ImGui::Button(isAdmin ? "E" : "D", ImVec2(btnSize, btnSize))) {
        wchar_t szPath[MAX_PATH];
        GetModuleFileNameW(nullptr, szPath, ARRAYSIZE(szPath));
        std::filesystem::path p(szPath);
        if (!isAdmin) {
            std::filesystem::path newPath = p.parent_path() / (p.stem().string() + ".EXE");
            std::filesystem::rename(p, newPath);
            if (!RunAsAdmin(true)) {
                std::filesystem::rename(newPath, p);
                MessageBoxW(hwnd, L"Failed to elevate privileges.", L"Error", MB_OK | MB_ICONERROR);
            }
        } else {
            std::filesystem::path newPath = p.parent_path() / (p.stem().string() + ".exe");
            std::filesystem::rename(p, newPath);
            if (!RunAsAdmin(false)) {
                std::filesystem::rename(newPath, p);
                MessageBoxW(hwnd, L"Failed to drop privileges.", L"Error", MB_OK | MB_ICONERROR);
            }
        }
    }

    // SendTo shortcut button
    ImGui::SetCursorPos(ImVec2(winW - each_width * 3, btnY));
    if (ImGui::Button("F", ImVec2(btnSize, btnSize))) {
        CreateSendToShortcut(false);
    }
    if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
        CreateSendToShortcut(true);
    }

    // Minimize button
    ImGui::SetCursorPos(ImVec2(winW - each_width * 2, btnY));
    if (ImGui::Button("G", ImVec2(btnSize, btnSize))) {
        showWindow = false;
        ShowWindow(hwnd, SW_HIDE);
    }

    // Close button
    ImGui::SetCursorPos(ImVec2(winW - each_width, btnY));
    if (ImGui::Button("H", ImVec2(btnSize, btnSize))) {
        done = true;
    }

    ImGui::PopStyleColor();   // white text
    ImGui::PopStyleColor(3);  // button bg colors

    if (fontIcon) {
        ImGui::PopFont();
    }

    // Window drag on empty area of the bar
    if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        ReleaseCapture();
        SendMessageW(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
        ImGui::GetIO().MouseDown[0] = false;
    }

    ImGui::EndChild();

    // === Main Content ===

    // Label 1 (font 10pt, height 17, left-aligned)
    if (fontLabel) ImGui::PushFont(fontLabel);
    ImGui::SetCursorPos(ImVec2(contentX, 42 * s));
    ImGui::Text("%s", L.file1Label);
    if (fontLabel) ImGui::PopFont();

    // Input 1 (font 14pt, height 28, with border + horizontal scrollbar for long text)
    if (fontInput) ImGui::PushFont(fontInput);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4 * s, (28 * s - ImGui::GetFontSize()) / 2.0f));
    ImGui::SetCursorPos(ImVec2(contentX, 62 * s));

    const float path1TextW = ImGui::CalcTextSize(path1.c_str()).x;
    const float path1InnerW = (std::max)(inputWidth, path1TextW + 24.0f * s);
    const float path1ChildH = 28.0f * s + ImGui::GetStyle().ScrollbarSize;

    ImGui::BeginChild("##path1_scroll", ImVec2(inputWidth, path1ChildH), false, ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::SetNextItemWidth(path1InnerW);
    ImGui::InputText("##path1", &path1);
    ImGui::EndChild();

    ImGui::PopStyleVar(2);
    if (fontInput) ImGui::PopFont();

    // Label 2 (font 10pt, height 17, left-aligned)
    if (fontLabel) ImGui::PushFont(fontLabel);
    ImGui::SetCursorPos(ImVec2(contentX, 110 * s));
    ImGui::Text("%s", L.file2Label);
    if (fontLabel) ImGui::PopFont();

    // Input 2 (font 14pt, height 28, with border + horizontal scrollbar for long text)
    if (fontInput) ImGui::PushFont(fontInput);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4 * s, (28 * s - ImGui::GetFontSize()) / 2.0f));
    ImGui::SetCursorPos(ImVec2(contentX, 130 * s));

    const float path2TextW = ImGui::CalcTextSize(path2.c_str()).x;
    const float path2InnerW = (std::max)(inputWidth, path2TextW + 24.0f * s);
    const float path2ChildH = 28.0f * s + ImGui::GetStyle().ScrollbarSize;

    ImGui::BeginChild("##path2_scroll", ImVec2(inputWidth, path2ChildH), false, ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::SetNextItemWidth(path2InnerW);
    ImGui::InputText("##path2", &path2);
    ImGui::EndChild();

    ImGui::PopStyleVar(2);
    if (fontInput) ImGui::PopFont();

    // Exchange button (font 20pt)
    if (fontStartBtn) ImGui::PushFont(fontStartBtn);
    float btnW = 124 * s;
    float btnH2 = 48 * s;
    ImGui::SetCursorPos(ImVec2((winW - btnW) / 2.0f, 180 * s));
    if (ImGui::Button(L.startButton, ImVec2(btnW, btnH2))) {
        int returnId = exchange(path1.c_str(), path2.c_str());
        const char* info = GetOutputInfo(returnId);
        if (returnId == 0) {
            path1.clear();
            path2.clear();
        } else {
            std::wstring winfo = Utf8ToUtf16(info);
            MessageBoxW(hwnd, winfo.c_str(), L.errorTitle, MB_OK | MB_ICONERROR);
        }
    }
    if (fontStartBtn) ImGui::PopFont();

    ImGui::End();
    ImGui::PopStyleVar();  // WindowPadding
}

void App::CreateSendToShortcut(bool remove) {
    const auto& L = GetCurrentLocale();

    // Use SHGetKnownFolderPath instead of deprecated SHGetFolderPathW
    wchar_t* sendToPath = nullptr;
    if (FAILED(SHGetKnownFolderPath(FOLDERID_SendTo, 0, nullptr, &sendToPath))) {
        return;
    }

    std::wstring shortcutPath = std::wstring(sendToPath) + L"\\name_exchanger.lnk";
    CoTaskMemFree(sendToPath);

    if (remove) {
        DeleteFileW(shortcutPath.c_str());
        MessageBoxW(hwnd, L.shortcutRemoved, L.tipsTitle, MB_OK);
    } else {
        IShellLinkW* psl = nullptr;
        if (SUCCEEDED(CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLinkW,
                                       reinterpret_cast<void**>(&psl)))) {
            wchar_t exePath[MAX_PATH];
            GetModuleFileNameW(nullptr, exePath, MAX_PATH);
            psl->SetPath(exePath);
            psl->SetDescription(L"FilenameExchanger");
            psl->SetIconLocation(exePath, 0);

            IPersistFile* ppf = nullptr;
            if (SUCCEEDED(psl->QueryInterface(IID_IPersistFile, reinterpret_cast<void**>(&ppf)))) {
                ppf->Save(shortcutPath.c_str(), TRUE);
                ppf->Release();
                MessageBoxW(hwnd, L.shortcutCreated, L.tipsTitle, MB_OK);
            }
            psl->Release();
        }
    }
}

LRESULT App::HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_SIZE:
            if (wParam == SIZE_MINIMIZED) {
                return 0;
            }
            d3d.resizeWidth = LOWORD(lParam);
            d3d.resizeHeight = HIWORD(lParam);
            return 0;

        case WM_SYSCOMMAND:
            if ((wParam & 0xfff0) == SC_KEYMENU) {
                return 0;  // Disable ALT application menu
            }
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        case WM_DPICHANGED: {
            UpdateDpiScale();
            RECT* pRect = reinterpret_cast<RECT*>(lParam);
            SetWindowPos(hwnd, nullptr, pRect->left, pRect->top, pRect->right - pRect->left, pRect->bottom - pRect->top,
                         SWP_NOZORDER | SWP_NOACTIVATE);

            // Rebuild fonts with new DPI
            ImGuiIO& io = ImGui::GetIO();
            io.Fonts->Clear();

            LoadMsyhFont(io, 16.0f * dpiScale);
            fontLabel = LoadMsyhFont(io, 16.0f * dpiScale);
            fontInput = LoadMsyhFont(io, 15.0f * dpiScale);
            fontStartBtn = LoadMsyhFont(io, 24.0f * dpiScale);

            ImFontConfig cfg;
            cfg.FontDataOwnedByAtlas = false;
            fontIcon = io.Fonts->AddFontFromMemoryTTF(const_cast<unsigned char*>(kIconFontData),
                                                      static_cast<int>(kIconFontDataSize), 15.0f * dpiScale, &cfg);

            ImGui_ImplDX11_InvalidateDeviceObjects();
            return 0;
        }

        case WM_DROPFILES: {
            HDROP hDrop = reinterpret_cast<HDROP>(wParam);
            UINT count = DragQueryFileW(hDrop, 0xFFFFFFFF, nullptr, 0);

            if (count == 1) {
                wchar_t file[MAX_PATH];
                DragQueryFileW(hDrop, 0, file, MAX_PATH);
                std::string u8file = Utf16ToUtf8(file);

                if (path1.empty()) {
                    path1 = u8file;
                } else if (path2.empty()) {
                    path2 = u8file;
                } else {
                    path1 = u8file;
                    path2.clear();
                }
            } else if (count >= 2) {
                wchar_t file1[MAX_PATH];
                wchar_t file2[MAX_PATH];
                DragQueryFileW(hDrop, 0, file1, MAX_PATH);
                DragQueryFileW(hDrop, 1, file2, MAX_PATH);
                path1 = Utf16ToUtf8(file1);
                path2 = Utf16ToUtf8(file2);
            }
            DragFinish(hDrop);
            return 0;
        }

        case WM_USER + 1: {
            switch (lParam) {
                case WM_LBUTTONUP:
                    showWindow = !showWindow;
                    ShowWindow(hwnd, showWindow ? SW_SHOW : SW_HIDE);
                    if (showWindow) {
                        SetForegroundWindow(hwnd);
                    }
                    break;
                case WM_RBUTTONUP:
                    PostQuitMessage(0);
                    break;
            }
            return 0;
        }
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

auto App::LoadMsyhFont(ImGuiIO& io, float size) -> ImFont* {
    const char* fps[] = {"c:\\Windows\\Fonts\\msyh.ttc", "c:\\Windows\\Fonts\\msyh.ttf"};
    for (const char* fp : fps) {
        if (GetFileAttributesA(fp) != INVALID_FILE_ATTRIBUTES) {
            return io.Fonts->AddFontFromFileTTF(fp, size, nullptr, io.Fonts->GetGlyphRangesChineseFull());
        }
    }
    ImFontConfig cfg;
    cfg.SizePixels = size;
    return io.Fonts->AddFontDefault(&cfg);
};