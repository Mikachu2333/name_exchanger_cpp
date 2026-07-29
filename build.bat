@echo off
setlocal

where pwsh.exe >nul 2>&1
if errorlevel 1 (
    echo [ERROR] pwsh.exe was not found. Install PowerShell 7 or later.
    exit /b 1
)

pwsh.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0build.ps1" %*
set "EXIT_CODE=%ERRORLEVEL%"

if not "%EXIT_CODE%"=="0" (
    echo.
    echo [ERROR] Build failed with exit code %EXIT_CODE%.
)

exit /b %EXIT_CODE%
