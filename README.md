# Persian Win32 Task Manager

A lightweight native Windows task manager built with C++ and the Win32 API. It uses a Persian RTL interface without mirroring the visual layout of controls.

## Features

- Organize tasks into five priority categories (A to E; highest to lowest)
- Add, edit, and delete tasks
- **Double-click a task row to edit its title and category** in a separate window
- **Hold Ctrl and click rows to select multiple tasks**, then delete them together after confirmation
- Delete selected tasks with the Delete key or the delete button
- Persist tasks in `tasks.dat` beside the executable
- Automatically sort tasks by priority
- Optional run-at-login setting in the Settings window
- Native Win32 controls; no Qt or heavyweight GUI framework

## Build

The project targets MSYS2 UCRT64 with GCC and C++11.

On Windows, run `build.bat` (it uses `C:\msys64\ucrt64\bin\g++.exe`), or open the MSYS2 UCRT64 terminal and run:

```bash
bash build.sh
```

The executable `TaskManager.exe` is created in the project directory.

## Data

Tasks are stored in `tasks.dat` beside the executable. Keep this file when replacing the application to retain existing tasks. The data-file format is unchanged in this update.

## License

MIT. See [LICENSE](LICENSE).

For Persian documentation, see [README_FA.md](README_FA.md).
