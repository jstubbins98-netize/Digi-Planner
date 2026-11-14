# Digi-Planner for Windows 11

a free and open source digital planner app for Windows 11 available for students and businesses.

## Build on Windows 11

Install these prerequisites:

1. **Visual Studio 2026 (version 18) or newer**, Community, Professional, Enterprise or Build Tools, with **Desktop development with C++**, its current MSVC x64 tools (**v145 or newer**) and a Windows 11 SDK.
2. **Qt 6.5 or newer**, using a compatible **MSVC x64 shared-library kit** with Qt Widgets and Qt SQL / SQLite support. Existing kits may still be named `msvc2022_64` (for example Qt 6.8.3). That name identifies the compiler used to build Qt, not the Visual Studio version this app uses. Microsoft's binary-compatibility rules allow older MSVC libraries to be linked by newer MSVC tools. This is not a claim that every Qt release officially certifies VS 2026. Prefer a maintained Qt release; use a compatible kit or rebuild Qt with your selected compiler if you encounter compatibility issues. Do not select MinGW, ARM or WebAssembly kits.
3. **CMake 4.2 or newer**, added to PATH. CMake 4.2 introduced the `Visual Studio 18 2026` generator. For later Visual Studio releases, install a CMake version that supports that release.

Open PowerShell in this folder and run:

```powershell
.\build.ps1 -Launch
```

Before configuring or compiling, the script automatically searches Qt environment hints (`QTDIR`, `QT_ROOT_DIR`, `Qt6_DIR`, `CMAKE_PREFIX_PATH`), `qmake.exe` locations on PATH, `Qt` folders on local fixed drives, Program Files and user folders, and Qt uninstall-registry installation locations. It checks each root and two directory levels below it rather than scanning entire drives.

It selects the newest detected Qt 6.5+ MSVC x64 shared-library kit with the required CMake modules, DLLs, SQLite driver, Windows platform plugin and deployment tool. The selected version and path are printed. Incompatible or incomplete kits are skipped; if none qualify, the script lists searched roots and rejection reasons, including missing files or the wrong architecture. For a custom location or to choose a specific kit, supply an explicit path (an invalid explicit path stops the build instead of falling back):

```powershell
.\build.ps1 -QtPath "C:\Qt\6.8.3\msvc2022_64" -Launch
```

`-QtPath` also accepts an installation root such as `C:\Qt`, a version folder, or a kit's `bin` / `lib\cmake\Qt6` folder. It searches only within the supplied location, not unrelated installations.

### If Qt cannot be found

The message **“No suitable Qt installation found”** occurs before compilation. It means Qt is missing, installed outside the searched locations, or was rejected as incompatible or incomplete. Older versions of the script did not report the specific reason.

If updating from an earlier source ZIP, replace **both `build.ps1` and `Find-Qt.ps1`** with the versions from the updated package. Keep them together in the `digi-planner` folder.

Run detection without compiling:

```powershell
.\build.ps1 -DiagnoseQt
# If you installed Qt at a custom location, replace this path with that actual folder:
.\build.ps1 -QtPath "C:\Qt" -DiagnoseQt
```

If no kit is found, install the **Qt libraries**, not just Qt Creator or Visual Studio. Use the [Qt installer](https://www.qt.io/download-qt-installer-oss) or an existing Qt Maintenance Tool to add a Qt 6.5+ **MSVC x64 desktop kit**. Qt Base includes Widgets and SQL; the SQLite and Windows platform plugins must be present. Do not select MinGW or ARM for this project. If a kit is found but rejected, the diagnostic output names the missing file or incompatible architecture. Repair the listed components with Qt Maintenance Tool.

If detection still fails, share the complete diagnostic output, including the searched folders and rejection reasons. Use your actual Qt installation path rather than copying an example path that does not exist on your PC.

Once detection succeeds, compile and launch with `.\build.ps1 -Launch`, or include the same `-QtPath` override if your installation is in a custom location.

The discovery regression checks can be run with `pwsh -NoProfile -File .\tests\QtDiscovery.Tests.ps1`. They use temporary simulated kits, not a real Qt installation. Passing them does not verify native Windows compilation or registry discovery.

If PowerShell blocks the local script, use a one-process policy override, only after reviewing the script:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\build.ps1 -Launch
```

The script detects the newest installed stable Visual Studio 2026-or-newer instance with C++ tools using Microsoft's `vswhere`, then selects its matching generator from CMake's available generators. It fails with upgrade instructions if CMake does not support that version; it never silently falls back to VS 2022. Builds are stored separately under `build\vs18-x64` (or the selected newer major version), leaving old build folders intact.

The script refreshes CMake's configuration cache so switching Qt kits does not reuse an old Qt path, builds Release and stages the application and Qt runtime in **`dist`**. Run **`dist\DigiPlanner.exe`**. The script does not install dependencies automatically or change your system policy.

### Manual equivalent

```powershell
cmake -S . -B build\vs18-x64 -G "Visual Studio 18 2026" -A x64 -DCMAKE_PREFIX_PATH="C:\Qt\6.8.3\msvc2022_64"
cmake --build build\vs18-x64 --config Release --parallel
cmake --install build\vs18-x64 --config Release --prefix dist
.\dist\DigiPlanner.exe
```

For newer Visual Studio releases, use the matching generator listed by `cmake --help`, or simply use the detection script above. The CMake project rejects non-Windows, non-MSVC, pre-2026 Visual Studio generators and toolsets older than v145. At startup the app checks the actual Windows build (22000 or newer) and workstation product type, rejecting older Windows releases and Windows Server.

References: [CMake's VS 2026 generator](https://cmake.org/cmake/help/latest/generator/Visual%20Studio%2018%202026.html), [Microsoft's MSVC binary compatibility rules](https://learn.microsoft.com/en-us/cpp/porting/binary-compat-2015-2017), and [Qt's supported Windows configurations](https://doc.qt.io/qt-6/windows.html).

## Features

- Add assignments with a name, due date, class, status, priority, estimated decimal hours and weekly recurrence.
- Weekly recurrence inserts exactly **8 entries**: the initial date and seven weekly occurrences, in one database transaction.
- Select a table row to instantly save changes to **Class**, **Status** and **Priority** in the bottom action bar. These are the editable fields requested in the specification; name, date and hours are set when creating an assignment.
- Delete a selected assignment after confirmation. Recurring occurrences are independent: editing or deleting one does not affect the others.
- Add classes; remove them after confirmation. Assignments move to **Other**, which cannot be removed. Defaults are seeded only during initial database setup, so removed classes stay removed.
- Status dots: red Not Started, orange In Progress, green Completed, dark yellow On Hold. Priority stars: red High, orange Medium, green Low. Text labels and a legend accompany the colors.
- Case-insensitive name search combines with class and status filters; Clear Filters resets them together.
- Sortable list starts in ascending due-date order. The pending-hours total counts only visible non-completed assignments. A zero estimate means no estimate was set.
- Calendar with month/year navigation, up to five status dots per day and a `+N` overflow indicator. Click a day for all event details, including empty days. Calendar always shows all events, independent of list filters.
- On launch, a dialog lists unfinished assignments due today through three days ahead (inclusive); completed and overdue assignments are excluded.
- Export **all events**, independent of filters, to UTF-8 TXT or a landscape PDF, sorted by due date. Exports include timestamp, count and every specified field. PDF rows alternate colors and paginate automatically.
- Local SQLite persistence after each successful operation, explicit failure dialogs and single-instance protection.

## Data and backups

The database is **`%LOCALAPPDATA%\DigiPlanner\DigiPlanner\planner.db`**.

Unlike the original cross-platform description, the Windows edition stores data in the Windows user's writable local application-data folder, **not** the executable's working directory. This avoids permission failures under Program Files and preserves assignments when the program is updated. Hover over “Saved automatically on this PC” to see the exact path.

Close Digi-Planner before copying `planner.db` for a backup. To restore, close the app, keep a copy of the current database, then replace it with your backup. TXT and PDF are reports, not importable backups.

## Sharing a build

Share the **entire `dist` folder** as a ZIP. Do not share only the EXE: Qt DLLs, `platforms\qwindows.dll` and `sqldrivers\qsqlite.dll` are required. The Qt deploy step collects runtime dependencies. A recipient may also need the latest Microsoft Visual C++ x64 Redistributable, at least as new as the build tools used for the application and its dependencies.

This is an unsigned portable-folder application, not an installer. Windows SmartScreen may warn about unsigned downloads. Code signing and installer creation are not included. Review Qt's LGPL/GPL or commercial licensing requirements before distributing; retain applicable license notices and satisfy the terms for your chosen Qt distribution.

## Source layout

```
CMakeLists.txt           Windows-only build and Qt deployment
build.ps1               Windows build and stage script
Find-Qt.ps1              Bounded Qt discovery and kit validation
src/main.cpp            Windows check, data path, single instance, theme
src/Event.h             Event structure and status/priority colors
src/Database.*          SQLite schema and transactional operations
src/MainWindow.*        Assignment form, table, filters and class management
src/CalendarView.*      Calendar cell markers and day-details dialog
src/Export.*            Atomic TXT/PDF exports
```

## License

license is Apache License, Version 2.0
