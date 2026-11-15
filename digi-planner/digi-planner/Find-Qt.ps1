# Discovery only: never download Qt or execute binaries found during the search.
function Get-PlannerQtKit {
    param([string]$Path)
    if (!(Test-Path -LiteralPath $Path -PathType Container)) { throw "Folder does not exist." }
    $kit = (Resolve-Path -LiteralPath $Path).Path
    $required = @(
        "mkspecs\qconfig.pri", "lib\Qt6Core.lib", "bin\Qt6Core.dll",
        "bin\Qt6Gui.dll", "bin\Qt6Widgets.dll", "bin\Qt6Sql.dll",
        "bin\windeployqt.exe", "plugins\sqldrivers\qsqlite.dll",
        "plugins\platforms\qwindows.dll"
    )
    foreach ($module in @("Qt6", "Qt6Core", "Qt6Gui", "Qt6Widgets", "Qt6Sql")) {
        $required += "lib\cmake\$module\${module}Config.cmake"
    }
    $missing = @($required | Where-Object {
        !(Test-Path -LiteralPath (Join-Path $kit $_) -PathType Leaf)
    })
    if ($missing.Count -gt 0) {
        throw ("Missing required files: " + ($missing -join ", ") +
            ". Use Qt Maintenance Tool to install the MSVC x64 desktop kit (Qt Base includes Widgets and SQL).")
    }
    $config = Get-Content -LiteralPath (Join-Path $kit "mkspecs\qconfig.pri") -Raw
    if ($config -notmatch '(?m)^\s*QT_VERSION\s*=\s*(\d+\.\d+\.\d+)\s*$') {
        throw "Cannot read QT_VERSION from mkspecs\qconfig.pri."
    }
    $version = [version]$Matches[1]
    if ($version.Major -ne 6 -or $version -lt [version]"6.5.0") { throw "Qt $version is unsupported; Qt 6.5 or newer within Qt 6 is required." }
    # Read the PE machine field instead of trusting the folder name.
    $stream = $null
    $reader = $null
    try {
        $stream = [System.IO.File]::OpenRead((Join-Path $kit "bin\Qt6Core.dll"))
        $reader = New-Object System.IO.BinaryReader($stream)
        if ($reader.ReadUInt16() -ne 0x5A4D) { throw "Qt6Core.dll is not a Windows DLL." }
        $stream.Position = 0x3C
        $peOffset = $reader.ReadInt32()
        if ($peOffset -lt 0 -or $peOffset -gt ($stream.Length - 6)) { throw "Qt6Core.dll has an invalid PE header." }
        $stream.Position = $peOffset
        if ($reader.ReadUInt32() -ne 0x00004550) { throw "Qt6Core.dll has an invalid PE signature." }
        $machine = $reader.ReadUInt16()
        if ($machine -ne 0x8664) { throw ("Qt6Core.dll is not x64 (PE machine 0x{0:X4}); select the MSVC x64 kit, not ARM or 32-bit." -f $machine) }
    } finally {
        if ($null -ne $reader) { $reader.Dispose() }
        elseif ($null -ne $stream) { $stream.Dispose() }
    }
    [pscustomobject]@{ Path = $kit; Version = $version }
}

function Find-PlannerQt {
    param([string]$ExplicitPath)
    Write-Host "Searching for Qt 6.5+ MSVC x64 installations..."
    $roots = New-Object 'System.Collections.Generic.List[string]'
    $explicit = ![string]::IsNullOrWhiteSpace($ExplicitPath)
    if ($explicit) {
        $path = [Environment]::ExpandEnvironmentVariables($ExplicitPath.Trim().Trim('"'))
        $path = $path -replace '[\\/]lib[\\/]cmake[\\/]Qt6[\\/]?$', ''
        $path = $path -replace '[\\/]bin[\\/]?$', ''
        if (!(Test-Path -LiteralPath $path -PathType Container)) {
            throw "Qt folder '$path' does not exist. Supply the actual installed location, not the example path."
        }
        $roots.Add($path)
    } else {
    foreach ($variable in @("QTDIR", "QT_ROOT_DIR", "Qt6_DIR", "CMAKE_PREFIX_PATH")) {
        $value = [Environment]::GetEnvironmentVariable($variable)
        foreach ($entry in ($value -split ';')) {
            if (![string]::IsNullOrWhiteSpace($entry)) {
                $path = [Environment]::ExpandEnvironmentVariables($entry.Trim().Trim('"'))
                # CMake hints can point to the config directory rather than the kit.
                if ($path -match '[\\/]lib[\\/]cmake[\\/]Qt6[\\/]?$') {
                    $path = $path -replace '[\\/]lib[\\/]cmake[\\/]Qt6[\\/]?$', ''
                }
                $roots.Add($path)
            }
        }
    }
    foreach ($entry in ($env:PATH -split ';')) {
        if (![string]::IsNullOrWhiteSpace($entry)) {
            $bin = [Environment]::ExpandEnvironmentVariables($entry.Trim().Trim('"'))
            if ((Test-Path -LiteralPath (Join-Path $bin "qmake.exe") -PathType Leaf) -or
                (Test-Path -LiteralPath (Join-Path $bin "qmake6.exe") -PathType Leaf)) {
                $roots.Add((Split-Path $bin -Parent))
            }
        }
    }
    # Bounded searches: drive:\Qt, Program Files\Qt, and per-user Qt installs.
    foreach ($drive in [System.IO.DriveInfo]::GetDrives()) {
        if ($drive.IsReady -and $drive.DriveType -eq [System.IO.DriveType]::Fixed) {
            $roots.Add((Join-Path $drive.RootDirectory.FullName "Qt"))
        }
    }
    foreach ($parent in @($env:ProgramFiles, ${env:ProgramFiles(x86)}, $env:USERPROFILE, $env:LOCALAPPDATA)) {
        if (![string]::IsNullOrWhiteSpace($parent)) { $roots.Add((Join-Path $parent "Qt")) }
    }
    # Qt Maintenance Tool entries also identify nonstandard installation roots.
    foreach ($key in @(
        "HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\*",
        "HKLM:\Software\Microsoft\Windows\CurrentVersion\Uninstall\*",
        "HKLM:\Software\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall\*"
    )) {
        foreach ($entry in @(Get-ItemProperty -Path $key -ErrorAction SilentlyContinue)) {
            if ($entry.PSObject.Properties["DisplayName"] -and
                $entry.PSObject.Properties["InstallLocation"] -and
                $entry.DisplayName -match '^Qt(?:\s|$)' -and
                ![string]::IsNullOrWhiteSpace($entry.InstallLocation)) {
                $roots.Add($entry.InstallLocation)
            }
        }
    }
    }

    $seen = New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::OrdinalIgnoreCase)
    $rejections = New-Object 'System.Collections.Generic.List[string]'
    $kits = @(
        foreach ($root in ($roots | Sort-Object -Unique)) {
            if (!(Test-Path -LiteralPath $root -PathType Container)) { continue }
            # Accept a kit, a version directory, or an installer root (two levels).
            $candidates = @($root)
            $children = @(Get-ChildItem -LiteralPath $root -Directory -ErrorAction SilentlyContinue |
                Where-Object { !($_.Attributes -band [IO.FileAttributes]::ReparsePoint) })
            $candidates += @($children | ForEach-Object { $_.FullName })
            foreach ($child in $children) {
                $candidates += @(Get-ChildItem -LiteralPath $child.FullName -Directory -ErrorAction SilentlyContinue |
                    Where-Object { !($_.Attributes -band [IO.FileAttributes]::ReparsePoint) } |
                    ForEach-Object { $_.FullName })
            }
            foreach ($candidate in $candidates) {
                if (!$seen.Add($candidate)) { continue }
                # Do not report ordinary version/container folders as broken kits.
                if (!(Test-Path -LiteralPath (Join-Path $candidate "mkspecs\qconfig.pri")) -and
                    !(Test-Path -LiteralPath (Join-Path $candidate "bin\Qt6Core.dll")) -and
                    !(Test-Path -LiteralPath (Join-Path $candidate "lib\cmake\Qt6")) -and
                    (Split-Path $candidate -Leaf) -notmatch '^(msvc|mingw)') { continue }
                try { Get-PlannerQtKit -Path $candidate }
                catch { $rejections.Add("$candidate`n  $($_.Exception.Message)") }
            }
        }
    )
    if ($kits.Count -eq 0) {
        $details = if ($rejections.Count) {
            "Qt candidates were found but rejected:`n" + (($rejections | Select-Object -First 20) -join "`n")
        } else {
            "No Qt desktop kit was found in the searched folders. Qt Creator or Visual Studio alone does not include the required Qt libraries."
        }
        throw ("No suitable Qt installation found.`n$details`nSearched roots (up to two levels below each):`n" +
            (($roots | Sort-Object -Unique) -join "`n") +
            "`nInstall a Qt 6.5+ MSVC x64 desktop kit using Qt's installer/Maintenance Tool." +
            "`nIf Qt is installed elsewhere, run .\build.ps1 -QtPath '<your actual Qt folder>' -DiagnoseQt." +
            "`n-QtPath accepts a Qt installation root, version folder, or kit folder. An explicit location never falls back to unrelated installations.")
    }
    # Newest version first; path breaks ties deterministically.
    $kits = @($kits | Sort-Object @{ Expression = { $_.Version }; Descending = $true }, Path)
    if ($kits.Count -gt 1) {
        Write-Host "Found $($kits.Count) suitable kits; selecting the newest. Use -QtPath to override."
    }
    return $kits[0]
}
