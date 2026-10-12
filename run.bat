@echo off
rem ==========================================================
rem  run.bat  --  build (if needed) and start the program
rem
rem  Double-click this file to compile and run.
rem  It always runs the exe with the project folder as the
rem  working directory, because the program reads .\data\*.txt
rem  (if you double-click build\hospital.exe directly, the
rem   working directory becomes build\ and it still works now,
rem   because the program also tries ..\data\)
rem
rem  NOTE: this file is kept ASCII-only on purpose (see build.bat).
rem ==========================================================
setlocal
cd /d "%~dp0"

if not exist build\hospital.exe (
    echo hospital.exe not found, building first ...
    call build.bat
    if errorlevel 1 (
        echo.
        echo Build failed. Fix the errors above and try again.
        pause
        exit /b 1
    )
)

echo.
echo Starting the program ...
echo.
build\hospital.exe

echo.
echo Program exited.
pause
