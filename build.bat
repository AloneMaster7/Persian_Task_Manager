@echo off
setlocal

rem Prefer the standalone MinGW installation; fall back to MSYS2 UCRT64.
set "GXX=C:\MinGW\bin\g++.exe"
set "TOOLPATH=C:\MinGW\bin"
if exist "%GXX%" goto compiler_found

set "GXX=C:\msys64\ucrt64\bin\g++.exe"
set "TOOLPATH=C:\msys64\ucrt64\bin;C:\msys64\usr\bin"
if exist "%GXX%" goto compiler_found

echo ERROR: g++.exe was not found.
echo Checked: C:\MinGW\bin\g++.exe
echo          C:\msys64\ucrt64\bin\g++.exe
pause
exit /b 1

:compiler_found
set "PATH=%TOOLPATH%;%PATH%"
pushd "%~dp0"
echo Building Persian Win32 Task Manager...
"%GXX%" -std=c++11 -O2 -Wall -finput-charset=UTF-8 -mwindows TaskManager.cpp -lcomctl32 -ladvapi32 -static-libgcc -static-libstdc++ -o TaskManager.exe
if errorlevel 1 (
    echo.
    echo Build failed.
    popd
    pause
    exit /b 1
)

echo.
echo Build successful: %~dp0TaskManager.exe
popd
pause
endlocal
