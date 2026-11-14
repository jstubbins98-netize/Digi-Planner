# Digi-Planner for Windows 11

A native C++17 / Qt 6 student planner. No browser, Node.js, account, server, or network service is used by this application. Windows 11 x64 is the supported target.

## Build on Windows 11

Install these prerequisites:

1. **Visual Studio 2022 Community or Build Tools**, with **Desktop development with C++**, the MSVC v143 x64 tools and a Windows 11 SDK.
2. **Qt 6.5 or newer**, using a **MSVC 2022 64-bit kit** (for example Qt 6.8.3 `msvc2022_64`). The Qt kit and compiler must match. Include Qt Widgets and Qt SQL / SQLite support. Do not select the MinGW, ARM or WebAssembly kits for this build.
3. **CMake 3.24 or newer**, added to PATH.

Open PowerShell in this folder and run, replacing the Qt path with your installed kit:

```powershell
.\build.ps1 -QtPath "C:\Qt\6.8.3\msvc2022_64" -Launch
```

If PowerShell blocks the local script, use a one-process policy override, only after reviewing the script:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\build.ps1 -QtPath "C:\Qt\6.8.3\msvc2022_64" -Launch
```

The script builds Release and stages the application and Qt runtime in **`dist`**. Run **`dist\DigiPlanner.exe`**. The script does not install dependencies automatically or change your system policy.

### Manual equivalent

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="C:\Qt\6.8.3\msvc2022_64"
cmake --build build --config Release --parallel
cmake --install build --config Release --prefix dist
.\dist\DigiPlanner.exe
```

The CMake project rejects non-Windows and non-MSVC builds. At startup the app checks the actual Windows build (22000 or newer) and workstation product type, rejecting older Windows releases and Windows Server.

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

Share the **entire `dist` folder** as a ZIP. Do not share only the EXE: Qt DLLs, `platforms\qwindows.dll` and `sqldrivers\qsqlite.dll` are required. The Qt deploy step collects runtime dependencies. A recipient may also need Microsoft's Visual C++ 2015–2022 x64 Redistributable.

This is an unsigned portable-folder application, not an installer. Windows SmartScreen may warn about unsigned downloads. Code signing and installer creation are not included. Review Qt's LGPL/GPL or commercial licensing requirements before distributing; retain applicable license notices and satisfy the terms for your chosen Qt distribution.

## Source layout

```
CMakeLists.txt           Windows-only build and Qt deployment
build.ps1               Windows build and stage script
src/main.cpp            Windows check, data path, single instance, theme
src/Event.h             Event structure and status/priority colors
src/Database.*          SQLite schema and transactional operations
src/MainWindow.*        Assignment form, table, filters and class management
src/CalendarView.*      Calendar cell markers and day-details dialog
src/Export.*            Atomic TXT/PDF exports
```

## Verification status

This source was authored in a Linux environment. The Windows executable has **not** been built or run here. Build and native GUI verification on Windows 11 are still required; this delivery does not include a compiled EXE.

After building on Windows, check:

1. Create an assignment, restart and confirm persistence.
2. Create a weekly assignment and verify eight dates spaced seven days apart.
3. Select an event, change class/status/priority, and confirm filters and pending hours update.
4. Remove a class with assignments; confirm they move to Other and the class stays removed after restart.
5. Test calendar dots with six events on one date and open its details.
6. Restart with unfinished assignments due in 0, 1, 2 and 3 days; confirm the startup reminder.
7. Export while filters are active; confirm all assignments are included. Export enough events to check multi-page PDF output.
8. Delete an event, including cancellation, and confirm only the chosen occurrence is removed.
9. Run the packaged `dist` folder on another Windows 11 x64 PC to verify runtime deployment.
