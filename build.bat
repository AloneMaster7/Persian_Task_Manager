@echo off
setlocal

rem Build with the MSYS2 UCRT64 toolchain installed at C:\msys64.
set "MSYS2_ROOT=C:\msys64"
set "UCRT64_BIN=%MSYS2_ROOT%\ucrt64\bin"
set "GXX=%UCRT64_BIN%\g++.exe"
set "SOURCE=%~dp0TaskManager.cpp"
set "OUTPUT=%~dp0TaskManager.exe"

if not exist "%GXX%" (
    echo ERROR: MSYS2 UCRT64 g++ was not found:
    echo   %GXX%
    echo Install or update the UCRT64 GCC toolchain, or edit MSYS2_ROOT in build.bat.
    pause
    exit /b 1
)

if not exist "%SOURCE%" (
    echo ERROR: TaskManager.cpp was not found next to build.bat.
    pause
    exit /b 1
)

set "PATH=%UCRT64_BIN%;%MSYS2_ROOT%\usr\bin;%PATH%"
echo Building Task Manager with MSYS2 UCRT64...
"%GXX%" -std=c++11 -O2 -Wall -finput-charset=UTF-8 -mwindows "%SOURCE%" -lcomctl32 -static-libgcc -static-libstdc++ -o "%OUTPUT%"

if errorlevel 1 goto build_failed

echo.
echo Build successful:
echo   %OUTPUT%
pause
exit /b 0

:build_failed
echo.
echo Build failed. Read the compiler messages above.
pause
exit /b 1
