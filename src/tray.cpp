#include "tray.hpp"

#include "i18n.hpp"

#include <shellapi.h>

namespace {
NOTIFYICONDATAW g_notification{};
bool g_added = false;
}

bool SetupTrayIcon(HWND hwnd) {
    g_notification = {};
    g_notification.cbSize = sizeof(g_notification);
    g_notification.hWnd = hwnd;
    g_notification.uID = 1;
    g_notification.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_notification.uCallbackMessage = WM_TRAYICON;
    g_notification.hIcon = static_cast<HICON>(
        LoadImageW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(101), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED));
    if (!g_notification.hIcon) g_notification.hIcon = LoadIconW(nullptr, IDI_APPLICATION);

    wcsncpy_s(g_notification.szTip, GetCurrentLocale().trayTooltip, _TRUNCATE);
    g_added = Shell_NotifyIconW(NIM_ADD, &g_notification) != FALSE;
    if (g_added) {
        g_notification.uVersion = NOTIFYICON_VERSION_4;
        Shell_NotifyIconW(NIM_SETVERSION, &g_notification);
    }
    return g_added;
}

void RemoveTrayIcon() {
    if (g_added) {
        Shell_NotifyIconW(NIM_DELETE, &g_notification);
        g_added = false;
    }
}
