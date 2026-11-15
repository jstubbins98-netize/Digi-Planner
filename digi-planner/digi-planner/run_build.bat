@echo off
setlocal
pushd "%~dp0"
if errorlevel 1 (
    echo Could not open the Digi-Planner folder.
    pause
    exit /b 1
)

powershell.exe -ExecutionPolicy Bypass -File ".\build.ps1"
set "BuildExitCode=%ERRORLEVEL%"

echo.
if "%BuildExitCode%"=="0" (
    echo Build completed successfully.
) else (
    echo Build failed. Review the error messages above.
)
popd
pause
exit /b %BuildExitCode%
