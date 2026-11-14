# Persian Win32 Task Manager

A lightweight native Windows task manager built with C++ and the Win32 API. No Qt or heavyweight GUI framework is required.

## Features

- Five priority categories (A–E), automatically sorted from highest to lowest priority
- Add, edit, and delete tasks
- Double-click a row to edit its title and category
- Ctrl-click to select multiple tasks; delete selected tasks with the Delete button or `Delete` key
- Persistent local storage in `tasks.dat` beside the executable
- Optional run-at-login setting
- Persian RTL text while preserving the normal (non-mirrored) control layout
- Custom application icon in the executable, window title bar, and Windows notification area (System Tray)
- Minimize or close the main window to the System Tray; click the tray icon to restore it
- Tray context menu for opening the app, opening About, or fully exiting
- About window with clickable GitHub and Telegram links

## Priority categories

| Category | Meaning |
|---|---|
| A | Severe consequences if left undone |
| B | Beneficial to do, but no severe consequences if omitted |
| C | Minor consequences if left undone |
| D | Low-priority tasks better delegated to someone else |
| E | No real necessity or importance |

## Add your application icon

Place your Windows icon file, named exactly `icon.ico`, in the same folder as `TaskManager.cpp` and the build scripts:

```text
Project/
├── TaskManager.cpp
├── icon.ico
├── build.bat
├── build.sh
├── README.md
└── README_FA.md
```

You do **not** need to create an `.rc` file manually. Both build scripts generate a temporary resource script, compile it with `windres`, link the icon into the executable, and remove the temporary files. The icon is embedded in the EXE, so `icon.ico` is not needed beside the finished executable at runtime.

For best results, use a real Windows ICO file containing multiple sizes (16, 32, 48, and 256 pixels). Renaming a PNG file to `.ico` is not sufficient.

## Build on Windows

The project targets MSYS2 UCRT64, GCC, and C++11. The UCRT64 GCC toolchain and binutils package (which provides `windres`) are required.

- Run `build.bat` on Windows. It expects MSYS2 at `C:\msys64\ucrt64\bin`.
- Or open the MSYS2 UCRT64 terminal, change to the project directory, and run:

```bash
bash build.sh
```

The output executable is `TaskManager.exe` in the project folder.

## System Tray behavior

Minimizing or closing the main window keeps the application running in the notification area next to the clock. Click the tray icon to restore the main window. Right-click it to open the context menu. Choose **Exit** there to terminate the application; if there are unsaved changes, the app asks whether to save them first.

## Links

- GitHub: https://github.com/AloneMaster7
- Telegram: https://t.me/AISpecialists_ir

## Data storage

Tasks are stored in `tasks.dat` beside the executable. Keep that file when replacing the executable to retain existing tasks. The task-file format is unchanged.

## License

MIT. See [LICENSE](LICENSE).
