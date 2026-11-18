#include "app.hpp"

#include "d3d_helpers.hpp"
#include "exchange_name_lib.hpp"
#include "font_data.hpp"
#include "i18n.hpp"
#include "tray.hpp"
#include "utils.hpp"

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_stdlib.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

#include <shellapi.h>
#include <shlobj.h>
#include <windows.h>
#include <algorithm>
#include <cctype>
#include <dwmapi.h>
#include <filesystem>
#include <limits>
#include <optional>
#include <string>
#include <utility>

// Forward declaration for ImGui Win32 handler
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static App g_app;

App& GetApp() { return g_app; }

namespace {
std::string ReadDroppedPath(HDROP drop, UINT index) {
    const UINT length = DragQueryFileW(drop, index, nullptr, 0);
    if (!length) return {};
    std::wstring path(length + 1, L'\0');
    if (DragQueryFileW(drop, index, path.data(), length + 1) != length) return {};
    path.resize(length);
    return Utf16ToUtf8(path);
}

ImFont* LoadTextFont(ImGuiIO& io) {
    wchar_t windowsDirectory[MAX_PATH]{};
    const UINT length = GetWindowsDirectoryW(windowsDirectory, static_cast<UINT>(std::size(windowsDirectory)));
    if (!length || length >= std::size(windowsDirectory)) return io.Fonts->AddFontDefault();
    const auto directory = std::filesystem::path(windowsDirectory) / L"Fonts";
    ImFontConfig config;
    config.Flags |= ImFontFlags_NoLoadError;
    const auto load = [&](const wchar_t* filename) {
        const auto path = Utf16ToUtf8((directory / filename).wstring());
        return io.Fonts->AddFontFromFileTTF(path.c_str(), 16.0f, &config);
    };
    ImFont* base = nullptr;
    for (const auto* filename : {L"msyh.ttc", L"msyh.ttf", L"msjh.ttc", L"segoeui.ttf"}) {
        base = load(filename);
        if (base) break;
    }
    if (!base) base = io.Fonts->AddFontDefault();
    config.MergeMode = true;
    for (const auto* filename : {L"simsunb.ttf", L"seguiemj.ttf"}) load(filename);
    return base;
}

ImVec4 ToImVec4(COLORREF color, float alpha = 1.0f) {
    return ImVec4(GetRValue(color) / 255.0f, GetGValue(color) / 255.0f, GetBValue(color) / 255.0f, alpha);
}

float Luminance(const ImVec4& color) { return 0.2126f * color.x + 0.7152f * color.y + 0.0722f * color.z; }

ImVec4 ShiftLuminanceToRange(const ImVec4& color, float minLum, float maxLum) {
    const float lum = Luminance(color);
    float delta = 0.0f;
    if (lum < minLum) {
        delta = minLum - lum;
    } else if (lum > maxLum) {
        delta = maxLum - lum;
    }

    ImVec4 out = color;
    out.x = std::clamp(out.x + delta, 0.0f, 1.0f);
    out.y = std::clamp(out.y + delta, 0.0f, 1.0f);
    out.z = std::clamp(out.z + delta, 0.0f, 1.0f);
    return out;
}

bool IsWindowsAppsDarkMode() {
    DWORD value = 1;
    DWORD size = sizeof(value);
    const LSTATUS status =
        RegGetValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                     L"AppsUseLightTheme", RRF_RT_REG_DWORD, nullptr, &value, &size);
    if (status != ERROR_SUCCESS) {
        return false;
    }
    return value == 0;
}

ImVec4 GetWindowsAccentColor() {
    DWORD argb = 0;
    BOOL opaqueBlend = FALSE;
    if (SUCCEEDED(DwmGetColorizationColor(&argb, &opaqueBlend))) {
        return ImVec4(((argb >> 16) & 0xFF) / 255.0f, ((argb >> 8) & 0xFF) / 255.0f, (argb & 0xFF) / 255.0f, 1.0f);
    }
    return ToImVec4(GetSysColor(COLOR_HIGHLIGHT));
}

bool WriteWideTextToStderr(const std::wstring& text) {
    HANDLE hErr = GetStdHandle(STD_ERROR_HANDLE);
    if (hErr == nullptr || hErr == INVALID_HANDLE_VALUE) {
        return false;
    }

    DWORD mode = 0;
    if (GetConsoleMode(hErr, &mode) != 0) {
        DWORD written = 0;
        return WriteConsoleW(hErr, text.c_str(), static_cast<DWORD>(text.size()), &written, nullptr) != 0;
    }

    const int utf8Size =
        WideCharToMultiByte(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
    if (utf8Size <= 0) {
        return false;
    }

    std::string utf8(static_cast<size_t>(utf8Size), '\0');
    const int converted = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), utf8.data(),
                                              utf8Size, nullptr, nullptr);
    if (converted <= 0) {
        return false;
    }

    DWORD written = 0;
    return WriteFile(hErr, utf8.data(), static_cast<DWORD>(utf8.size()), &written, nullptr) != 0;
}

void PrintCommandLineUsageToConsole(const std::wstring& message) {
    const std::wstring output = message + L"\r\n";
    if (WriteWideTextToStderr(output)) {
        return;
    }

    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        WriteWideTextToStderr(output);
        FreeConsole();
    }
}

std::optional<bool> ParsePreserveFlag(const wchar_t* rawFlag) {
    std::string flag = Utf16ToUtf8(rawFlag ? rawFlag : L"");
    std::transform(flag.begin(), flag.end(), flag.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    if (flag == "true" || flag == "t" || flag == "yes" || flag == "y" || flag == "1") return true;
    if (flag == "false" || flag == "f" || flag == "no" || flag == "n" || flag == "0") return false;
    return std::nullopt;
}

int ExchangePaths(const std::string& first, const std::string& second, bool preserve) {
    return static_cast<int>(exchange_n(reinterpret_cast<const uint8_t*>(first.data()), first.size(),
                                       reinterpret_cast<const uint8_t*>(second.data()), second.size(),
                                       static_cast<uint8_t>(preserve)));
}

void ShowCommandLineError(int returnId, bool includeUsage) {
    if (returnId == 0) return;

    const auto& L = GetCurrentLocale();
    std::wstring message = std::wstring(L.cmdErrorPrefix) + Utf8ToUtf16(GetOutputInfo(returnId));
    if (includeUsage) message += std::wstring(L"\n\n") + L.cmdUsage;
    PrintCommandLineUsageToConsole(message);
}

}  // namespace

static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) {
        return TRUE;
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

void App::ApplySystemTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowBorderSize = 0.0f;
    style.WindowRounding = 0.0f;
    style.FrameRounding = 0.0f;
    style.FrameBorderSize = 0.0f;
    style.Colors[ImGuiCol_ChildBg] = ImVec4(0, 0, 0, 0);

    const bool darkMode = IsWindowsAppsDarkMode();

    ImVec4 windowBg = ToImVec4(GetSysColor(COLOR_WINDOW));
    ImVec4 text = ToImVec4(GetSysColor(COLOR_WINDOWTEXT));
    ImVec4 border = ToImVec4(GetSysColor(COLOR_3DSHADOW));
    ImVec4 frameBg = ToImVec4(GetSysColor(COLOR_BTNFACE));
    ImVec4 scrollbarBg = ToImVec4(GetSysColor(COLOR_SCROLLBAR));
    ImVec4 accent = GetWindowsAccentColor();

    if (darkMode) {
        windowBg = ShiftLuminanceToRange(windowBg, 0.08f, 0.18f);
        text = ShiftLuminanceToRange(text, 0.82f, 0.96f);
        border = ShiftLuminanceToRange(border, 0.28f, 0.42f);
        frameBg = ShiftLuminanceToRange(frameBg, 0.14f, 0.24f);
        scrollbarBg = ShiftLuminanceToRange(scrollbarBg, 0.12f, 0.22f);
        accent = ShiftLuminanceToRange(accent, 0.08f, 0.12f);
    } else {
        windowBg = ShiftLuminanceToRange(windowBg, 0.90f, 0.98f);
        text = ShiftLuminanceToRange(text, 0.05f, 0.20f);
        border = ShiftLuminanceToRange(border, 0.50f, 0.70f);
        frameBg = ShiftLuminanceToRange(frameBg, 0.94f, 1.00f);
        scrollbarBg = ShiftLuminanceToRange(scrollbarBg, 0.90f, 0.98f);
        accent = ShiftLuminanceToRange(accent, 0.35f, 0.62f);
    }

    style.Colors[ImGuiCol_WindowBg] = windowBg;
    style.Colors[ImGuiCol_Text] = text;
    style.Colors[ImGuiCol_Border] = border;
    style.Colors[ImGuiCol_FrameBg] = frameBg;
    style.Colors[ImGuiCol_FrameBgHovered] =
        ShiftLuminanceToRange(frameBg, darkMode ? 0.20f : 0.88f, darkMode ? 0.32f : 0.94f);
    style.Colors[ImGuiCol_FrameBgActive] =
        ShiftLuminanceToRange(frameBg, darkMode ? 0.24f : 0.82f, darkMode ? 0.38f : 0.90f);
    style.Colors[ImGuiCol_Button] = ShiftLuminanceToRange(frameBg, darkMode ? 0.18f : 0.90f, darkMode ? 0.28f : 0.96f);
    style.Colors[ImGuiCol_ButtonHovered] =
        ShiftLuminanceToRange(frameBg, darkMode ? 0.24f : 0.82f, darkMode ? 0.36f : 0.92f);
    style.Colors[ImGuiCol_ButtonActive] =
        ShiftLuminanceToRange(frameBg, darkMode ? 0.30f : 0.75f, darkMode ? 0.44f : 0.86f);
    style.Colors[ImGuiCol_ScrollbarBg] = scrollbarBg;
    style.Colors[ImGuiCol_ScrollbarGrab] =
        ShiftLuminanceToRange(accent, darkMode ? 0.36f : 0.45f, darkMode ? 0.55f : 0.65f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] =
        ShiftLuminanceToRange(accent, darkMode ? 0.46f : 0.55f, darkMode ? 0.62f : 0.75f);
    style.Colors[ImGuiCol_ScrollbarGrabActive] =
        ShiftLuminanceToRange(accent, darkMode ? 0.56f : 0.65f, darkMode ? 0.72f : 0.85f);

    clearColor = windowBg;
    topBarBgColor = accent;
    topBarTextColor =
        (Luminance(topBarBgColor) > 0.50f) ? ImVec4(0.0f, 0.0f, 0.0f, 1.0f) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    topBarButtonHoveredColor = ImVec4(topBarTextColor.x, topBarTextColor.y, topBarTextColor.z, 0.14f);
    topBarButtonActiveColor = ImVec4(topBarTextColor.x, topBarTextColor.y, topBarTextColor.z, 0.24f);
    tooltipBgColor = ShiftLuminanceToRange(windowBg, darkMode ? 0.16f : 0.94f, darkMode ? 0.24f : 1.0f);
    tooltipTextColor = text;
}

int App::RunCommandLine(int argc, wchar_t** argv) {
    if ((argc != 3 && argc != 4) || !argv) {
        PrintCommandLineUsageToConsole(GetCurrentLocale().cmdUsage);
        return 5;
    }

    const std::string first = Utf16ToUtf8(argv[1] ? argv[1] : L"");
    const std::string second = Utf16ToUtf8(argv[2] ? argv[2] : L"");
    if (first.empty() || second.empty()) {
        ShowCommandLineError(5, true);
        return 5;
    }

    bool preserve = true;
    if (argc == 4) {
        const std::optional<bool> parsed = ParsePreserveFlag(argv[3]);
        if (!parsed) {
            ShowCommandLineError(5, true);
            return 5;
        }
        preserve = *parsed;
    }

    const int result = ExchangePaths(first, second, preserve);
    ShowCommandLineError(result, false);
    return result;
}

bool App::Init(HINSTANCE hInstance) {
    isAdmin = IsRunAsAdmin();
    const HRESULT comResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    comInitialized = SUCCEEDED(comResult);
    if (FAILED(comResult) && comResult != RPC_E_CHANGED_MODE) return false;

    // Create application window
    ImGui_ImplWin32_EnableDpiAwareness();

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_CLASSDC;
    wc.lpfnWndProc = ::WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"NameExchangerClass";
    if (!RegisterClassExW(&wc)) {
        if (comInitialized) CoUninitialize();
        comInitialized = false;
        return false;
    }

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
        UnregisterClassW(wc.lpszClassName, hInstance);
        if (comInitialized) CoUninitialize();
        comInitialized = false;
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
        DestroyWindow(hwnd);
        hwnd = nullptr;
        UnregisterClassW(wc.lpszClassName, hInstance);
        if (comInitialized) CoUninitialize();
        comInitialized = false;
        return false;
    }

    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);
    SetForegroundWindow(hwnd);

    if (isTopmost) {
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }

    // A tray failure is non-fatal; the window remains fully usable.
    taskbarCreatedMessage = RegisterWindowMessageW(L"TaskbarCreated");
    if (taskbarCreatedMessage) ChangeWindowMessageFilterEx(hwnd, taskbarCreatedMessage, MSGFLT_ALLOW, nullptr);
    if (!SetupTrayIcon(hwnd)) ShowWindow(hwnd, SW_SHOW);

    // Enable drag and drop
    DragAcceptFiles(hwnd, TRUE);
    ChangeWindowMessageFilterEx(hwnd, WM_DROPFILES, MSGFLT_ALLOW, nullptr);
    ChangeWindowMessageFilterEx(hwnd, 0x0049 /* WM_COPYGLOBALDATA, required by shell drag/drop */, MSGFLT_ALLOW,
                                nullptr);

    // Setup ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;  // Disable ini file

    ApplySystemTheme();

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(d3d.device, d3d.deviceContext);

    // Load fonts
    fontLabel = LoadTextFont(io);
    ImGui::GetStyle().FontScaleDpi = dpiScale;

    // Icon font (15pt for title bar buttons)
    {
        ImFontConfig cfg;
        cfg.FontDataOwnedByAtlas = false;
        fontIcon = io.Fonts->AddFontFromMemoryTTF(const_cast<unsigned char*>(kIconFontData),
                                                  static_cast<int>(kIconFontDataSize), 15.0f, &cfg);
    }

    return true;
}

int App::Run() {
    lastInteractionTick = GetTickCount64();
    ULONGLONG retryRendererAt = 0;
    while (!done) {
        MSG msg{};
        while (PeekMessageW(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            if (msg.message == WM_QUIT) return 0;
            if ((msg.message >= WM_MOUSEFIRST && msg.message <= WM_MOUSELAST) ||
                (msg.message >= WM_KEYFIRST && msg.message <= WM_KEYLAST)) {
                lastInteractionTick = GetTickCount64();
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        if (done) {
            break;
        }

        if (!IsWindowVisible(hwnd) || IsIconic(hwnd)) {
            MsgWaitForMultipleObjectsEx(0, nullptr, INFINITE, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
            continue;
        }

        if (!d3d.device) {
            const ULONGLONG now = GetTickCount64();
            if (now < retryRendererAt) {
                MsgWaitForMultipleObjectsEx(0, nullptr, static_cast<DWORD>(retryRendererAt - now), QS_ALLINPUT,
                                            MWMO_INPUTAVAILABLE);
                continue;
            }
            if (!CreateDeviceD3D(hwnd, d3d) || !ImGui_ImplDX11_Init(d3d.device, d3d.deviceContext) ||
                !ImGui_ImplDX11_CreateDeviceObjects()) {
                ReleaseRenderer();
                retryRendererAt = GetTickCount64() + 500;
                continue;
            }
        }

        // Handle swap chain occlusion without busy polling.
        if (d3d.swapChainOccluded) {
            const HRESULT result = d3d.swapChain->Present(0, DXGI_PRESENT_TEST);
            if (result == DXGI_STATUS_OCCLUDED) {
                MsgWaitForMultipleObjectsEx(0, nullptr, 100, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
                continue;
            }
            if (FAILED(result)) {
                if (HandleGraphicsError(result)) continue;
                return 2;
            }
        }
        d3d.swapChainOccluded = false;

        // Handle resize
        if (d3d.resizeWidth != 0 && d3d.resizeHeight != 0) {
            const UINT width = d3d.resizeWidth;
            const UINT height = d3d.resizeHeight;
            d3d.resizeWidth = d3d.resizeHeight = 0;
            d3d.deviceContext->OMSetRenderTargets(0, nullptr, nullptr);
            CleanupRenderTarget(d3d);
            HRESULT result = d3d.swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
            if (SUCCEEDED(result)) result = CreateRenderTarget(d3d);
            if (FAILED(result)) {
                if (HandleGraphicsError(result)) continue;
                return 2;
            }
        }

        if (pendingDpi) {
            dpiScale = static_cast<float>(pendingDpi) / 96.0f;
            ImGui::GetStyle().FontScaleDpi = dpiScale;
            optionWidth = 0.0f;
            pathWidths.fill(-1.0f);
            pendingDpi = 0;
        }
        // Start ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        RenderUI();

        // Rendering
        ImGui::Render();
        const float clearColorData[4] = {clearColor.x, clearColor.y, clearColor.z, clearColor.w};
        d3d.deviceContext->OMSetRenderTargets(1, &d3d.renderTargetView, nullptr);
        d3d.deviceContext->ClearRenderTargetView(d3d.renderTargetView, clearColorData);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        const HRESULT presentResult = d3d.swapChain->Present(1, 0);  // Present with vsync
        d3d.swapChainOccluded = (presentResult == DXGI_STATUS_OCCLUDED);
        if (FAILED(presentResult)) {
            if (HandleGraphicsError(presentResult)) continue;
            return 2;
        }

        // A static immediate-mode UI does not need 60 redraws per second
        // indefinitely.
        if (!done && !d3d.swapChainOccluded && GetTickCount64() - lastInteractionTick > 1000) {
            const DWORD timeout = ImGui::GetIO().WantTextInput ? 200 : INFINITE;
            MsgWaitForMultipleObjectsEx(0, nullptr, timeout, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
        }
    }

    return 0;
}

void App::ReleaseRenderer() {
    if (ImGui::GetIO().BackendRendererUserData) ImGui_ImplDX11_Shutdown();
    CleanupDeviceD3D(d3d);
}

bool App::HandleGraphicsError(HRESULT result) {
    if (result != DXGI_ERROR_DEVICE_REMOVED && result != DXGI_ERROR_DEVICE_RESET &&
        SUCCEEDED(d3d.device->GetDeviceRemovedReason())) return false;
    ReleaseRenderer();
    return true;
}

void App::Shutdown() {
    RemoveTrayIcon();
    ReleaseRenderer();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    if (hwnd) {
        DestroyWindow(hwnd);
        hwnd = nullptr;
    }
    UnregisterClassW(L"NameExchangerClass", GetModuleHandleW(nullptr));
    if (comInitialized) {
        CoUninitialize();
        comInitialized = false;
    }
}

void App::DrawPathInput(size_t index, const char* childId, float inputWidth) {
    const float height = 28.0f * dpiScale;
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f * dpiScale, (height - ImGui::GetFontSize()) * 0.5f));
    ImGui::BeginChild(childId, ImVec2(inputWidth, height + ImGui::GetStyle().ScrollbarSize), false,
                      ImGuiWindowFlags_HorizontalScrollbar);
    if (pathWidths[index] < 0.0f) pathWidths[index] = ImGui::CalcTextSize(paths[index].c_str()).x;
    ImGui::SetNextItemWidth((std::max)(inputWidth, pathWidths[index] + 24.0f * dpiScale));
    ImGui::PushID(static_cast<int>(pathRevision));
    if (ImGui::InputText("##path", &paths[index])) pathWidths[index] = -1.0f;
    ImGui::PopID();
    ImGui::EndChild();
    ImGui::PopStyleVar(2);
}

void App::RenderUI() {
    if (pendingPaths) {
        paths = std::move(*pendingPaths);
        pendingPaths.reset();
        ++pathRevision;
        pathWidths.fill(-1.0f);
    }
    const auto& L = GetCurrentLocale();
    const float s = dpiScale;

    const ImVec2 display = ImGui::GetIO().DisplaySize;
    const float winW = display.x;
    const float winH = display.y;
    float barH = 32 * s;
    float btnSize = 29 * s;
    float btnY = (barH - btnSize) / 2.0f;
    float contentX = 11 * s;
    float inputWidth = winW - contentX * 2;
    const float each_width = 6 * s + btnSize;

    // Clear input focus when window loses foreground
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

    ImGui::PushStyleColor(ImGuiCol_ChildBg, topBarBgColor);
    ImGui::BeginChild("TopBar", ImVec2(winW, barH), false);
    ImGui::PopStyleColor();

    if (fontIcon) {
        ImGui::PushFont(fontIcon);
    }

    // Transparent button background
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, topBarButtonHoveredColor);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, topBarButtonActiveColor);

    // Pin toggle button - yellow text
    ImGui::SetCursorPos(ImVec2(6 * s, btnY));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
    if (ImGui::Button(isTopmost ? "B" : "A", ImVec2(btnSize, btnSize))) {
        isTopmost = !isTopmost;
        SetWindowPos(hwnd, isTopmost ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }
    if (ImGui::IsItemHovered()) {
        ImGui::PushStyleColor(ImGuiCol_PopupBg, tooltipBgColor);
        ImGui::BeginTooltip();
        if (fontLabel) ImGui::PushFont(fontLabel);
        ImGui::PushStyleColor(ImGuiCol_Text, tooltipTextColor);
        ImGui::TextUnformatted(L.pinTooltip);
        ImGui::PopStyleColor();
        if (fontLabel) ImGui::PopFont();
        ImGui::EndTooltip();
        ImGui::PopStyleColor();
    }
    ImGui::PopStyleColor();

    // About button - red text
    ImGui::SetCursorPos(ImVec2(winW - each_width * 5, btnY));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
    if (ImGui::Button("C", ImVec2(btnSize, btnSize))) {
        MessageBoxW(hwnd, L.aboutMessageW, L.aboutTooltip, MB_OK);
    }
    ImGui::PopStyleColor();

    // Remaining 4 buttons - white text
    ImGui::PushStyleColor(ImGuiCol_Text, topBarTextColor);

    // Admin button
    ImGui::SetCursorPos(ImVec2(winW - each_width * 4, btnY));
    if (ImGui::Button(isAdmin ? "E" : "D", ImVec2(btnSize, btnSize))) {
        if (RunAsAdmin(!isAdmin)) {
            done = true;
        } else {
            MessageBoxW(hwnd,
                        isAdmin ? L"Failed to start a standard-user instance." : L"Elevation was cancelled or failed.",
                        L.errorTitle, MB_OK | MB_ICONERROR);
        }
    }
    if (ImGui::IsItemHovered()) {
        ImGui::PushStyleColor(ImGuiCol_PopupBg, tooltipBgColor);
        ImGui::BeginTooltip();
        if (fontLabel) ImGui::PushFont(fontLabel);
        ImGui::PushStyleColor(ImGuiCol_Text, tooltipTextColor);
        ImGui::TextUnformatted(L.adminTooltip);
        ImGui::PopStyleColor();
        if (fontLabel) ImGui::PopFont();
        ImGui::EndTooltip();
        ImGui::PopStyleColor();
    }

    // SendTo shortcut button
    ImGui::SetCursorPos(ImVec2(winW - each_width * 3, btnY));
    if (ImGui::Button("F", ImVec2(btnSize, btnSize))) {
        CreateSendToShortcut(false);
    }
    if (ImGui::IsItemHovered()) {
        ImGui::PushStyleColor(ImGuiCol_PopupBg, tooltipBgColor);
        ImGui::BeginTooltip();
        if (fontLabel) ImGui::PushFont(fontLabel);
        ImGui::PushStyleColor(ImGuiCol_Text, tooltipTextColor);
        ImGui::TextUnformatted(L.sendToTooltip);
        ImGui::PopStyleColor();
        if (fontLabel) ImGui::PopFont();
        ImGui::EndTooltip();
        ImGui::PopStyleColor();
    }
    if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
        CreateSendToShortcut(true);
    }

    // Minimize button
    ImGui::SetCursorPos(ImVec2(winW - each_width * 2, btnY));
    ImGui::BeginDisabled(!IsTrayIconAvailable());
    if (ImGui::Button("G", ImVec2(btnSize, btnSize))) {
        ShowWindow(hwnd, SW_HIDE);
    }
    ImGui::EndDisabled();

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
        ImGui::GetIO().AddMouseButtonEvent(ImGuiMouseButton_Left, false);
        PostMessageW(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, static_cast<LPARAM>(GetMessagePos()));
    }

    ImGui::EndChild();

    // === Main Content ===

    // Label 1
    if (fontLabel) ImGui::PushFont(fontLabel);
    ImGui::SetCursorPos(ImVec2(contentX, 42 * s));
    ImGui::Text("%s", L.file1Label);
    ImGui::SetCursorPos(ImVec2(contentX, 62 * s));
    DrawPathInput(0, "##path1_scroll", inputWidth);

    // Label 2
    ImGui::SetCursorPos(ImVec2(contentX, 104 * s));
    ImGui::Text("%s", L.file2Label);
    ImGui::SetCursorPos(ImVec2(contentX, 124 * s));
    DrawPathInput(1, "##path2_scroll", inputWidth);

    const float optionY = 166.0f * s;
    const float startBtnY = winH - 48.0f * s;

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f * s, 1.0f * s));
    const float optionGap = 40.0f * s;
    if (optionWidth <= 0.0f) {
        optionWidth = ImGui::CalcTextSize(L.preserveExtLabel).x + ImGui::GetFrameHeight() + optionGap +
                      ImGui::CalcTextSize(L.swapFullNameLabel).x + ImGui::GetFrameHeight();
    }
    ImGui::SetCursorPos(ImVec2((std::max)(contentX, (winW - optionWidth) * 0.5f), optionY));
    if (ImGui::RadioButton(L.preserveExtLabel, preserveExt)) preserveExt = true;
    ImGui::SameLine(0.0f, optionGap);
    if (ImGui::RadioButton(L.swapFullNameLabel, !preserveExt)) preserveExt = false;
    ImGui::PopStyleVar();
    if (fontLabel) ImGui::PopFont();

    // Exchange button
    ImGui::PushFont(fontLabel, 24.0f);
    float btnW = 124 * s;
    float btnH2 = 44 * s;
    ImGui::SetCursorPos(ImVec2((winW - btnW) / 2.0f, startBtnY));
    if (ImGui::Button(L.startButton, ImVec2(btnW, btnH2))) {
        const int returnId = ExchangePaths(paths[0], paths[1], preserveExt);
        if (returnId == 0) {
            paths = {};
            pathWidths.fill(-1.0f);
            ++pathRevision;
        } else {
            const char* info = GetOutputInfo(returnId);
            std::wstring winfo = Utf8ToUtf16(info);
            MessageBoxW(hwnd, winfo.c_str(), L.errorTitle, MB_OK | MB_ICONERROR);
        }
    }
    ImGui::PopFont();

    ImGui::End();
    ImGui::PopStyleVar();  // WindowPadding
}

void App::CreateSendToShortcut(bool remove) {
    const auto& L = GetCurrentLocale();
    PWSTR rawSendTo = nullptr;
    const HRESULT folderResult = SHGetKnownFolderPath(FOLDERID_SendTo, 0, nullptr, &rawSendTo);
    if (FAILED(folderResult) || !rawSendTo) {
        MessageBoxW(hwnd, L"Unable to locate the Send To folder.", L.errorTitle, MB_OK | MB_ICONERROR);
        return;
    }
    const std::filesystem::path shortcut = std::filesystem::path(rawSendTo) / L"name_exchanger.lnk";
    CoTaskMemFree(rawSendTo);

    if (remove) {
        if (!DeleteFileW(shortcut.c_str()) && GetLastError() != ERROR_FILE_NOT_FOUND) {
            MessageBoxW(hwnd, L"Unable to remove the Send To shortcut.", L.errorTitle, MB_OK | MB_ICONERROR);
            return;
        }
        MessageBoxW(hwnd, L.shortcutRemoved, L.tipsTitle, MB_OK);
        return;
    }

    const std::wstring executable = GetExecutablePath();
    if (executable.empty()) {
        MessageBoxW(hwnd, L"Unable to determine the executable path.", L.errorTitle, MB_OK | MB_ICONERROR);
        return;
    }

    IShellLinkW* shellLink = nullptr;
    HRESULT result = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&shellLink));
    if (SUCCEEDED(result)) result = shellLink->SetPath(executable.c_str());
    if (SUCCEEDED(result)) result = shellLink->SetDescription(L"FilenameExchanger");
    if (SUCCEEDED(result)) result = shellLink->SetIconLocation(executable.c_str(), 0);

    IPersistFile* persist = nullptr;
    if (SUCCEEDED(result)) result = shellLink->QueryInterface(IID_PPV_ARGS(&persist));
    if (SUCCEEDED(result)) result = persist->Save(shortcut.c_str(), TRUE);
    if (persist) persist->Release();
    if (shellLink) shellLink->Release();

    if (FAILED(result)) {
        MessageBoxW(hwnd, L"Unable to create the Send To shortcut.", L.errorTitle, MB_OK | MB_ICONERROR);
        return;
    }
    MessageBoxW(hwnd, L.shortcutCreated, L.tipsTitle, MB_OK);
}

LRESULT App::HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (taskbarCreatedMessage && msg == taskbarCreatedMessage) {
        if (!SetupTrayIcon(hWnd) && (!IsWindowVisible(hWnd) || IsIconic(hWnd))) ShowWindow(hWnd, SW_RESTORE);
        InvalidateRect(hWnd, nullptr, FALSE);
        return 0;
    }
    switch (msg) {
        case WM_SHOWWINDOW:
            if (wParam) {
                lastInteractionTick = GetTickCount64();
                InvalidateRect(hWnd, nullptr, FALSE);
            }
            break;

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

        case WM_THEMECHANGED:
        case WM_SETTINGCHANGE:
            if (ImGui::GetCurrentContext()) {
                ApplySystemTheme();
            }
            return 0;

        case WM_DPICHANGED: {
            pendingDpi = LOWORD(wParam);
            const auto* suggested = reinterpret_cast<const RECT*>(lParam);
            if (suggested) {
                SetWindowPos(hWnd, nullptr, suggested->left, suggested->top, suggested->right - suggested->left,
                             suggested->bottom - suggested->top, SWP_NOZORDER | SWP_NOACTIVATE);
            }
            return 0;
        }

        case WM_DROPFILES: {
            const auto drop = reinterpret_cast<HDROP>(wParam);
            if (!drop) return 0;
            const UINT count = DragQueryFileW(drop, 0xFFFFFFFF, nullptr, 0);
            auto first = count ? ReadDroppedPath(drop, 0) : std::string{};
            auto second = count >= 2 ? ReadDroppedPath(drop, 1) : std::string{};
            DragFinish(drop);
            if (first.empty() || (count >= 2 && second.empty())) return 0;

            if (count >= 2) {
                pendingPaths = std::array{std::move(first), std::move(second)};
            } else {
                if (!pendingPaths) pendingPaths = paths;
                auto& [left, right] = *pendingPaths;
                if (left.empty()) {
                    left = std::move(first);
                } else if (right.empty()) {
                    right = std::move(first);
                } else {
                    left = std::move(first);
                    right.clear();
                }
            }
            InvalidateRect(hWnd, nullptr, FALSE);
            return 0;
        }

        case WM_TRAYICON: {
            // NOTIFYICON_VERSION_4 places the event in LOWORD(lParam).
            switch (LOWORD(lParam)) {
                case WM_LBUTTONUP:
                case NIN_KEYSELECT:
                    if (!IsWindowVisible(hWnd) || IsIconic(hWnd)) {
                        ShowWindow(hWnd, SW_RESTORE);
                        SetForegroundWindow(hWnd);
                    } else if (IsTrayIconAvailable()) {
                        ShowWindow(hWnd, SW_HIDE);
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
