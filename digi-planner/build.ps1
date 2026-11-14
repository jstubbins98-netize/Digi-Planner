param(
    [Parameter(Mandatory = $true)]
    [string]$QtPath,
    [switch]$Launch
)
$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

if ([Environment]::OSVersion.Platform -ne [PlatformID]::Win32NT -or
    [Environment]::OSVersion.Version.Build -lt 22000) {
    throw "This script requires Windows 11."
}
$QtPath = (Resolve-Path $QtPath).Path
if (!(Test-Path "$QtPath\lib\cmake\Qt6\Qt6Config.cmake")) {
    throw "QtPath must point to a Qt MSVC 2022 64-bit kit, for example C:\Qt\6.8.3\msvc2022_64."
}
if (!(Get-Command cmake -ErrorAction SilentlyContinue)) {
    throw "Install CMake 3.24+ and add it to PATH, then open a new PowerShell window."
}

function Invoke-Checked {
    param([string]$Executable, [string[]]$Arguments)
    & $Executable @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Executable failed with exit code $LASTEXITCODE." }
}

$build = Join-Path $PSScriptRoot "build"
$dist = Join-Path $PSScriptRoot "dist"
Invoke-Checked "cmake" @("-S", $PSScriptRoot, "-B", $build, "-G", "Visual Studio 17 2022",
    "-A", "x64", "-DCMAKE_PREFIX_PATH=$QtPath")
Invoke-Checked "cmake" @("--build", $build, "--config", "Release", "--parallel")
Invoke-Checked "cmake" @("--install", $build, "--config", "Release", "--prefix", $dist)
if (!(Test-Path "$dist\sqldrivers\qsqlite.dll")) {
    throw "The SQLite driver was not packaged. Check your Qt installation."
}
Write-Host "`nBuild finished. Run: $dist\DigiPlanner.exe"
Write-Host "When sharing the program, share the entire dist folder, not just the EXE."
if ($Launch) { Start-Process (Join-Path $dist "DigiPlanner.exe") }
