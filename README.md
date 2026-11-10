# Persian Win32 Task Manager

**Version: 1.0**

A lightweight, native Windows desktop application for organizing tasks by priority. The application is written in **C++11** using the **Win32 API** and stores task data locally on the computer.

## Features

- Add tasks with a title and one of five priority categories.
- Automatically sort tasks from category **A** to **E**, while keeping the existing order of tasks within the same category.
- Display task titles centered in the task list and visually distinguish categories with colors.
- Delete the selected task, with a confirmation prompt before deletion.
- Save task data manually, and receive a prompt to save unsaved changes when closing the application.
- Open a separate Settings window and enable or disable automatic startup when the current Windows user signs in.
- Verify startup registry changes and report Windows errors if an operation fails.
- Keep task data on the local computer; task contents are not sent to a server.

## Priority categories

| Category | Meaning |
| --- | --- |
| A | Serious consequences may result from not completing the task. |
| B | Completing the task is beneficial, but not doing it has no serious consequences. |
| C | Not completing the task may have minor consequences. |
| D | Low-priority tasks that may be delegated to others. |
| E | Tasks with no real necessity or importance. |

Tasks are sorted in the order **A → B → C → D → E**.

## Requirements

- Windows desktop environment.
- A C++ compiler with C++11 support and Windows development headers.
- The Windows libraries `comctl32` and `advapi32` (available with the Windows toolchain).

No Qt, .NET, Electron, or other GUI framework is required.

## Build on Windows

1. Place `TaskManager.cpp` and `build.bat` in the same folder.
2. Make sure one of these compiler paths is available:
   - `C:\MinGW\bin\g++.exe`
   - `C:\msys64\ucrt64\bin\g++.exe`
3. Run `build.bat`.
4. When compilation succeeds, `TaskManager.exe` is created in the project folder.

The build script invokes `g++` with C++11 support and links `comctl32` and `advapi32`.

### Build with MSYS2 UCRT64

Open the **MSYS2 UCRT64** terminal, change to the project directory, and run:

```bash
bash build.sh
```

The script must be run from the UCRT64 environment.

## Data storage

Tasks are stored in a binary file named `tasks.dat` in the same directory as `TaskManager.exe`.

- Keep `tasks.dat` beside the executable to retain your task list.
- To move the application and its data to another folder or computer, close the application and copy both `TaskManager.exe` and `tasks.dat`.
- The application needs write permission for its folder to save changes.
- Adding or deleting tasks marks the list as modified. Use **Save** to write the changes to disk. When closing with unsaved changes, the application asks whether to save, discard, or cancel closing.

## Automatic startup

The startup option is available from the gear-shaped **Settings** button at the bottom of the main window.

When enabled, the application registers its executable path under the current Windows user's Run key:

```text
HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run
```

The registry value is named `PersianWin32TaskManager`. This is a per-user setting and does not normally require administrator privileges. When disabled, the application removes only its own registry value; it does not modify other startup entries or delete task data. The application checks the registry after attempting a change and keeps the displayed setting unchanged if the operation fails.

## Project files

| File | Description |
| --- | --- |
| `TaskManager.cpp` | Main application source code. |
| `build.bat` | Windows build script for MinGW or MSYS2 UCRT64. |
| `build.sh` | Build script for the MSYS2 UCRT64 terminal. |
| `README.md` | Project documentation in English. |
| `README_FA.md` | Project documentation in Persian. |

## Notes

- The application uses native Win32 controls and APIs.
- Very long task titles may be visually shortened with an ellipsis when they exceed the available column width; the complete title remains stored in the task data.
- Startup is configured for the Windows account currently signed in, not for every account on the computer.
