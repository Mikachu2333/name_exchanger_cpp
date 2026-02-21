#include "tray.h"

#include "i18n.h"

#include <shellapi.h>

static NOTIFYICONDATAW g_nid = {};

void SetupTrayIcon(HWND hwnd) {
    g_nid.cbSize = sizeof(NOTIFYICONDATAW);
    g_nid.hWnd = hwnd;
    g_nid.uID = 1;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_USER + 1;

    // Load the application's own icon from resources (IDI_ICON1 = 101)
    g_nid.hIcon = static_cast<HICON>(
        LoadImageW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(101), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED));
    if (!g_nid.hIcon) {
        g_nid.hIcon = LoadIconW(nullptr, MAKEINTRESOURCEW(32512)); // IDI_APPLICATION
    }

    const auto &locale = GetCurrentLocale();
    wcsncpy_s(g_nid.szTip, locale.trayTooltip, _TRUNCATE);
    Shell_NotifyIconW(NIM_ADD, &g_nid);
}

void RemoveTrayIcon() {
    Shell_NotifyIconW(NIM_DELETE, &g_nid);
}
