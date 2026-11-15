# No Qt, Pester, registry writes or compilation required. Uses temporary fixtures.
$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest
. (Join-Path $PSScriptRoot "..\Find-Qt.ps1")
$root = Join-Path ([IO.Path]::GetTempPath()) ("planner-qt-tests-" + [guid]::NewGuid())
$passed = 0
function Assert-Failure {
    param([scriptblock]$Action, [string]$Pattern)
    $message = $null
    try { & $Action | Out-Null } catch { $message = $_.Exception.Message }
    if ($null -eq $message -or $message -notmatch $Pattern) {
        throw "Expected error matching '$Pattern', received '$message'."
    }
    $script:passed++
}
function New-TestKit {
    param([string]$Folder, [string]$Version, [int]$Machine = 0x8664)
    foreach ($file in @(
        "lib/Qt6Core.lib", "bin/Qt6Gui.dll", "bin/Qt6Widgets.dll", "bin/Qt6Sql.dll",
        "bin/windeployqt.exe", "plugins/sqldrivers/qsqlite.dll", "plugins/platforms/qwindows.dll",
        "lib/cmake/Qt6/Qt6Config.cmake", "lib/cmake/Qt6Core/Qt6CoreConfig.cmake",
        "lib/cmake/Qt6Gui/Qt6GuiConfig.cmake", "lib/cmake/Qt6Widgets/Qt6WidgetsConfig.cmake",
        "lib/cmake/Qt6Sql/Qt6SqlConfig.cmake", "mkspecs/qconfig.pri"
    )) {
        $target = Join-Path $Folder $file
        [void][IO.Directory]::CreateDirectory((Split-Path $target))
        [IO.File]::WriteAllText($target, "")
    }
    [IO.File]::WriteAllText((Join-Path $Folder "mkspecs/qconfig.pri"), "QT_VERSION = $Version`r`n")
    $bytes = New-Object byte[] 134
    $bytes[0] = 0x4D; $bytes[1] = 0x5A; $bytes[0x3C] = 128
    $bytes[128] = 0x50; $bytes[129] = 0x45
    $bytes[132] = $Machine -band 255; $bytes[133] = $Machine -shr 8
    [IO.File]::WriteAllBytes((Join-Path $Folder "bin/Qt6Core.dll"), $bytes)
}
try {
    foreach ($file in @("build.ps1", "Find-Qt.ps1")) {
        $tokens = $null; $parseErrors = $null
        [void][Management.Automation.Language.Parser]::ParseFile(
            (Join-Path $PSScriptRoot "../$file"), [ref]$tokens, [ref]$parseErrors)
        if ($parseErrors.Count) { throw ($parseErrors -join "`n") }
        $passed++
    }
    $old = Join-Path $root "Qt with spaces/6.8.3/msvc2022_64"
    $latest = Join-Path $root "Qt with spaces/6.9.1/msvc2022_64"
    New-TestKit $old "6.8.3"
    New-TestKit $latest "6.9.1"
    $found = Find-PlannerQt -ExplicitPath (Join-Path $root "Qt with spaces")
    if ($found.Version -ne [version]"6.9.1") { throw "Newest kit not selected." }; $passed++
    foreach ($hint in @($old, (Join-Path $old "bin"), (Join-Path $old "lib/cmake/Qt6"))) {
        $found = Find-PlannerQt -ExplicitPath $hint
        if ($found.Version -ne [version]"6.8.3") { throw "Explicit kit hint failed." }; $passed++
    }
    Remove-Item -LiteralPath (Join-Path $latest "plugins/sqldrivers/qsqlite.dll")
    Assert-Failure { Find-PlannerQt -ExplicitPath $latest } 'qsqlite\.dll'
    $found = Find-PlannerQt -ExplicitPath (Join-Path $root "Qt with spaces")
    if ($found.Version -ne [version]"6.8.3") { throw "Incomplete newest kit was not skipped." }; $passed++
    $arm = Join-Path $root "arm/msvc_arm64"
    New-TestKit $arm "6.9.1" 0xAA64
    Assert-Failure { Find-PlannerQt -ExplicitPath $arm } 'not x64'
    $mingw = Join-Path $root "mingw_64"
    New-TestKit $mingw "6.9.1"
    Remove-Item -LiteralPath (Join-Path $mingw "lib/Qt6Core.lib")
    Assert-Failure { Find-PlannerQt -ExplicitPath $mingw } 'Qt6Core\.lib'
    $unsupported = Join-Path $root "msvc_old"
    New-TestKit $unsupported "6.4.3"
    Assert-Failure { Find-PlannerQt -ExplicitPath $unsupported } '6\.4\.3 is unsupported'
    Assert-Failure { Find-PlannerQt -ExplicitPath (Join-Path $root "missing") } 'does not exist'
    $empty = Join-Path $root "empty"
    [void][IO.Directory]::CreateDirectory($empty)
    Assert-Failure { Find-PlannerQt -ExplicitPath $empty } 'No Qt desktop kit was found'
    Write-Host "$passed checks passed."
} finally {
    if (Test-Path -LiteralPath $root) { Remove-Item -LiteralPath $root -Recurse -Force }
}
