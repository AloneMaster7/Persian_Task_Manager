@echo off
setlocal

set "MSYS2_ROOT=C:\msys64"
set "UCRT64_BIN=%MSYS2_ROOT%\ucrt64\bin"
set "GXX=%UCRT64_BIN%\g++.exe"
set "WINDRES=%UCRT64_BIN%\windres.exe"
set "SOURCE=%~dp0TaskManager.cpp"
set "ICON=%~dp0icon.ico"
set "OUTPUT=%~dp0TaskManager.exe"
set "TEMP_TAG=PersianTaskManager_%RANDOM%_%RANDOM%"
set "RESOURCE_RC=%TEMP%\%TEMP_TAG%.rc"
set "RESOURCE_OBJ=%TEMP%\%TEMP_TAG%.o"
set "PUSHD_DONE=0"
set "BUILD_RESULT=1"

if not exist "%GXX%" (
    echo ERROR: MSYS2 UCRT64 g++ was not found:
    echo %GXX%
    echo Check that MSYS2 UCRT64 is installed in C:\msys64.
    goto cleanup
)

if not exist "%WINDRES%" (
    echo ERROR: windres was not found:
    echo %WINDRES%
    echo Install the MSYS2 UCRT64 binutils package.
    goto cleanup
)

if not exist "%SOURCE%" (
    echo ERROR: TaskManager.cpp was not found beside build.bat.
    goto cleanup
)

if not exist "%ICON%" (
    echo ERROR: icon.ico was not found.
    echo Put your icon file named icon.ico in this project folder.
    goto cleanup
)

set "PATH=%UCRT64_BIN%;%MSYS2_ROOT%\usr\bin;%PATH%"
pushd "%~dp0"
if errorlevel 1 goto cleanup
set "PUSHD_DONE=1"

rem Generate a temporary resource script automatically; no manual .rc file is needed.
for %%I in ("%ICON%") do set "ICON_RC_PATH=%%~fI"
set "ICON_RC_PATH=%ICON_RC_PATH:\=/%"
> "%RESOURCE_RC%" echo 101 ICON "%ICON_RC_PATH%"

"%WINDRES%" -i "%RESOURCE_RC%" -o "%RESOURCE_OBJ%" -O coff
if errorlevel 1 goto build_failed

echo.
echo Building Persian Win32 Task Manager v1.3.0 with MSYS2 UCRT64...
"%GXX%" -std=c++11 -O2 -Wall -finput-charset=UTF-8 -mwindows TaskManager.cpp "%RESOURCE_OBJ%" -lcomctl32 -ladvapi32 -lshell32 -static-libgcc -static-libstdc++ -o TaskManager.exe
if errorlevel 1 goto build_failed

set "BUILD_RESULT=0"
echo.
echo Build successful: %OUTPUT%
goto cleanup

:build_failed
echo.
echo Build failed. Review the compiler or windres messages above.
set "BUILD_RESULT=1"

:cleanup
if exist "%RESOURCE_RC%" del /q "%RESOURCE_RC%" >nul 2>nul
if exist "%RESOURCE_OBJ%" del /q "%RESOURCE_OBJ%" >nul 2>nul
if "%PUSHD_DONE%"=="1" popd
if "%BUILD_RESULT%"=="0" (
    echo.
    echo Done.
) else (
    echo.
    echo Could not build the application.
)
pause
endlocal & exit /b %BUILD_RESULT%
