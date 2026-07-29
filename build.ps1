[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release', 'RelWithDebInfo', 'MinSizeRel')]
    [string]$Configuration = 'Release',

    [switch]$Clean,

    [Parameter(DontShow)]
    [ValidateSet('', 'x86', 'x64')]
    [string]$Architecture = ''
)

$ErrorActionPreference = 'Stop'
[System.Console]::OutputEncoding = [System.Console]::InputEncoding = [System.Text.Encoding]::UTF8

$projectRoot = $PSScriptRoot

function Get-VisualStudioPath {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (-not (Test-Path -LiteralPath $vswhere -PathType Leaf)) {
        throw '未找到 vswhere.exe。请安装 Visual Studio 2022/Build Tools 及“使用 C++ 的桌面开发”工作负载。'
    }

    $installationPath = & $vswhere -latest -products * `
        -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
        -property installationPath
    if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($installationPath)) {
        throw '未找到包含 MSVC x86/x64 工具链的 Visual Studio。'
    }
    return $installationPath.Trim()
}

function Invoke-ArchitectureBuild {
    param(
        [Parameter(Mandatory)]
        [ValidateSet('x86', 'x64')]
        [string]$TargetArchitecture
    )

    $visualStudioPath = Get-VisualStudioPath
    $devShellModule = Join-Path $visualStudioPath 'Common7\Tools\Microsoft.VisualStudio.DevShell.dll'
    if (-not (Test-Path -LiteralPath $devShellModule -PathType Leaf)) {
        throw "未找到 Visual Studio Developer Shell 模块：$devShellModule"
    }

    Import-Module $devShellModule
    $devArguments = if ($TargetArchitecture -eq 'x64') {
        '-arch=x64 -host_arch=x64'
    }
    else {
        '-arch=x86 -host_arch=x64'
    }
    Enter-VsDevShell -VsInstallPath $visualStudioPath -SkipAutomaticLocation `
        -DevCmdArguments $devArguments | Out-Null

    foreach ($tool in @('cmake', 'ninja', 'cl')) {
        if (-not (Get-Command $tool -ErrorAction SilentlyContinue)) {
            throw "未找到构建工具：$tool"
        }
    }

    $buildDirectory = Join-Path $projectRoot "build-$TargetArchitecture"
    if ($Clean -and (Test-Path -LiteralPath $buildDirectory)) {
        Write-Host "清理 $buildDirectory" -ForegroundColor Yellow
        Remove-Item -LiteralPath $buildDirectory -Recurse -Force
    }

    # 避免不记录文件所有权的磁盘导致 FetchContent 中的 Git 拒绝访问。
    $previousGitConfigCount = $env:GIT_CONFIG_COUNT
    $previousGitConfigKey = $env:GIT_CONFIG_KEY_0
    $previousGitConfigValue = $env:GIT_CONFIG_VALUE_0
    try {
        $env:GIT_CONFIG_COUNT = '1'
        $env:GIT_CONFIG_KEY_0 = 'safe.directory'
        $env:GIT_CONFIG_VALUE_0 = '*'

        Write-Host "[$TargetArchitecture] 配置 $Configuration" -ForegroundColor Cyan
        & cmake -S $projectRoot -B $buildDirectory -G Ninja `
            "-DCMAKE_BUILD_TYPE=$Configuration"
        if ($LASTEXITCODE -ne 0) {
            throw "[$TargetArchitecture] CMake 配置失败，退出码：$LASTEXITCODE"
        }

        Write-Host "[$TargetArchitecture] 编译" -ForegroundColor Cyan
        & cmake --build $buildDirectory --config $Configuration --parallel
        if ($LASTEXITCODE -ne 0) {
            throw "[$TargetArchitecture] 编译失败，退出码：$LASTEXITCODE"
        }
    }
    finally {
        $env:GIT_CONFIG_COUNT = $previousGitConfigCount
        $env:GIT_CONFIG_KEY_0 = $previousGitConfigKey
        $env:GIT_CONFIG_VALUE_0 = $previousGitConfigValue
    }

    $suffix = if ($TargetArchitecture -eq 'x64') { 'x64' } else { 'x86' }
    $executable = Join-Path $buildDirectory "name_exchanger_$suffix.exe"
    if (-not (Test-Path -LiteralPath $executable -PathType Leaf)) {
        throw "编译结束，但未找到输出文件：$executable"
    }

    Write-Host "[$TargetArchitecture] 完成：$executable" -ForegroundColor Green
}

if ($Architecture) {
    Invoke-ArchitectureBuild -TargetArchitecture $Architecture
    exit 0
}

# 每个架构使用独立 pwsh 进程，避免同一进程内切换 MSVC 环境造成变量污染。
foreach ($target in @('x86', 'x64')) {
    $arguments = @(
        '-NoLogo',
        '-NoProfile',
        '-ExecutionPolicy', 'Bypass',
        '-File', $PSCommandPath,
        '-Configuration', $Configuration,
        '-Architecture', $target
    )
    if ($Clean) { $arguments += '-Clean' }

    & pwsh @arguments
    if ($LASTEXITCODE -ne 0) {
        throw "[$target] 子构建进程失败，退出码：$LASTEXITCODE"
    }
}

Write-Host ''
Write-Host 'x86 与 x64 均已编译完成。' -ForegroundColor Green
