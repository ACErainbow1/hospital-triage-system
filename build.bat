@echo off
rem ==========================================================
rem  build.bat  --  one-click build script
rem  Usage: double-click this file, or type build.bat in cmd
rem
rem  NOTE: this file is kept ASCII-only on purpose. cmd.exe reads
rem  .bat files using the system code page, so Chinese characters
rem  saved in UTF-8 would be mis-decoded and could break the script.
rem ==========================================================
setlocal
cd /d "%~dp0"

set CXX=g++
set CXXFLAGS=-std=c++17 -Wall -O2 -Isrc -finput-charset=UTF-8 -fexec-charset=GBK
set LIBS=-leasyx -lgdi32 -lole32 -loleaut32 -lcomdlg32
set SRC=src\main.cpp src\data.cpp src\logic.cpp src\graph.cpp src\ui.cpp

if not exist build mkdir build

echo Compiling ...
%CXX% %CXXFLAGS% %SRC% -o build\hospital.exe %LIBS%
if errorlevel 1 goto fail

echo.
echo Build OK  --^>  build\hospital.exe
echo Run it from this folder (the program reads the data folder).
exit /b 0

:fail
echo.
echo *** BUILD FAILED -- see the error messages above ***
pause
exit /b 1
