# name_exchanger (C++)

Language: [简体中文 / 繁體中文](./README.md) | **English**

A lightweight Windows utility to swap names between two files or directories (C++ + ImGui).

## Features

- Supports dragging and dropping 1 or 2 files/folders, as well as manual path input.
- Two swap modes:
  - Preserve extensions (swaps base names only)
  - Swap full names (including extensions)
- System tray support: left-click to show/hide the window, right-click to exit.
- Always-on-top toggle.
- Create/remove "Send To" shortcut (left-click to create, right-click to remove).
- Administrator mode toggle.

## Command Line Usage

```text
name_exchanger <path1> <path2> [preserve]
```

- `preserve` is an optional parameter, defaulting to `true` (preserves extensions). Can be set to `false` (swaps full file names).

## Other Notes

- Supports Windows 10 version 1607 and later.
- Supports local, network, and virtual paths when both the library and underlying filesystem permit renaming.
- Each rename step is atomic, but the complete three-step exchange is not a crash-safe transaction.
- Ordinary failures trigger a rollback attempt. If both the operation and rollback fail, the app issues a critical warning requiring manual inspection.
- Both entries and the temporary directory must be within a filesystem scope that permits the required renames.

## Screenshot

![screenshot](./en.png)

## Related Library

- [exchange_name_lib](https://github.com/Mikachu2333/exchange_name_lib)
