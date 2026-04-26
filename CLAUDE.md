# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build

```
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja -C build
```

Requires MSVC (GNU/MinGW is explicitly rejected). ImGui is fetched automatically via CMake FetchContent at configure time (requires network access). The CMake config auto-detects x64/x86 for linking the correct pre-built lib. The output is `build/name_exchanger_x64.exe` (or `_x86`).

To update Dear ImGui, change the `GIT_TAG` in `CMakeLists.txt` (e.g. `v1.91.8` → `v1.92.0`).

## Project overview

Windows desktop app (C++20, ImGui, DirectX 11) that swaps the names of two files or directories. A no-GUI command-line mode is also supported (`name_exchanger <path1> <path2> [preserve]`).

The actual file/directory rename logic lives in a pre-compiled Rust library at `lib/name_exchanger_x64.lib` and `lib/name_exchanger_x86.lib`, exposed via:

```cpp
extern "C" int exchange(const char* path1, const char* path2, bool preserve_ext);
// return 0 = success, 1 = not found, 2 = permission denied, 3 = target exists, 4 = same file, 5 = invalid path
```

## Architecture

- **`main.cpp`** — `WinMain` entry point. Enforces single-instance via a named mutex (`CFFD3CF9A003453C9893A8CD49EF7ED5`). If a second instance starts, it finds and activates the existing window instead.

- **`src/app.h` / `src/app.cpp`** — The entire application: window creation, D3D11/ImGui init, the main loop, and the full UI rendering in `RenderUI()`. Uses a `PopTooltip()` / `PopFont()` pattern throughout — each UI element is responsible for popping whatever style/font/color it pushes. Top bar uses custom icon-font "letters" (`A`–`H`) rendered via `kIconFontData` from `font_data.h`; each maps to a toolbar button glyph.

- **`src/utils.h` / `src/utils.cpp`** — `Utf16ToUtf8`/`Utf8ToUtf16` converters, `IsRunAsAdmin()`, and `RunAsAdmin()`. `RunAsAdmin` handles privilege escalation via `ShellExecuteEx(runas)` and de-escalation by duplicating explorer's token.

- **`src/i18n.h` / `src/i18n.cpp`** — Three-language i18n (Simplified Chinese, Traditional Chinese, English). `DetectSystemLanguage()` uses `GetUserDefaultUILanguage()`. All UI strings live in `LocaleStrings` structs; `GetCurrentLocale()` caches detection result.

- **`src/d3d_helpers.h` / `src/d3d_helpers.cpp`** — Thin helpers for D3D11 device/swapchain/render-target lifecycle. Tries hardware device first, falls back to WARP software rasterizer.

- **`src/tray.h` / `src/tray.cpp`** — System tray icon setup/removal via `Shell_NotifyIconW`. Tray messages arrive as `WM_USER + 1` and are handled in `HandleMessage`.

- **`src/font_data.h`** — Auto-generated binary blob from a custom TTF icon font. The `kIconFontData` array is loaded as an ImGui memory font at 15pt.

- **`res/`** — Windows resource files: icon, version info (currently `3.1.1`), and `version.h`.

## UI behavior notes

- The window is borderless (`WS_POPUP`), with a custom top bar painted in the Windows accent color. Empty area of the top bar is draggable (simulates `WM_NCLBUTTONDOWN` on `HTCAPTION`).
- DPI changes trigger font rebuild (`WM_DPICHANGED` handler in `HandleMessage`).
- Theme changes (`WM_THEMECHANGED`, `WM_SETTINGCHANGE`) cause `ApplySystemTheme()` to re-read system colors and Windows dark-mode setting.
- Input focus is cleared when the window loses foreground to avoid stale ImGui active IDs.
- All font face names are resolved against `c:\Windows\Fonts\msyh.ttc` (Chinese) with a fallback to ImGui default.

## Strings and encoding

All internal path data is UTF-8 (`std::string`). The Win32 layer and ImGui's `InputText` need UTF-8, so paths are converted at the boundary via `Utf16ToUtf8`/`Utf8ToUtf16`. Console output (for CLI mode errors) writes UTF-8 to stderr.
