# Persian Win32 Task Manager

A small, native Windows desktop app for organizing tasks into five priority categories. It is written in **C++11 using the Win32 API** and has a Persian (Farsi) interface.

**Language:** Persian / فارسی · **Platform:** Windows · **UI:** Native Win32 API · **License:** MIT

[راهنمای فارسی](README_FA.md)

## Features

- Add tasks and assign each one to one of five categories (A–E).
- Delete the selected task. Deletions are persisted when you save.
- Save tasks to `tasks.dat` in the same folder as `TaskManager.exe`.
- Keep saved tasks after closing and reopening the application.
- Warn before closing when there are unsaved changes; choose to save, discard, or cancel.
- Native Windows controls with a simple blue-and-green visual style.
- Persian text is right-to-left while the overall control layout is **not mirrored**.
- No third-party GUI framework is used.

## Priority categories

| Internal category | Meaning |
|---|---|
| A | Not doing the task may have severe consequences. |
| B | Doing the task is beneficial, but not doing it has no severe consequences. |
| C | Not doing the task has minor consequences. |
| D | Low-priority work that may be more efficiently handled by someone else. |
| E | No real necessity or importance. |

The app displays the categories with Persian labels (الف، ب، ج، د، هـ).

## Requirements

- Windows 10 or later (64-bit recommended).
- [MSYS2](https://www.msys2.org/) with the **UCRT64** environment and its GCC C++ compiler.
- The build scripts assume MSYS2 is installed in `C:\msys64`. If it is installed elsewhere, update `MSYS2_ROOT` in `build.bat`, or run `build.sh` from the UCRT64 terminal where `g++` is on `PATH`.

## Build

### Option 1: MSYS2 UCRT64 terminal (recommended)

1. Extract or clone this repository.
2. Open **MSYS2 UCRT64** (not the plain MSYS terminal).
3. Change to the project directory. For example:

   ```bash
   cd /c/path/to/Persian-Win32-Task-Manager
   ```

4. Run:

   ```bash
   bash build.sh
   ```

The executable `TaskManager.exe` will be created in the project directory.

### Option 2: Windows batch file

Run `build.bat` from Windows, or from the MSYS2 UCRT64 terminal run:

```bash
cmd.exe /c build.bat
```

Both scripts compile `TaskManager.cpp` with C++11 and link the Windows common-controls library (`comctl32`). The C++ and GCC runtime libraries are linked statically; Windows system libraries are still required as usual.

## Data and privacy

- The application stores task data locally in `tasks.dat`, next to the executable.
- The task file is created when you save for the first time.
- To move your tasks to another folder or computer, close the app and copy `tasks.dat` together with `TaskManager.exe`.
- Task content is not uploaded or sent to a server by this application.
- `tasks.dat` and the generated executable are intentionally excluded from Git by `.gitignore`.

## Notes

This repository contains source code and build scripts; it does not include a prebuilt executable. The application is implemented with the native Win32 API and standard C++ library. The ComboBox drop-down has been adjusted to reserve sufficient height for all five category options.

## License

Released under the [MIT License](LICENSE).
