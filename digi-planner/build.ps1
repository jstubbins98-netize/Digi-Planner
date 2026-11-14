param(
    [string]$QtPath,
    [switch]$Launch
)
$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

if ([Environment]::OSVersion.Platform -ne [PlatformID]::Win32NT -or
    [Environment]::OSVersion.Version.Build -lt 22000) {
    throw "This script requires Windows 11."
}
. (Join-Path $PSScriptRoot "Find-Qt.ps1")
$qt = Find-PlannerQt -ExplicitPath $QtPath
$QtPath = $qt.Path
Write-Host "Using Qt $($qt.Version) at $QtPath"
if (!(Get-Command cmake -ErrorAction SilentlyContinue)) {
    throw "Install CMake 4.2+ and add it to PATH, then open a new PowerShell window."
}

function Invoke-Checked {
    param([string]$Executable, [string[]]$Arguments)
    & $Executable @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Executable failed with exit code $LASTEXITCODE." }
}

$capabilitiesJson = & cmake -E capabilities
if ($LASTEXITCODE -ne 0) { throw "Could not query CMake capabilities." }
$capabilities = ($capabilitiesJson -join "`n") | ConvertFrom-Json
if ([version]("$($capabilities.version.major).$($capabilities.version.minor)") -lt [version]"4.2") {
    throw "Visual Studio 2026 requires CMake 4.2 or newer."
}
$vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
if (!(Test-Path $vswhere)) { throw "Install Visual Studio 2026 or newer with Desktop development with C++." }
$instancesJson = & $vswhere -products '*' -version '[18.0,)' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -format json
if ($LASTEXITCODE -ne 0) { throw "Visual Studio detection failed." }
$instances = @(($instancesJson -join "`n") | ConvertFrom-Json |
    Sort-Object { [version]$_.installationVersion } -Descending)
if ($instances.Count -eq 0) {
    throw "No Visual Studio 2026+ installation with MSVC x64 tools was found. Install the C++ workload."
}
$instance = $instances[0]
$major = ([version]$instance.installationVersion).Major
$generator = @($capabilities.generators | Where-Object { $_.name -match "^Visual Studio $major " })
if ($generator.Count -eq 0) {
    throw "This CMake version does not support installed Visual Studio $major. Upgrade CMake."
}
$generatorName = $generator[0].name
Write-Host "Using $generatorName at $($instance.installationPath)"
# Keep old Visual Studio caches intact and separate from the new generator.
$build = Join-Path $PSScriptRoot "build\vs$major-x64"
$dist = Join-Path $PSScriptRoot "dist"
Invoke-Checked "cmake" @("--fresh", "-S", $PSScriptRoot, "-B", $build, "-G", $generatorName,
    "-A", "x64", "-DCMAKE_GENERATOR_INSTANCE=$($instance.installationPath)", "-DCMAKE_PREFIX_PATH=$QtPath")
Invoke-Checked "cmake" @("--build", $build, "--config", "Release", "--parallel")
Invoke-Checked "cmake" @("--install", $build, "--config", "Release", "--prefix", $dist)
if (!(Test-Path "$dist\sqldrivers\qsqlite.dll")) {
    throw "The SQLite driver was not packaged. Check your Qt installation."
}
Write-Host "`nBuild finished. Run: $dist\DigiPlanner.exe"
Write-Host "When sharing the program, share the entire dist folder, not just the EXE."
if ($Launch) { Start-Process (Join-Path $dist "DigiPlanner.exe") }
