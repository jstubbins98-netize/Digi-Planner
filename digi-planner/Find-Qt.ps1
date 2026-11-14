# Discovery only: never download Qt or execute binaries found during the search.
function Get-PlannerQtKit {
    param([string]$Path)
    if (!(Test-Path -LiteralPath $Path -PathType Container)) { return }
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
    foreach ($file in $required) {
        if (!(Test-Path -LiteralPath (Join-Path $kit $file) -PathType Leaf)) { return }
    }
    $config = Get-Content -LiteralPath (Join-Path $kit "mkspecs\qconfig.pri") -Raw
    if ($config -notmatch '(?m)^\s*QT_VERSION\s*=\s*(6\.\d+\.\d+)\s*$') { return }
    $version = [version]$Matches[1]
    if ($version -lt [version]"6.5.0") { return }
    # Read the PE machine field instead of trusting the folder name.
    $stream = $null
    $reader = $null
    try {
        $stream = [System.IO.File]::OpenRead((Join-Path $kit "bin\Qt6Core.dll"))
        $reader = New-Object System.IO.BinaryReader($stream)
        if ($reader.ReadUInt16() -ne 0x5A4D) { return }
        $stream.Position = 0x3C
        $peOffset = $reader.ReadInt32()
        if ($peOffset -lt 0 -or $peOffset -gt ($stream.Length - 6)) { return }
        $stream.Position = $peOffset
        if ($reader.ReadUInt32() -ne 0x00004550) { return }
        if ($reader.ReadUInt16() -ne 0x8664) { return } # AMD64, not x86/ARM
    } finally {
        if ($null -ne $reader) { $reader.Dispose() }
        elseif ($null -ne $stream) { $stream.Dispose() }
    }
    [pscustomobject]@{ Path = $kit; Version = $version }
}

function Find-PlannerQt {
    param([string]$ExplicitPath)
    if (![string]::IsNullOrWhiteSpace($ExplicitPath)) {
        $kit = Get-PlannerQtKit -Path $ExplicitPath
        if ($null -eq $kit) {
            throw "Invalid -QtPath '$ExplicitPath'. Supply a Qt 6.5+ MSVC x64 shared-library kit with Widgets, SQL, SQLite, Windows platform plugin and windeployqt. The explicit path will not be silently replaced."
        }
        return $kit
    }

    Write-Host "Searching for Qt 6.5+ MSVC x64 installations..."
    $roots = New-Object 'System.Collections.Generic.List[string]'
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
            if (Test-Path -LiteralPath (Join-Path $bin "qmake.exe") -PathType Leaf) {
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

    $seen = New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::OrdinalIgnoreCase)
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
                try { Get-PlannerQtKit -Path $candidate }
                catch { Write-Verbose "Skipping unreadable Qt candidate '$candidate': $_" }
            }
        }
    )
    if ($kits.Count -eq 0) {
        throw 'No suitable Qt installation found. Install Qt 6.5+ MSVC x64 with Widgets and SQL, or run .\build.ps1 -QtPath "D:\YourQt\6.x.y\msvc2022_64". MinGW, ARM, 32-bit, static and incomplete kits are not selected.'
    }
    # Newest version first; path breaks ties deterministically.
    $kits = @($kits | Sort-Object @{ Expression = { $_.Version }; Descending = $true }, Path)
    if ($kits.Count -gt 1) {
        Write-Host "Found $($kits.Count) suitable kits; selecting the newest. Use -QtPath to override."
    }
    return $kits[0]
}
