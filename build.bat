@echo off
setlocal

set "MSYS2_ROOT=C:\msys64"
set "UCRT64_BIN=%MSYS2_ROOT%\ucrt64\bin"
set "GXX=%UCRT64_BIN%\g++.exe"
set "SOURCE=%~dp0TaskManager.cpp"
set "OUTPUT=%~dp0TaskManager.exe"

if not exist "%GXX%" (
    echo ERROR: MSYS2 UCRT64 compiler was not found:
    echo %GXX%
    echo Check that MSYS2 UCRT64 is installed in C:\msys64.
    pause
    exit /b 1
)

if not exist "%SOURCE%" (
    echo ERROR: TaskManager.cpp was not found beside build.bat.
    pause
    exit /b 1
)

set "PATH=%UCRT64_BIN%;%MSYS2_ROOT%\usr\bin;%PATH%"
pushd "%~dp0"
echo Building Persian Win32 Task Manager with MSYS2 UCRT64...
"%GXX%" -std=c++11 -O2 -Wall -finput-charset=UTF-8 -mwindows TaskManager.cpp -lcomctl32 -ladvapi32 -static-libgcc -static-libstdc++ -o TaskManager.exe

if errorlevel 1 (
    echo.
    echo Build failed.
    popd
    pause
    exit /b 1
)

echo.
echo Build successful: %OUTPUT%
popd
pause
endlocal
