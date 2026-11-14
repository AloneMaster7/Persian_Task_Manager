#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#ifndef _WIN32_IE
#define _WIN32_IE 0x0600
#endif

#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <vector>
#include <string>
#include <algorithm>
#include <cstring>


// -----------------------------------------------------------------------------
//  Task Manager - native Win32 API, C++11
//  Data is stored in tasks.dat beside the executable.
// -----------------------------------------------------------------------------

struct Task
{
    std::wstring title;
    wchar_t category; // A, B, C, D or E
};

static HINSTANCE g_instance = NULL;
static HWND g_window = NULL;
static HWND g_titleLabel = NULL;
static HWND g_subtitleLabel = NULL;
static HWND g_taskLabel = NULL;
static HWND g_categoryLabel = NULL;
static HWND g_taskEdit = NULL;
static HWND g_categoryCombo = NULL;
static HWND g_addButton = NULL;
static HWND g_legendLabel = NULL;
static HWND g_listCaption = NULL;
static HWND g_list = NULL;
static HWND g_deleteButton = NULL;
static HWND g_saveButton = NULL;
static HWND g_statusLabel = NULL;
static HWND g_settingsButton = NULL;
static HWND g_aboutButton = NULL;

static HWND g_aboutWindow = NULL;
static HWND g_aboutTitleLabel = NULL;
static HWND g_aboutSubtitleLabel = NULL;
static HWND g_aboutDescriptionLabel = NULL;
static HWND g_aboutVersionLabel = NULL;
static HWND g_aboutFooterLabel = NULL;
static HWND g_aboutGithubButton = NULL;
static HWND g_aboutTelegramButton = NULL;

static HICON g_appIconBig = NULL;
static HICON g_appIconSmall = NULL;
static bool g_trayIconAdded = false;
static bool g_trayNoticeShown = false;
static bool g_mainWasMaximized = false;
static UINT g_taskbarCreatedMessage = 0;

static HWND g_settingsWindow = NULL;
static HWND g_editWindow = NULL;
static HWND g_editHeadingLabel = NULL;
static HWND g_editSubheadingLabel = NULL;
static HWND g_editTitleLabel = NULL;
static HWND g_editCategoryLabel = NULL;
static HWND g_editTitleEdit = NULL;
static HWND g_editCategoryCombo = NULL;
static HWND g_editSaveButton = NULL;
static HWND g_editCancelButton = NULL;
static int g_editTaskIndex = -1;
static bool g_mainShuttingDown = false;

static HWND g_settingsHeading = NULL;
static HWND g_settingsSubheading = NULL;
static HWND g_settingsOptionLabel = NULL;
static HWND g_startupStateLabel = NULL;
static HWND g_settingsHintLabel = NULL;
static HWND g_startupToggle = NULL;
static bool g_startupEnabled = false;
static bool g_startupStateKnown = false;
static LONG g_startupLastError = ERROR_SUCCESS;

static HFONT g_font = NULL;
static HFONT g_boldFont = NULL;
static HBRUSH g_backgroundBrush = NULL;
static HBRUSH g_headerBrush = NULL;
static HBRUSH g_whiteBrush = NULL;

static std::vector<Task> g_tasks;
static std::wstring g_dataPath;
static bool g_dirty = false;
static bool g_loadError = false;

static const int ID_TASK_EDIT = 101;
static const int ID_CATEGORY_COMBO = 102;
static const int ID_ADD = 103;
static const int ID_LIST = 104;
static const int ID_DELETE = 105;
static const int ID_SAVE = 106;
static const int ID_SETTINGS = 107;
static const int ID_ABOUT = 108;
static const int ID_TRAY_OPEN = 601;
static const int ID_TRAY_ABOUT = 602;
static const int ID_TRAY_EXIT = 603;
static const int ID_ABOUT_GITHUB = 701;
static const int ID_ABOUT_TELEGRAM = 702;
static const int IDI_APP_RESOURCE = 101;
static const UINT WM_TRAY_CALLBACK = WM_APP + 0x51;
static const int ID_STARTUP_TOGGLE = 301;
static const int ID_EDIT_TITLE = 401;
static const int ID_EDIT_CATEGORY = 402;
static const int ID_EDIT_SAVE = 403;
static const int ID_EDIT_CANCEL = 404;

// A dropdown COMBOBOX HWND must retain enough total height for its list.
// The closed selection field remains visually compact; the extra height is
// used only when the drop-down list is displayed.
static const int CATEGORY_COMBO_WIDTH = 310;
static const int CATEGORY_COMBO_WINDOW_HEIGHT = 210;

static const COLORREF COLOR_BG = RGB(242, 247, 250);
static const COLORREF COLOR_NAVY = RGB(20, 49, 72);
static const COLORREF COLOR_GREEN = RGB(31, 158, 119);
static const COLORREF COLOR_BLUE = RGB(48, 116, 181);
static const COLORREF COLOR_RED = RGB(211, 83, 83);
static const COLORREF COLOR_TEXT = RGB(42, 57, 70);
static const COLORREF COLOR_MUTED = RGB(111, 128, 142);

static void LoadApplicationIcons(HINSTANCE instance)
{
    g_appIconBig = reinterpret_cast<HICON>(LoadImageW(
        instance, MAKEINTRESOURCEW(IDI_APP_RESOURCE), IMAGE_ICON,
        GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR));
    g_appIconSmall = reinterpret_cast<HICON>(LoadImageW(
        instance, MAKEINTRESOURCEW(IDI_APP_RESOURCE), IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR));
}

static void DestroyApplicationIcons()
{
    if (g_appIconBig)
        DestroyIcon(g_appIconBig);
    if (g_appIconSmall)
        DestroyIcon(g_appIconSmall);
    g_appIconBig = NULL;
    g_appIconSmall = NULL;
}

static void AddTrayIcon(HWND window)
{
    if (g_trayIconAdded || !g_appIconSmall)
        return;

    NOTIFYICONDATAW data;
    ZeroMemory(&data, sizeof(data));
    data.cbSize = sizeof(data);
    data.hWnd = window;
    data.uID = 1;
    data.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    data.uCallbackMessage = WM_TRAY_CALLBACK;
    data.hIcon = g_appIconSmall;
    lstrcpynW(data.szTip, L"مدیریت کارها", static_cast<int>(sizeof(data.szTip) / sizeof(data.szTip[0])));

    if (Shell_NotifyIconW(NIM_ADD, &data))
        g_trayIconAdded = true;
}

static void RemoveTrayIcon(HWND window)
{
    if (!g_trayIconAdded)
        return;

    NOTIFYICONDATAW data;
    ZeroMemory(&data, sizeof(data));
    data.cbSize = sizeof(data);
    data.hWnd = window;
    data.uID = 1;
    Shell_NotifyIconW(NIM_DELETE, &data);
    g_trayIconAdded = false;
}

static void ShowTrayNotice(HWND window)
{
    if (!g_trayIconAdded || g_trayNoticeShown)
        return;

    NOTIFYICONDATAW data;
    ZeroMemory(&data, sizeof(data));
    data.cbSize = sizeof(data);
    data.hWnd = window;
    data.uID = 1;
    data.uFlags = NIF_INFO;
    lstrcpynW(data.szInfoTitle, L"مدیریت کارها", static_cast<int>(sizeof(data.szInfoTitle) / sizeof(data.szInfoTitle[0])));
    lstrcpynW(data.szInfo,
              L"برنامه همچنان در پس‌زمینه فعال است. برای خروج کامل، روی آیکون کنار ساعت راست‌کلیک کنید.",
              static_cast<int>(sizeof(data.szInfo) / sizeof(data.szInfo[0])));
    data.dwInfoFlags = NIIF_INFO;
    if (Shell_NotifyIconW(NIM_MODIFY, &data))
        g_trayNoticeShown = true;
}

static void ShowMainWindow(HWND window)
{
    // Remember whether the main window was maximized before it was hidden
    // or minimized to the notification area.
    ShowWindow(window, g_mainWasMaximized ? SW_SHOWMAXIMIZED : SW_SHOWNORMAL);
    SetForegroundWindow(window);
    BringWindowToTop(window);
}

static void ShowTrayContextMenu(HWND window)
{
    HMENU menu = CreatePopupMenu();
    if (!menu)
        return;

    AppendMenuW(menu, MF_STRING, ID_TRAY_OPEN, L"باز کردن برنامه");
    AppendMenuW(menu, MF_STRING, ID_TRAY_ABOUT, L"درباره برنامه");
    AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(menu, MF_STRING, ID_TRAY_EXIT, L"خروج کامل");

    POINT point;
    GetCursorPos(&point);
    SetForegroundWindow(window);
    TrackPopupMenu(menu, TPM_RIGHTBUTTON | TPM_RIGHTALIGN | TPM_BOTTOMALIGN,
                   point.x, point.y, 0, window, NULL);
    PostMessageW(window, WM_NULL, 0, 0);
    DestroyMenu(menu);
}

// Keep the task vector in priority order (A -> E). stable_sort preserves
// the previous order of tasks that belong to the same category.
static void SortTasksByCategory()
{
    std::stable_sort(g_tasks.begin(), g_tasks.end(),
        [](const Task& left, const Task& right)
        {
            return left.category < right.category;
        });
}

static std::wstring Trim(const std::wstring& value)
{
    const std::wstring::size_type first = value.find_first_not_of(L" \t\r\n");
    if (first == std::wstring::npos)
        return L"";

    const std::wstring::size_type last = value.find_last_not_of(L" \t\r\n");
    return value.substr(first, last - first + 1);
}

static bool WriteExact(HANDLE file, const void* data, DWORD bytes)
{
    const BYTE* current = static_cast<const BYTE*>(data);
    DWORD total = 0;

    while (total < bytes)
    {
        DWORD written = 0;
        if (!WriteFile(file, current + total, bytes - total, &written, NULL) || written == 0)
            return false;
        total += written;
    }
    return true;
}

static bool ReadExact(HANDLE file, void* data, DWORD bytes)
{
    BYTE* current = static_cast<BYTE*>(data);
    DWORD total = 0;

    while (total < bytes)
    {
        DWORD received = 0;
        if (!ReadFile(file, current + total, bytes - total, &received, NULL) || received == 0)
            return false;
        total += received;
    }
    return true;
}

static void BuildDataPath()
{
    std::vector<wchar_t> buffer(32768, L'\0');
    DWORD length = GetModuleFileNameW(NULL, &buffer[0], static_cast<DWORD>(buffer.size()));

    if (length == 0 || length >= buffer.size())
    {
        g_dataPath = L"tasks.dat";
        return;
    }

    std::wstring executablePath(&buffer[0], length);
    const std::wstring::size_type slash = executablePath.find_last_of(L"\\/");
    if (slash != std::wstring::npos)
        executablePath.resize(slash + 1);
    else
        executablePath.clear();

    g_dataPath = executablePath + L"tasks.dat";
}

// Startup is controlled explicitly from the Settings window. The per-user Run
// key does not require administrator privileges.
static const wchar_t* STARTUP_RUN_KEY =
    L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
static const wchar_t* STARTUP_VALUE_NAME = L"PersianWin32TaskManager";

static LONG ReadStartupState(bool* enabled)
{
    if (!enabled)
        return ERROR_INVALID_PARAMETER;

    *enabled = false;
    HKEY runKey = NULL;
    LONG result = RegOpenKeyExW(
        HKEY_CURRENT_USER, STARTUP_RUN_KEY, 0, KEY_QUERY_VALUE, &runKey);

    if (result == ERROR_FILE_NOT_FOUND || result == ERROR_PATH_NOT_FOUND)
        return ERROR_SUCCESS; // Missing Run key means startup is disabled.
    if (result != ERROR_SUCCESS)
        return result;

    DWORD type = 0;
    DWORD size = 0;
    result = RegQueryValueExW(
        runKey, STARTUP_VALUE_NAME, NULL, &type, NULL, &size);

    if (result == ERROR_FILE_NOT_FOUND)
        result = ERROR_SUCCESS;
    else if (result == ERROR_SUCCESS)
        *enabled = true;

    RegCloseKey(runKey);
    return result;
}

static LONG SetStartupState(bool enabled)
{
    HKEY runKey = NULL;
    LONG result = ERROR_SUCCESS;

    if (enabled)
    {
        std::vector<wchar_t> pathBuffer(32768, L'\0');
        const DWORD pathLength = GetModuleFileNameW(
            NULL, &pathBuffer[0], static_cast<DWORD>(pathBuffer.size()));

        if (pathLength == 0)
            return static_cast<LONG>(GetLastError());
        if (pathLength >= pathBuffer.size())
            return ERROR_INSUFFICIENT_BUFFER;

        const std::wstring executablePath(&pathBuffer[0], pathLength);
        const std::wstring command = L"\"" + executablePath + L"\"";

        result = RegCreateKeyExW(
            HKEY_CURRENT_USER,
            STARTUP_RUN_KEY,
            0,
            NULL,
            REG_OPTION_NON_VOLATILE,
            KEY_QUERY_VALUE | KEY_SET_VALUE,
            NULL,
            &runKey,
            NULL);

        if (result != ERROR_SUCCESS)
            return result;

        const DWORD dataSize = static_cast<DWORD>(
            (command.size() + 1) * sizeof(wchar_t));
        result = RegSetValueExW(
            runKey,
            STARTUP_VALUE_NAME,
            0,
            REG_SZ,
            reinterpret_cast<const BYTE*>(command.c_str()),
            dataSize);

        // Verify the value after writing it. Do not show the switch as ON
        // unless the exact executable command is present in the registry.
        if (result == ERROR_SUCCESS)
        {
            std::vector<wchar_t> verifyBuffer(command.size() + 2, L'\0');
            DWORD verifySize = static_cast<DWORD>(verifyBuffer.size() * sizeof(wchar_t));
            DWORD verifyType = 0;
            result = RegQueryValueExW(
                runKey,
                STARTUP_VALUE_NAME,
                NULL,
                &verifyType,
                reinterpret_cast<BYTE*>(&verifyBuffer[0]),
                &verifySize);

            if (result == ERROR_SUCCESS &&
                (verifyType != REG_SZ || std::wstring(&verifyBuffer[0]) != command))
            {
                result = ERROR_INVALID_DATA;
            }
        }

        RegCloseKey(runKey);
        return result;
    }

    result = RegOpenKeyExW(
        HKEY_CURRENT_USER,
        STARTUP_RUN_KEY,
        0,
        KEY_QUERY_VALUE | KEY_SET_VALUE,
        &runKey);

    if (result == ERROR_FILE_NOT_FOUND || result == ERROR_PATH_NOT_FOUND)
        return ERROR_SUCCESS; // Already disabled.
    if (result != ERROR_SUCCESS)
        return result;

    result = RegDeleteValueW(runKey, STARTUP_VALUE_NAME);
    if (result == ERROR_FILE_NOT_FOUND)
        result = ERROR_SUCCESS; // Idempotent: deleting an absent value is OK.

    // Confirm that the value is actually gone before reporting success.
    if (result == ERROR_SUCCESS)
    {
        DWORD type = 0;
        DWORD size = 0;
        const LONG verifyResult = RegQueryValueExW(
            runKey, STARTUP_VALUE_NAME, NULL, &type, NULL, &size);

        if (verifyResult == ERROR_SUCCESS)
            result = ERROR_GEN_FAILURE;
        else if (verifyResult != ERROR_FILE_NOT_FOUND)
            result = verifyResult;
    }

    RegCloseKey(runKey);
    return result;
}

static void ShowStartupError(HWND owner, LONG errorCode)
{
    wchar_t* systemText = NULL;
    const DWORD chars = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        static_cast<DWORD>(errorCode),
        0,
        reinterpret_cast<LPWSTR>(&systemText),
        0,
        NULL);

    std::wstring message =
        L"تغییر تنظیم اجرای خودکار انجام نشد. وضعیت قبلی حفظ شده است.";
    if (chars && systemText)
        message += L"\n\nجزئیات: " + Trim(systemText);
    message += L"\nکد خطا: " + std::to_wstring(errorCode);

    if (systemText)
        LocalFree(systemText);

    MessageBoxW(owner, message.c_str(), L"خطا در تنظیم استارتاپ", MB_OK | MB_ICONERROR);
}

static bool SaveTasks()
{
    if (g_dataPath.empty())
        BuildDataPath();

    HANDLE file = CreateFileW(
        g_dataPath.c_str(),
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (file == INVALID_HANDLE_VALUE)
        return false;

    const char magic[8] = { 'T', 'A', 'S', 'K', 'S', '0', '1', '\0' };
    const DWORD count = static_cast<DWORD>(g_tasks.size());
    bool ok = WriteExact(file, magic, sizeof(magic)) && WriteExact(file, &count, sizeof(count));

    for (size_t i = 0; ok && i < g_tasks.size(); ++i)
    {
        const DWORD category = static_cast<DWORD>(g_tasks[i].category - L'A');
        const DWORD length = static_cast<DWORD>(g_tasks[i].title.size());

        if (length > 10000 || category > 4)
        {
            ok = false;
            break;
        }

        ok = WriteExact(file, &category, sizeof(category)) &&
             WriteExact(file, &length, sizeof(length));

        if (ok && length > 0)
            ok = WriteExact(file, g_tasks[i].title.data(), length * sizeof(wchar_t));
    }

    if (ok)
        ok = FlushFileBuffers(file) != 0;

    CloseHandle(file);

    if (ok)
        g_dirty = false;

    return ok;
}

static bool LoadTasks()
{
    if (g_dataPath.empty())
        BuildDataPath();

    HANDLE file = CreateFileW(
        g_dataPath.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (file == INVALID_HANDLE_VALUE)
    {
        const DWORD error = GetLastError();
        if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND)
            return true; // First launch: no saved file yet.
        return false;
    }

    char magic[8] = { 0 };
    DWORD count = 0;
    const char expected[8] = { 'T', 'A', 'S', 'K', 'S', '0', '1', '\0' };
    bool ok = ReadExact(file, magic, sizeof(magic)) &&
              ReadExact(file, &count, sizeof(count)) &&
              memcmp(magic, expected, sizeof(expected)) == 0 &&
              count <= 10000;

    std::vector<Task> loaded;

    for (DWORD i = 0; ok && i < count; ++i)
    {
        DWORD category = 0;
        DWORD length = 0;

        ok = ReadExact(file, &category, sizeof(category)) &&
             ReadExact(file, &length, sizeof(length)) &&
             category <= 4 && length > 0 && length <= 10000;

        if (!ok)
            break;

        Task task;
        task.category = static_cast<wchar_t>(L'A' + category);
        task.title.assign(length, L'\0');
        ok = ReadExact(file, &task.title[0], length * sizeof(wchar_t));

        if (ok)
        {
            if (task.title.find(L'\0') != std::wstring::npos)
                ok = false;
            else
                loaded.push_back(task);
        }
    }

    CloseHandle(file);

    if (ok)
    {
        g_tasks.swap(loaded);
        SortTasksByCategory();
    }

    return ok;
}

static COLORREF CategoryTextColor(wchar_t category)
{
    switch (category)
    {
    case L'A': return RGB(190, 61, 61);
    case L'B': return RGB(38, 111, 168);
    case L'C': return RGB(166, 113, 27);
    case L'D': return RGB(111, 83, 158);
    default:   return RGB(99, 111, 122);
    }
}

static COLORREF CategoryBackColor(wchar_t category)
{
    switch (category)
    {
    case L'A': return RGB(255, 235, 235);
    case L'B': return RGB(232, 244, 252);
    case L'C': return RGB(255, 247, 228);
    case L'D': return RGB(243, 238, 251);
    default:   return RGB(239, 243, 246);
    }
}

static std::wstring ToPersianDigits(size_t value)
{
    std::wstring digits = std::to_wstring(value);
    const wchar_t persianDigits[10] = { L'۰', L'۱', L'۲', L'۳', L'۴', L'۵', L'۶', L'۷', L'۸', L'۹' };

    for (size_t i = 0; i < digits.size(); ++i)
    {
        if (digits[i] >= L'0' && digits[i] <= L'9')
            digits[i] = persianDigits[digits[i] - L'0'];
    }
    return digits;
}

static const wchar_t* CategoryDisplayName(wchar_t category)
{
    switch (category)
    {
    case L'A': return L"الف";
    case L'B': return L"ب";
    case L'C': return L"ج";
    case L'D': return L"د";
    default:   return L"هـ";
    }
}

static void UpdateStatus()
{
    if (!g_statusLabel)
        return;

    std::wstring message;
    if (g_loadError)
        message = L"فایل ذخیره قبلی قابل خواندن نبود؛ اطلاعات جدید را ذخیره کنید.";
    else if (g_dirty)
        message = L"تغییرات ذخیره نشده است";
    else
        message = L"اطلاعات ذخیره شده است";

    message += L"    •    تعداد کارها: ";
    message += ToPersianDigits(g_tasks.size());
    SetWindowTextW(g_statusLabel, message.c_str());
}

static void UpdateDeleteButton()
{
    if (!g_deleteButton || !g_list)
        return;

    const int selectedCount = static_cast<int>(ListView_GetSelectedCount(g_list));
    EnableWindow(g_deleteButton, selectedCount > 0);
}

static void RefreshList()
{
    if (!g_list)
        return;

    // Rebuild the rows as one operation. This prevents ListView from painting
    // intermediate, partially rebuilt rows while the backing task vector is
    // already in its new sorted order.
    SendMessageW(g_list, WM_SETREDRAW, FALSE, 0);

    ListView_DeleteAllItems(g_list);

    for (size_t i = 0; i < g_tasks.size(); ++i)
    {
        LVITEMW item;
        ZeroMemory(&item, sizeof(item));
        item.mask = LVIF_TEXT;
        item.iItem = static_cast<int>(i);
        item.iSubItem = 0;
        item.pszText = const_cast<LPWSTR>(g_tasks[i].title.c_str());

        const int row = ListView_InsertItem(g_list, &item);
        if (row >= 0)
        {
            // Set the label explicitly after insertion as well. This avoids
            // relying on the initial LVITEM text pointer for the first column.
            ListView_SetItemText(
                g_list, row, 0, const_cast<LPWSTR>(g_tasks[i].title.c_str()));
            const wchar_t* categoryText = CategoryDisplayName(g_tasks[i].category);
            ListView_SetItemText(g_list, row, 1, const_cast<LPWSTR>(categoryText));
        }
    }

    // A full rebuild invalidates the old row selection. Explicitly clear it
    // so a newly inserted first row is not accidentally painted as selected.
    ListView_SetItemState(g_list, -1, 0, LVIS_SELECTED | LVIS_FOCUSED);

    SendMessageW(g_list, WM_SETREDRAW, TRUE, 0);
    RedrawWindow(g_list, NULL, NULL,
                 RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);

    UpdateDeleteButton();
    UpdateStatus();
}

static HWND CreateLabel(HWND parent, const wchar_t* text, DWORD style, int id)
{
    HWND control = CreateWindowExW(
        WS_EX_RTLREADING,
        L"STATIC",
        text,
        WS_CHILD | WS_VISIBLE | style,
        0, 0, 10, 10,
        parent,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
        g_instance,
        NULL);

    if (control && g_font)
        SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(g_font), TRUE);

    return control;
}

static HWND CreateActionButton(HWND parent, const wchar_t* text, int id)
{
    HWND control = CreateWindowExW(
        0,
        L"BUTTON",
        text,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
        0, 0, 10, 10,
        parent,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
        g_instance,
        NULL);

    if (control && g_font)
        SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(g_font), TRUE);

    return control;
}

static void MarkDirty(); // Implemented with the main task-editing commands below.

static void LayoutEditWindow(HWND window)
{
    RECT client;
    GetClientRect(window, &client);
    const int width = client.right - client.left;
    const int pad = 28;
    const int fieldWidth = width - 2 * pad;

    if (g_editHeadingLabel)
        MoveWindow(g_editHeadingLabel, pad, 10, fieldWidth, 30, TRUE);
    if (g_editSubheadingLabel)
        MoveWindow(g_editSubheadingLabel, pad, 42, fieldWidth, 20, TRUE);
    if (g_editTitleLabel)
        MoveWindow(g_editTitleLabel, pad, 91, fieldWidth, 21, TRUE);
    if (g_editTitleEdit)
        MoveWindow(g_editTitleEdit, pad, 116, fieldWidth, 36, TRUE);
    if (g_editCategoryLabel)
        MoveWindow(g_editCategoryLabel, pad, 171, fieldWidth, 21, TRUE);
    if (g_editCategoryCombo)
        MoveWindow(g_editCategoryCombo, pad, 196, fieldWidth,
                   CATEGORY_COMBO_WINDOW_HEIGHT, TRUE);

    const int buttonWidth = 132;
    const int buttonY = 420;
    if (g_editSaveButton)
        MoveWindow(g_editSaveButton, width - pad - buttonWidth,
                   buttonY, buttonWidth, 38, TRUE);
    if (g_editCancelButton)
        MoveWindow(g_editCancelButton, width - pad - 2 * buttonWidth - 12,
                   buttonY, buttonWidth, 38, TRUE);
}

static void SaveEditedTask(HWND window)
{
    if (g_editTaskIndex < 0 ||
        static_cast<size_t>(g_editTaskIndex) >= g_tasks.size())
    {
        MessageBoxW(window, L"کار موردنظر برای ویرایش پیدا نشد.",
                    L"خطا در ویرایش", MB_OK | MB_ICONERROR);
        DestroyWindow(window);
        return;
    }

    const int length = GetWindowTextLengthW(g_editTitleEdit);
    if (length <= 0)
    {
        MessageBoxW(window, L"عنوان کار نمی‌تواند خالی باشد.",
                    L"ویرایش کار", MB_OK | MB_ICONINFORMATION);
        SetFocus(g_editTitleEdit);
        return;
    }

    std::vector<wchar_t> buffer(static_cast<size_t>(length) + 1, L'\0');
    GetWindowTextW(g_editTitleEdit, &buffer[0], length + 1);
    const std::wstring title = Trim(&buffer[0]);
    if (title.empty())
    {
        MessageBoxW(window, L"عنوان کار نمی‌تواند خالی باشد.",
                    L"ویرایش کار", MB_OK | MB_ICONINFORMATION);
        SetFocus(g_editTitleEdit);
        return;
    }

    const int selectedCategory = static_cast<int>(
        SendMessageW(g_editCategoryCombo, CB_GETCURSEL, 0, 0));
    if (selectedCategory < 0 || selectedCategory > 4)
    {
        MessageBoxW(window, L"لطفاً یک دسته‌بندی معتبر انتخاب کنید.",
                    L"ویرایش کار", MB_OK | MB_ICONINFORMATION);
        SetFocus(g_editCategoryCombo);
        return;
    }

    Task& task = g_tasks[static_cast<size_t>(g_editTaskIndex)];
    const wchar_t category = static_cast<wchar_t>(L'A' + selectedCategory);
    const bool changed = (task.title != title || task.category != category);
    if (changed)
    {
        task.title = title;
        task.category = category;
        SortTasksByCategory();
        MarkDirty();
        RefreshList();
    }

    DestroyWindow(window);
}

static LRESULT CALLBACK EditWindowProc(
    HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
    {
        g_editWindow = window;
        g_editHeadingLabel = CreateLabel(
            window, L"ویرایش کار", SS_RIGHT, 501);
        g_editSubheadingLabel = CreateLabel(
            window, L"عنوان و دسته‌بندی کار را تغییر دهید.", SS_RIGHT, 502);
        g_editTitleLabel = CreateLabel(
            window, L"عنوان کار", SS_RIGHT, 503);
        g_editCategoryLabel = CreateLabel(
            window, L"دسته‌بندی", SS_RIGHT, 504);

        g_editTitleEdit = CreateWindowExW(
            WS_EX_CLIENTEDGE | WS_EX_RTLREADING,
            L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL | ES_RIGHT,
            0, 0, 10, 10, window,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_EDIT_TITLE)),
            g_instance, NULL);
        if (g_editTitleEdit)
            SendMessageW(g_editTitleEdit, EM_SETLIMITTEXT, 10000, 0);

        g_editCategoryCombo = CreateWindowExW(
            WS_EX_CLIENTEDGE | WS_EX_RTLREADING,
            L"COMBOBOX", L"",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL,
            0, 0, CATEGORY_COMBO_WIDTH, CATEGORY_COMBO_WINDOW_HEIGHT,
            window,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_EDIT_CATEGORY)),
            g_instance, NULL);

        const wchar_t* categories[5] = {
            L"الف ـ در صورت انجام ندادن، پیامد شدید",
            L"ب ـ انجام آن سودمند است؛ بدون پیامد شدید",
            L"ج ـ در صورت انجام ندادن، پیامد جزئی",
            L"د ـ کم‌اهمیت و مناسب واگذاری به دیگران",
            L"هـ ـ بدون ضرورت یا اهمیت"
        };
        if (g_editCategoryCombo)
        {
            if (g_font)
                SendMessageW(g_editCategoryCombo, WM_SETFONT,
                             reinterpret_cast<WPARAM>(g_font), TRUE);
            for (int i = 0; i < 5; ++i)
            {
                const LRESULT added = SendMessageW(
                    g_editCategoryCombo, CB_ADDSTRING, 0,
                    reinterpret_cast<LPARAM>(categories[i]));
                if (added == CB_ERR || added == CB_ERRSPACE)
                {
                    MessageBoxW(window, L"بارگذاری دسته‌بندی‌ها ناموفق بود.",
                                L"خطا", MB_OK | MB_ICONERROR);
                    break;
                }
            }
            SendMessageW(g_editCategoryCombo, CB_SETITEMHEIGHT,
                         static_cast<WPARAM>(-1), 34);
            SendMessageW(g_editCategoryCombo, CB_SETITEMHEIGHT, 0, 32);
            SendMessageW(g_editCategoryCombo, CB_SETMINVISIBLE, 5, 0);
            SendMessageW(g_editCategoryCombo, CB_SETDROPPEDWIDTH, 430, 0);
        }

        g_editSaveButton = CreateWindowExW(
            0, L"BUTTON", L"ذخیره تغییرات",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
            0, 0, 10, 10, window,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_EDIT_SAVE)),
            g_instance, NULL);
        g_editCancelButton = CreateWindowExW(
            0, L"BUTTON", L"انصراف",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
            0, 0, 10, 10, window,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_EDIT_CANCEL)),
            g_instance, NULL);

        HWND controls[] = {
            g_editTitleEdit, g_editSaveButton, g_editCancelButton
        };
        for (size_t i = 0; i < sizeof(controls) / sizeof(controls[0]); ++i)
        {
            if (controls[i] && g_font)
                SendMessageW(controls[i], WM_SETFONT,
                             reinterpret_cast<WPARAM>(g_font), TRUE);
        }

        if (g_editTaskIndex >= 0 &&
            static_cast<size_t>(g_editTaskIndex) < g_tasks.size())
        {
            const Task& task = g_tasks[static_cast<size_t>(g_editTaskIndex)];
            SetWindowTextW(g_editTitleEdit, task.title.c_str());
            const int categoryIndex = static_cast<int>(task.category - L'A');
            SendMessageW(g_editCategoryCombo, CB_SETCURSEL,
                         (categoryIndex >= 0 && categoryIndex < 5) ? categoryIndex : 0, 0);
        }
        else
        {
            SendMessageW(g_editCategoryCombo, CB_SETCURSEL, 0, 0);
        }

        LayoutEditWindow(window);
        return 0;
    }

    case WM_SIZE:
        LayoutEditWindow(window);
        return 0;

    case WM_ERASEBKGND:
    {
        RECT rect;
        GetClientRect(window, &rect);
        FillRect(reinterpret_cast<HDC>(wParam), &rect, g_backgroundBrush);
        return 1;
    }

    case WM_PAINT:
    {
        PAINTSTRUCT paint;
        HDC dc = BeginPaint(window, &paint);
        RECT client;
        GetClientRect(window, &client);
        RECT header = { 0, 0, client.right, 72 };
        FillRect(dc, &header, g_headerBrush);
        HBRUSH accentBrush = CreateSolidBrush(COLOR_GREEN);
        RECT accent = { 0, 0, 6, 72 };
        FillRect(dc, &accent, accentBrush);
        DeleteObject(accentBrush);
        HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(225, 233, 239));
        HGDIOBJ oldPen = SelectObject(dc, borderPen);
        HGDIOBJ oldBrush = SelectObject(dc, g_whiteBrush);
        RoundRect(dc, 18, 84, client.right - 18, client.bottom - 18, 12, 12);
        SelectObject(dc, oldBrush);
        SelectObject(dc, oldPen);
        DeleteObject(borderPen);
        EndPaint(window, &paint);
        return 0;
    }

    case WM_CTLCOLORSTATIC:
    {
        HDC dc = reinterpret_cast<HDC>(wParam);
        HWND control = reinterpret_cast<HWND>(lParam);
        SetBkMode(dc, TRANSPARENT);
        if (control == g_editHeadingLabel)
        {
            SetTextColor(dc, RGB(255, 255, 255));
            return reinterpret_cast<LRESULT>(g_headerBrush);
        }
        if (control == g_editSubheadingLabel)
        {
            SetTextColor(dc, RGB(205, 222, 234));
            return reinterpret_cast<LRESULT>(g_headerBrush);
        }
        SetTextColor(dc, COLOR_TEXT);
        return reinterpret_cast<LRESULT>(g_whiteBrush);
    }

    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    {
        HDC dc = reinterpret_cast<HDC>(wParam);
        SetTextColor(dc, COLOR_TEXT);
        SetBkColor(dc, RGB(255, 255, 255));
        return reinterpret_cast<LRESULT>(g_whiteBrush);
    }

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case ID_EDIT_SAVE:
            if (HIWORD(wParam) == BN_CLICKED)
                SaveEditedTask(window);
            return 0;
        case ID_EDIT_CANCEL:
            if (HIWORD(wParam) == BN_CLICKED)
                DestroyWindow(window);
            return 0;
        default:
            break;
        }
        break;

    case WM_CLOSE:
        DestroyWindow(window);
        return 0;

    case WM_DESTROY:
        g_editWindow = NULL;
        g_editHeadingLabel = NULL;
        g_editSubheadingLabel = NULL;
        g_editTitleLabel = NULL;
        g_editCategoryLabel = NULL;
        g_editTitleEdit = NULL;
        g_editCategoryCombo = NULL;
        g_editSaveButton = NULL;
        g_editCancelButton = NULL;
        g_editTaskIndex = -1;
        if (!g_mainShuttingDown && g_window && IsWindow(g_window))
        {
            EnableWindow(g_window, TRUE);
            SetForegroundWindow(g_window);
        }
        return 0;
    }

    return DefWindowProcW(window, message, wParam, lParam);
}

static void OpenEditTaskWindow(int taskIndex)
{
    if (taskIndex < 0 || static_cast<size_t>(taskIndex) >= g_tasks.size())
        return;

    if (g_editWindow && IsWindow(g_editWindow))
    {
        ShowWindow(g_editWindow, SW_SHOWNORMAL);
        SetForegroundWindow(g_editWindow);
        return;
    }

    g_editTaskIndex = taskIndex;
    RECT ownerRect;
    GetWindowRect(g_window, &ownerRect);
    const int windowWidth = 540;
    const int windowHeight = 510;
    const int x = ownerRect.left + ((ownerRect.right - ownerRect.left) - windowWidth) / 2;
    const int y = ownerRect.top + ((ownerRect.bottom - ownerRect.top) - windowHeight) / 2;

    HWND editWindow = CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_RTLREADING | WS_EX_CONTROLPARENT,
        L"NativeTaskManagerEdit",
        L"ویرایش کار",
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        x, y, windowWidth, windowHeight,
        g_window, NULL, g_instance, NULL);

    if (!editWindow)
    {
        g_editTaskIndex = -1;
        MessageBoxW(g_window, L"باز کردن پنجره ویرایش ناموفق بود.",
                    L"خطا", MB_OK | MB_ICONERROR);
        return;
    }

    EnableWindow(g_window, FALSE);
    ShowWindow(editWindow, SW_SHOWNORMAL);
    UpdateWindow(editWindow);
    SetFocus(g_editTitleEdit);
}


static void LayoutControls(HWND window)
{
    if (!g_titleLabel || !g_list)
        return;

    RECT client;
    GetClientRect(window, &client);
    const int width = client.right - client.left;
    const int height = client.bottom - client.top;
    const int pad = 28;

    MoveWindow(g_titleLabel, pad, 15, width - 2 * pad, 35, TRUE);
    MoveWindow(g_subtitleLabel, pad, 51, width - 2 * pad, 22, TRUE);

    const int innerWidth = width - 2 * pad;
    const int addWidth = 112;
    const int comboWidth = CATEGORY_COMBO_WIDTH;
    const int gap = 12;
    int editWidth = innerWidth - addWidth - comboWidth - 2 * gap;
    if (editWidth < 180)
        editWidth = 180;

    const int addX = pad;
    const int comboX = addX + addWidth + gap;
    const int editX = comboX + comboWidth + gap;

    MoveWindow(g_taskLabel, editX, 108, editWidth, 20, TRUE);
    MoveWindow(g_categoryLabel, comboX, 108, comboWidth, 20, TRUE);
    MoveWindow(g_taskEdit, editX, 132, editWidth, 36, TRUE);
    // Do not shrink a drop-down combo to the visible field height. Its HWND
    // height also reserves the list area; 36px leaves no room for its items.
    MoveWindow(g_categoryCombo, comboX, 132, comboWidth, CATEGORY_COMBO_WINDOW_HEIGHT, TRUE);
    MoveWindow(g_addButton, addX, 132, addWidth, 36, TRUE);

    MoveWindow(g_legendLabel, pad + 4, 180, innerWidth - 8, 20, TRUE);
    MoveWindow(g_listCaption, pad + 12, 211, innerWidth - 24, 24, TRUE);

    const int listX = pad + 12;
    const int listY = 241;
    const int listWidth = innerWidth - 24;
    const int listHeight = height - listY - 91;
    MoveWindow(g_list, listX, listY, listWidth, (listHeight > 120 ? listHeight : 120), TRUE);

    const int buttonY = height - 58;
    const int saveWidth = 112;
    const int deleteWidth = 190;
    const int actionGap = 12;
    const int saveX = width - pad - saveWidth;
    const int deleteX = saveX - actionGap - deleteWidth;
    const int settingsWidth = 42;
    const int aboutWidth = 42;
    const int utilityGap = 8;
    const int aboutX = pad + settingsWidth + utilityGap;
    const int statusX = aboutX + aboutWidth + 12;
    const int statusWidth = deleteX - statusX - actionGap;

    MoveWindow(g_saveButton, saveX, buttonY, saveWidth, 38, TRUE);
    MoveWindow(g_deleteButton, deleteX, buttonY, deleteWidth, 38, TRUE);
    MoveWindow(g_settingsButton, pad, buttonY, settingsWidth, 38, TRUE);
    MoveWindow(g_aboutButton, aboutX, buttonY, aboutWidth, 38, TRUE);
    MoveWindow(g_statusLabel, statusX, height - 53, statusWidth, 28, TRUE);

    if (g_list)
    {
        const int categoryColumn = 105;
        ListView_SetColumnWidth(g_list, 1, categoryColumn);
        ListView_SetColumnWidth(g_list, 0, listWidth - categoryColumn - 4);
    }

    InvalidateRect(window, NULL, TRUE);
}

static void MarkDirty()
{
    g_dirty = true;
    UpdateStatus();
}

static void AddTask()
{
    const int length = GetWindowTextLengthW(g_taskEdit);
    if (length <= 0)
    {
        MessageBoxW(g_window, L"ابتدا عنوان کار را وارد کنید.", L"افزودن کار", MB_OK | MB_ICONINFORMATION);
        SetFocus(g_taskEdit);
        return;
    }

    std::vector<wchar_t> buffer(static_cast<size_t>(length) + 1, L'\0');
    GetWindowTextW(g_taskEdit, &buffer[0], length + 1);
    const std::wstring title = Trim(&buffer[0]);

    if (g_tasks.size() >= 10000)
    {
        MessageBoxW(g_window, L"حداکثر ۱۰۰۰۰ کار قابل ثبت است.", L"محدودیت برنامه", MB_OK | MB_ICONINFORMATION);
        return;
    }

    if (title.empty())
    {
        MessageBoxW(g_window, L"عنوان کار نمی‌تواند خالی باشد.", L"افزودن کار", MB_OK | MB_ICONINFORMATION);
        SetFocus(g_taskEdit);
        return;
    }

    const int selectedCategory = static_cast<int>(SendMessageW(g_categoryCombo, CB_GETCURSEL, 0, 0));
    if (selectedCategory < 0 || selectedCategory > 4)
        return;

    Task task;
    task.title = title;
    task.category = static_cast<wchar_t>(L'A' + selectedCategory);
    g_tasks.push_back(task);
    SortTasksByCategory();

    MarkDirty();
    RefreshList();
    SetWindowTextW(g_taskEdit, L"");
    SetFocus(g_taskEdit);
}

static void DeleteSelectedTask()
{
    if (!g_list || g_tasks.empty())
        return;

    // Collect every selected row first. Erase in descending index order so
    // removing one row never changes the indices of rows still to be removed.
    std::vector<int> selectedRows;
    int row = -1;
    while ((row = ListView_GetNextItem(g_list, row, LVNI_SELECTED)) != -1)
    {
        if (row >= 0 && static_cast<size_t>(row) < g_tasks.size())
            selectedRows.push_back(row);
    }

    if (selectedRows.empty())
        return;

    std::wstring prompt;
    if (selectedRows.size() == 1)
    {
        prompt = L"این کار حذف شود؟\nبا زدن دکمه ذخیره، حذف آن برای همیشه ثبت می‌شود.";
    }
    else
    {
        prompt = L"آیا ";
        prompt += ToPersianDigits(selectedRows.size());
        prompt += L" کار انتخاب‌شده حذف شوند؟\nبا زدن دکمه ذخیره، حذف آن‌ها برای همیشه ثبت می‌شود.";
    }

    const int answer = MessageBoxW(
        g_window,
        prompt.c_str(),
        L"حذف کار",
        MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);

    if (answer != IDYES)
        return;

    for (std::vector<int>::reverse_iterator it = selectedRows.rbegin();
         it != selectedRows.rend(); ++it)
    {
        g_tasks.erase(g_tasks.begin() + *it);
    }

    MarkDirty();
    RefreshList();
}

static void SaveAndReport()
{
    if (SaveTasks())
    {
        g_loadError = false;
        UpdateStatus();
    }
    else
    {
        MessageBoxW(
            g_window,
            L"ذخیره‌سازی انجام نشد. مطمئن شوید پوشه‌ای که فایل اجرایی در آن قرار دارد، قابل نوشتن است.",
            L"خطا در ذخیره‌سازی",
            MB_OK | MB_ICONERROR);
    }
}

static void DrawActionButton(const DRAWITEMSTRUCT* item)
{
    if (!item || item->CtlType != ODT_BUTTON)
        return;

    if (item->CtlID == ID_SETTINGS)
    {
        const bool pressed = (item->itemState & ODS_SELECTED) != 0;
        const COLORREF buttonColor = pressed ? RGB(211, 225, 235) : RGB(229, 238, 244);
        HBRUSH buttonBrush = CreateSolidBrush(buttonColor);
        HPEN buttonPen = CreatePen(PS_SOLID, 1, buttonColor);
        HGDIOBJ oldBrush = SelectObject(item->hDC, buttonBrush);
        HGDIOBJ oldPen = SelectObject(item->hDC, buttonPen);
        RoundRect(item->hDC, item->rcItem.left + 1, item->rcItem.top + 1,
                  item->rcItem.right - 1, item->rcItem.bottom - 1, 10, 10);
        SelectObject(item->hDC, oldBrush);
        SelectObject(item->hDC, oldPen);
        DeleteObject(buttonBrush);
        DeleteObject(buttonPen);

        // A simple three-slider glyph is clearer at this small button size
        // than the previous hand-drawn gear.
        const int cx = (item->rcItem.left + item->rcItem.right) / 2;
        const int cy = (item->rcItem.top + item->rcItem.bottom) / 2;
        HPEN sliderPen = CreatePen(PS_SOLID, 2, COLOR_NAVY);
        HBRUSH sliderBrush = CreateSolidBrush(buttonColor);
        HGDIOBJ oldSliderPen = SelectObject(item->hDC, sliderPen);
        HGDIOBJ oldSliderBrush = SelectObject(item->hDC, sliderBrush);
        const int yPositions[3] = { cy - 7, cy, cy + 7 };
        const int knobPositions[3] = { cx - 4, cx + 5, cx - 1 };
        for (int i = 0; i < 3; ++i)
        {
            MoveToEx(item->hDC, cx - 11, yPositions[i], NULL);
            LineTo(item->hDC, cx + 11, yPositions[i]);
            Ellipse(item->hDC,
                    knobPositions[i] - 3, yPositions[i] - 3,
                    knobPositions[i] + 4, yPositions[i] + 4);
        }
        SelectObject(item->hDC, oldSliderBrush);
        SelectObject(item->hDC, oldSliderPen);
        DeleteObject(sliderBrush);
        DeleteObject(sliderPen);
        return;
    }

    if (item->CtlID == ID_ABOUT)
    {
        const bool pressed = (item->itemState & ODS_SELECTED) != 0;
        const COLORREF buttonColor = pressed ? RGB(211, 225, 235) : RGB(229, 238, 244);
        HBRUSH buttonBrush = CreateSolidBrush(buttonColor);
        HPEN buttonPen = CreatePen(PS_SOLID, 1, RGB(213, 225, 234));
        HGDIOBJ oldBrush = SelectObject(item->hDC, buttonBrush);
        HGDIOBJ oldPen = SelectObject(item->hDC, buttonPen);
        RoundRect(item->hDC, item->rcItem.left + 1, item->rcItem.top + 1,
                  item->rcItem.right - 1, item->rcItem.bottom - 1, 10, 10);
        SelectObject(item->hDC, oldBrush);
        SelectObject(item->hDC, oldPen);
        DeleteObject(buttonBrush);
        DeleteObject(buttonPen);

        const int cx = (item->rcItem.left + item->rcItem.right) / 2;
        const int cy = (item->rcItem.top + item->rcItem.bottom) / 2;
        HPEN markPen = CreatePen(PS_SOLID, 2, COLOR_NAVY);
        HGDIOBJ oldMarkPen = SelectObject(item->hDC, markPen);
        HGDIOBJ oldMarkBrush = SelectObject(item->hDC, GetStockObject(NULL_BRUSH));
        Ellipse(item->hDC, cx - 10, cy - 10, cx + 10, cy + 10);
        SelectObject(item->hDC, oldMarkBrush);
        SelectObject(item->hDC, oldMarkPen);
        DeleteObject(markPen);

        SetBkMode(item->hDC, TRANSPARENT);
        SetTextColor(item->hDC, COLOR_NAVY);
        RECT questionRect = { cx - 7, cy - 9, cx + 7, cy + 9 };
        DrawTextW(item->hDC, L"?", -1, &questionRect,
                  DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        if (item->itemState & ODS_FOCUS)
        {
            RECT focusRect = item->rcItem;
            InflateRect(&focusRect, -4, -4);
            DrawFocusRect(item->hDC, &focusRect);
        }
        return;
    }

    COLORREF color = COLOR_BLUE;
    if (item->CtlID == ID_ADD)
        color = COLOR_GREEN;
    else if (item->CtlID == ID_DELETE)
        color = COLOR_RED;
    else if (item->CtlID == ID_SAVE)
        color = COLOR_BLUE;

    if (item->itemState & ODS_DISABLED)
        color = RGB(166, 178, 187);
    else if (item->itemState & ODS_SELECTED)
        color = RGB(GetRValue(color) - 18, GetGValue(color) - 18, GetBValue(color) - 18);

    HBRUSH brush = CreateSolidBrush(color);
    HPEN pen = CreatePen(PS_SOLID, 1, color);
    HGDIOBJ oldBrush = SelectObject(item->hDC, brush);
    HGDIOBJ oldPen = SelectObject(item->hDC, pen);

    RoundRect(item->hDC,
              item->rcItem.left + 1,
              item->rcItem.top + 1,
              item->rcItem.right - 1,
              item->rcItem.bottom - 1,
              9, 9);

    SetBkMode(item->hDC, TRANSPARENT);
    SetTextColor(item->hDC, RGB(255, 255, 255));
    RECT textRect = item->rcItem;
    if (item->itemState & ODS_SELECTED)
        OffsetRect(&textRect, 0, 1);

    wchar_t text[128] = { 0 };
    GetWindowTextW(item->hwndItem, text, 127);
    DrawTextW(item->hDC, text, -1, &textRect,
              DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_RTLREADING);

    if (item->itemState & ODS_FOCUS)
    {
        RECT focusRect = item->rcItem;
        InflateRect(&focusRect, -5, -5);
        DrawFocusRect(item->hDC, &focusRect);
    }

    SelectObject(item->hDC, oldBrush);
    SelectObject(item->hDC, oldPen);
    DeleteObject(brush);
    DeleteObject(pen);
}

static void CreateControls(HWND window)
{
    g_titleLabel = CreateLabel(window, L"مدیریت کارها", SS_RIGHT, 201);
    g_subtitleLabel = CreateLabel(window, L"کارها را بر اساس میزان ضرورت دسته‌بندی و اولویت‌بندی کنید.", SS_RIGHT, 202);
    g_taskLabel = CreateLabel(window, L"عنوان کار", SS_RIGHT, 203);
    g_categoryLabel = CreateLabel(window, L"دسته‌بندی", SS_RIGHT, 204);
    g_legendLabel = CreateLabel(
        window,
        L"الف: پیامد شدید     ب: سودمند     ج: پیامد جزئی     د: قابل واگذاری     هـ: بدون اهمیت",
        SS_RIGHT,
        205);
    g_listCaption = CreateLabel(window, L"فهرست کارها", SS_RIGHT, 206);

    g_taskEdit = CreateWindowExW(
        WS_EX_CLIENTEDGE | WS_EX_RTLREADING,
        L"EDIT",
        L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL | ES_RIGHT,
        0, 0, 10, 10,
        window,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_TASK_EDIT)),
        g_instance,
        NULL);

    SendMessageW(g_taskEdit, EM_SETLIMITTEXT, 10000, 0);

    g_categoryCombo = CreateWindowExW(
        WS_EX_CLIENTEDGE | WS_EX_RTLREADING,
        L"COMBOBOX",
        L"",
        // Deliberately omit CBS_NOINTEGRALHEIGHT: let Windows keep complete
        // rows visible instead of clipping a row to the available list height.
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL,
        0, 0, CATEGORY_COMBO_WIDTH, CATEGORY_COMBO_WINDOW_HEIGHT,
        window,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_CATEGORY_COMBO)),
        g_instance,
        NULL);

    if (g_categoryCombo && g_font)
        SendMessageW(g_categoryCombo, WM_SETFONT, reinterpret_cast<WPARAM>(g_font), TRUE);

    const wchar_t* categories[5] = {
        L"الف ـ در صورت انجام ندادن، پیامد شدید",
        L"ب ـ انجام آن سودمند است؛ بدون پیامد شدید",
        L"ج ـ در صورت انجام ندادن، پیامد جزئی",
        L"د ـ کم‌اهمیت و مناسب واگذاری به دیگران",
        L"هـ ـ بدون ضرورت یا اهمیت"
    };

    if (g_categoryCombo)
    {
        for (int i = 0; i < 5; ++i)
        {
            const LRESULT added = SendMessageW(
                g_categoryCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(categories[i]));
            if (added == CB_ERR || added == CB_ERRSPACE)
            {
                MessageBoxW(window, L"افزودن گزینه‌های دسته‌بندی ناموفق بود.",
                            L"خطای دسته‌بندی", MB_OK | MB_ICONERROR);
                break;
            }
        }

        // Five categories, each with a 32px row. Set item metrics first, then
        // ask Windows to display at least five rows in the drop-down list.
        SendMessageW(g_categoryCombo, CB_SETITEMHEIGHT, static_cast<WPARAM>(-1), 34);
        SendMessageW(g_categoryCombo, CB_SETITEMHEIGHT, 0, 32);
        SendMessageW(g_categoryCombo, CB_SETMINVISIBLE, 5, 0);
        SendMessageW(g_categoryCombo, CB_SETDROPPEDWIDTH, 370, 0);
        SendMessageW(g_categoryCombo, CB_SETCURSEL, 0, 0);
    }

    g_addButton = CreateActionButton(window, L"افزودن +", ID_ADD);

    g_list = CreateWindowExW(
        WS_EX_CLIENTEDGE | WS_EX_RTLREADING,
        WC_LISTVIEWW,
        L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_SHOWSELALWAYS,
        0, 0, 10, 10,
        window,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_LIST)),
        g_instance,
        NULL);

    ListView_SetExtendedListViewStyle(
        g_list,
        LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
    ListView_SetBkColor(g_list, RGB(255, 255, 255));
    ListView_SetTextBkColor(g_list, RGB(255, 255, 255));
    ListView_SetTextColor(g_list, COLOR_TEXT);

    LVCOLUMNW column;
    ZeroMemory(&column, sizeof(column));
    column.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
    // Center the task-title column. Its cells are custom-drawn below, while
    // this format also keeps the native column metadata consistent.
    column.fmt = LVCFMT_CENTER;
    column.pszText = const_cast<LPWSTR>(L"عنوان کار");
    column.cx = 600;
    ListView_InsertColumn(g_list, 0, &column);

    // Keep the category column right-aligned as before.
    column.fmt = LVCFMT_RIGHT;
    column.pszText = const_cast<LPWSTR>(L"دسته‌بندی");
    column.cx = 105;
    ListView_InsertColumn(g_list, 1, &column);

    // Center the task-title header and preserve the category header alignment.
    HWND headerControl = ListView_GetHeader(g_list);
    if (headerControl)
    {
        for (int columnIndex = 0; columnIndex < 2; ++columnIndex)
        {
            HDITEMW headerItem;
            ZeroMemory(&headerItem, sizeof(headerItem));
            headerItem.mask = HDI_FORMAT;
            if (SendMessageW(headerControl, HDM_GETITEMW,
                             static_cast<WPARAM>(columnIndex),
                             reinterpret_cast<LPARAM>(&headerItem)))
            {
                if (columnIndex == 0)
                {
                    headerItem.fmt &= ~HDF_RIGHT;
                    headerItem.fmt |= HDF_CENTER;
                }
                else
                {
                    headerItem.fmt &= ~HDF_CENTER;
                    headerItem.fmt |= HDF_RIGHT;
                }
                SendMessageW(headerControl, HDM_SETITEMW,
                             static_cast<WPARAM>(columnIndex),
                             reinterpret_cast<LPARAM>(&headerItem));
            }
        }
    }

    g_deleteButton = CreateActionButton(window, L"حذف کار انتخاب‌شده", ID_DELETE);
    g_saveButton = CreateActionButton(window, L"ذخیره", ID_SAVE);
    g_settingsButton = CreateActionButton(window, L"", ID_SETTINGS);
    g_aboutButton = CreateActionButton(window, L"", ID_ABOUT);
    g_statusLabel = CreateLabel(window, L"", SS_RIGHT, 207);

    HWND controls[] = {
        g_taskEdit, g_list
    };

    for (size_t i = 0; i < sizeof(controls) / sizeof(controls[0]); ++i)
    {
        if (controls[i] && g_font)
            SendMessageW(controls[i], WM_SETFONT, reinterpret_cast<WPARAM>(g_font), TRUE);
    }

    if (g_titleLabel)
        SendMessageW(g_titleLabel, WM_SETFONT, reinterpret_cast<WPARAM>(g_boldFont), TRUE);
    if (g_listCaption)
        SendMessageW(g_listCaption, WM_SETFONT, reinterpret_cast<WPARAM>(g_boldFont), TRUE);

    LayoutControls(window);
}

static LRESULT HandleListCustomDraw(LPARAM lParam)
{
    LPNMLVCUSTOMDRAW draw = reinterpret_cast<LPNMLVCUSTOMDRAW>(lParam);

    if (draw->nmcd.dwDrawStage == CDDS_PREPAINT)
        return CDRF_NOTIFYITEMDRAW;

    // Draw the complete row ourselves at ITEMPREPAINT instead of depending on
    // SUBITEM notifications. This is especially important for column zero:
    // the ListView's native first-column renderer behaves differently from
    // its other report columns, and could leave Persian task titles invisible.
    if (draw->nmcd.dwDrawStage == CDDS_ITEMPREPAINT)
    {
        const int itemIndex = static_cast<int>(draw->nmcd.dwItemSpec);
        if (itemIndex < 0 || static_cast<size_t>(itemIndex) >= g_tasks.size())
            return CDRF_DODEFAULT;

        wchar_t storedTitle[10001] = { 0 };
        ListView_GetItemText(g_list, itemIndex, 0, storedTitle, 10001);
        const wchar_t* titleText = storedTitle[0] != L'\0'
            ? storedTitle : g_tasks[itemIndex].title.c_str();

        const UINT itemState = ListView_GetItemState(
            g_list, itemIndex, LVIS_SELECTED | LVIS_FOCUSED);
        const bool selected = (itemState & LVIS_SELECTED) != 0;
        HDC dc = draw->nmcd.hdc;
        const int savedDC = SaveDC(dc);

        if (g_font)
            SelectObject(dc, g_font);
        SetBkMode(dc, TRANSPARENT);

        for (int column = 0; column < 2; ++column)
        {
            RECT cellRect;
            if (!ListView_GetSubItemRect(g_list, itemIndex, column,
                                         LVIR_BOUNDS, &cellRect))
                continue;

            COLORREF background = RGB(255, 255, 255);
            COLORREF foreground = COLOR_TEXT;
            if (selected)
            {
                background = GetSysColor(COLOR_HIGHLIGHT);
                foreground = GetSysColor(COLOR_HIGHLIGHTTEXT);
            }
            else if (column == 1)
            {
                background = CategoryBackColor(g_tasks[itemIndex].category);
                foreground = CategoryTextColor(g_tasks[itemIndex].category);
            }

            HBRUSH backgroundBrush = CreateSolidBrush(background);
            FillRect(dc, &cellRect, backgroundBrush);
            DeleteObject(backgroundBrush);

            HPEN gridPen = CreatePen(PS_SOLID, 1, RGB(225, 233, 239));
            HGDIOBJ oldPen = SelectObject(dc, gridPen);
            MoveToEx(dc, cellRect.left, cellRect.bottom - 1, NULL);
            LineTo(dc, cellRect.right, cellRect.bottom - 1);
            MoveToEx(dc, cellRect.right - 1, cellRect.top, NULL);
            LineTo(dc, cellRect.right - 1, cellRect.bottom);
            SelectObject(dc, oldPen);
            DeleteObject(gridPen);

            RECT textRect = cellRect;
            // Keep a small, symmetric inset so long titles get as much of
            // the available column width as possible.
            textRect.left += (column == 0) ? 4 : 8;
            textRect.right -= (column == 0) ? 4 : 10;
            textRect.top += 1;
            textRect.bottom -= 1;

            const wchar_t* cellText = (column == 0)
                ? titleText
                : CategoryDisplayName(g_tasks[itemIndex].category);

            SetTextColor(dc, foreground);
            const UINT textFlags = column == 0
                ? (DT_CENTER | DT_RTLREADING | DT_VCENTER | DT_SINGLELINE |
                   DT_END_ELLIPSIS | DT_NOPREFIX)
                : (DT_RIGHT | DT_RTLREADING | DT_VCENTER | DT_SINGLELINE |
                   DT_END_ELLIPSIS | DT_NOPREFIX);
            DrawTextW(dc, cellText, -1, &textRect, textFlags);
        }

        if (selected && (itemState & LVIS_FOCUSED))
        {
            RECT focusRect;
            if (ListView_GetItemRect(g_list, itemIndex, &focusRect, LVIR_BOUNDS))
            {
                InflateRect(&focusRect, -3, -2);
                DrawFocusRect(dc, &focusRect);
            }
        }

        if (savedDC != 0)
            RestoreDC(dc, savedDC);

        return CDRF_SKIPDEFAULT;
    }

    return CDRF_DODEFAULT;
}

static void UpdateStartupSettingsUi()
{
    if (g_startupStateLabel)
    {
        const wchar_t* stateText = !g_startupStateKnown
            ? L"وضعیت نامشخص"
            : (g_startupEnabled
                ? L"روشن — اجرا هنگام ورود"
                : L"خاموش — اجرا غیرفعال");
        SetWindowTextW(g_startupStateLabel, stateText);
    }

    if (g_startupToggle)
    {
        EnableWindow(g_startupToggle, g_startupStateKnown ? TRUE : FALSE);
        InvalidateRect(g_startupToggle, NULL, TRUE);
    }
}

static void DrawStartupToggle(const DRAWITEMSTRUCT* item)
{
    if (!item || item->CtlType != ODT_BUTTON || item->CtlID != ID_STARTUP_TOGGLE)
        return;

    const bool enabled = g_startupStateKnown && g_startupEnabled;
    const bool disabled = (item->itemState & ODS_DISABLED) != 0;
    COLORREF trackColor = enabled ? COLOR_GREEN : RGB(158, 171, 181);
    if (disabled)
        trackColor = RGB(190, 198, 204);

    RECT track = item->rcItem;
    InflateRect(&track, -2, -3);
    HBRUSH trackBrush = CreateSolidBrush(trackColor);
    HPEN trackPen = CreatePen(PS_SOLID, 1, trackColor);
    HGDIOBJ oldBrush = SelectObject(item->hDC, trackBrush);
    HGDIOBJ oldPen = SelectObject(item->hDC, trackPen);
    RoundRect(item->hDC, track.left, track.top, track.right, track.bottom, 24, 24);
    SelectObject(item->hDC, oldBrush);
    SelectObject(item->hDC, oldPen);
    DeleteObject(trackBrush);
    DeleteObject(trackPen);

    const int centerY = (track.top + track.bottom) / 2;
    const int knobX = enabled ? track.right - 18 : track.left + 18;
    HBRUSH knobBrush = CreateSolidBrush(RGB(255, 255, 255));
    HPEN knobPen = CreatePen(PS_SOLID, 1, RGB(245, 248, 250));
    oldBrush = SelectObject(item->hDC, knobBrush);
    oldPen = SelectObject(item->hDC, knobPen);
    Ellipse(item->hDC, knobX - 14, centerY - 14, knobX + 14, centerY + 14);
    SelectObject(item->hDC, oldBrush);
    SelectObject(item->hDC, oldPen);
    DeleteObject(knobBrush);
    DeleteObject(knobPen);

    SetBkMode(item->hDC, TRANSPARENT);
    SetTextColor(item->hDC, RGB(255, 255, 255));
    RECT textRect = track;
    if (enabled)
        textRect.right -= 30;
    else
        textRect.left += 30;
    DrawTextW(item->hDC, enabled ? L"روشن" : L"خاموش", -1,
              &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_RTLREADING);

    if (item->itemState & ODS_FOCUS)
    {
        RECT focusRect = item->rcItem;
        InflateRect(&focusRect, -4, -4);
        DrawFocusRect(item->hDC, &focusRect);
    }
}

static LRESULT CALLBACK SettingsWindowProc(
    HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
    {
        g_settingsHeading = CreateWindowExW(
            WS_EX_RTLREADING, L"STATIC", L"تنظیمات",
            WS_CHILD | WS_VISIBLE | SS_RIGHT,
            24, 14, 390, 30, window, NULL, g_instance, NULL);
        g_settingsSubheading = CreateWindowExW(
            WS_EX_RTLREADING, L"STATIC", L"شخصی‌سازی اجرای برنامه",
            WS_CHILD | WS_VISIBLE | SS_RIGHT,
            24, 44, 390, 20, window, NULL, g_instance, NULL);
        g_settingsOptionLabel = CreateWindowExW(
            WS_EX_RTLREADING, L"STATIC", L"اجرای خودکار هنگام ورود به ویندوز",
            WS_CHILD | WS_VISIBLE | SS_RIGHT,
            34, 101, 370, 25, window, NULL, g_instance, NULL);
        g_startupStateLabel = CreateWindowExW(
            WS_EX_RTLREADING, L"STATIC", L"",
            WS_CHILD | WS_VISIBLE | SS_RIGHT,
            34, 141, 250, 26, window, NULL, g_instance, NULL);
        g_settingsHintLabel = CreateWindowExW(
            WS_EX_RTLREADING, L"STATIC",
            L"این تنظیم فقط برای حساب کاربری فعلی ویندوز اعمال می‌شود.",
            WS_CHILD | WS_VISIBLE | SS_RIGHT,
            30, 198, 390, 22, window, NULL, g_instance, NULL);
        g_startupToggle = CreateWindowExW(
            0, L"BUTTON", L"",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
            300, 126, 104, 44, window,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_STARTUP_TOGGLE)),
            g_instance, NULL);

        HWND labels[] = {
            g_settingsHeading, g_settingsSubheading,
            g_settingsOptionLabel, g_startupStateLabel, g_settingsHintLabel
        };
        for (size_t i = 0; i < sizeof(labels) / sizeof(labels[0]); ++i)
        {
            if (labels[i] && g_font)
                SendMessageW(labels[i], WM_SETFONT,
                             reinterpret_cast<WPARAM>(g_font), TRUE);
        }
        if (g_settingsHeading && g_boldFont)
            SendMessageW(g_settingsHeading, WM_SETFONT,
                         reinterpret_cast<WPARAM>(g_boldFont), TRUE);

        g_startupLastError = ReadStartupState(&g_startupEnabled);
        g_startupStateKnown = (g_startupLastError == ERROR_SUCCESS);
        UpdateStartupSettingsUi();
        return 0;
    }

    case WM_ERASEBKGND:
    {
        RECT rect;
        GetClientRect(window, &rect);
        FillRect(reinterpret_cast<HDC>(wParam), &rect, g_backgroundBrush);
        return 1;
    }

    case WM_PAINT:
    {
        PAINTSTRUCT paint;
        HDC dc = BeginPaint(window, &paint);
        RECT client;
        GetClientRect(window, &client);

        RECT header = { 0, 0, client.right, 78 };
        FillRect(dc, &header, g_headerBrush);
        HBRUSH accentBrush = CreateSolidBrush(COLOR_GREEN);
        RECT accent = { 0, 0, 6, 78 };
        FillRect(dc, &accent, accentBrush);
        DeleteObject(accentBrush);

        HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(225, 233, 239));
        HGDIOBJ oldPen = SelectObject(dc, borderPen);
        HGDIOBJ oldBrush = SelectObject(dc, g_whiteBrush);
        RoundRect(dc, 18, 91, client.right - 18, client.bottom - 18, 12, 12);
        SelectObject(dc, oldBrush);
        SelectObject(dc, oldPen);
        DeleteObject(borderPen);
        EndPaint(window, &paint);
        return 0;
    }

    case WM_CTLCOLORSTATIC:
    {
        HDC dc = reinterpret_cast<HDC>(wParam);
        HWND control = reinterpret_cast<HWND>(lParam);
        SetBkMode(dc, TRANSPARENT);

        if (control == g_settingsHeading)
        {
            SetTextColor(dc, RGB(255, 255, 255));
            return reinterpret_cast<LRESULT>(g_headerBrush);
        }
        if (control == g_settingsSubheading)
        {
            SetTextColor(dc, RGB(205, 222, 234));
            return reinterpret_cast<LRESULT>(g_headerBrush);
        }
        if (control == g_settingsOptionLabel)
        {
            SetTextColor(dc, COLOR_TEXT);
            return reinterpret_cast<LRESULT>(g_whiteBrush);
        }
        if (control == g_startupStateLabel)
        {
            SetTextColor(dc, g_startupStateKnown
                ? (g_startupEnabled ? COLOR_GREEN : COLOR_MUTED)
                : COLOR_RED);
            return reinterpret_cast<LRESULT>(g_whiteBrush);
        }
        if (control == g_settingsHintLabel)
        {
            SetTextColor(dc, COLOR_MUTED);
            return reinterpret_cast<LRESULT>(g_whiteBrush);
        }
        break;
    }

    case WM_DRAWITEM:
        DrawStartupToggle(reinterpret_cast<const DRAWITEMSTRUCT*>(lParam));
        return TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == ID_STARTUP_TOGGLE && HIWORD(wParam) == BN_CLICKED)
        {
            const LONG writeResult = SetStartupState(!g_startupEnabled);
            if (writeResult != ERROR_SUCCESS)
            {
                ShowStartupError(window, writeResult);
                return 0;
            }

            bool verifiedState = false;
            const LONG readResult = ReadStartupState(&verifiedState);
            if (readResult != ERROR_SUCCESS || verifiedState != !g_startupEnabled)
            {
                ShowStartupError(window,
                    readResult != ERROR_SUCCESS ? readResult : ERROR_INVALID_DATA);
                return 0;
            }

            g_startupEnabled = verifiedState;
            g_startupStateKnown = true;
            g_startupLastError = ERROR_SUCCESS;
            UpdateStartupSettingsUi();
            return 0;
        }
        break;

    case WM_CLOSE:
        DestroyWindow(window);
        return 0;

    case WM_DESTROY:
        if (g_settingsWindow == window)
            g_settingsWindow = NULL;
        g_settingsHeading = NULL;
        g_settingsSubheading = NULL;
        g_settingsOptionLabel = NULL;
        g_startupStateLabel = NULL;
        g_settingsHintLabel = NULL;
        g_startupToggle = NULL;
        return 0;
    }

    return DefWindowProcW(window, message, wParam, lParam);
}

static void OpenSettingsWindow(HWND owner)
{
    if (g_settingsWindow && IsWindow(g_settingsWindow))
    {
        ShowWindow(g_settingsWindow, SW_SHOWNORMAL);
        SetForegroundWindow(g_settingsWindow);
        return;
    }

    RECT ownerRect;
    GetWindowRect(owner, &ownerRect);
    const int windowWidth = 460;
    const int windowHeight = 285;
    const int x = ownerRect.left + ((ownerRect.right - ownerRect.left) - windowWidth) / 2;
    const int y = ownerRect.top + ((ownerRect.bottom - ownerRect.top) - windowHeight) / 2;

    g_settingsWindow = CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_RTLREADING,
        L"NativeTaskManagerSettings",
        L"تنظیمات برنامه",
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        x, y, windowWidth, windowHeight,
        owner, NULL, g_instance, NULL);

    if (!g_settingsWindow)
    {
        MessageBoxW(owner, L"باز کردن پنجره تنظیمات ناموفق بود.",
                    L"خطا", MB_OK | MB_ICONERROR);
        return;
    }

    ShowWindow(g_settingsWindow, SW_SHOWNORMAL);
    UpdateWindow(g_settingsWindow);
    if (!g_startupStateKnown && g_startupLastError != ERROR_SUCCESS)
        ShowStartupError(g_settingsWindow, g_startupLastError);
}

static void LayoutAboutControls(HWND window)
{
    RECT client;
    GetClientRect(window, &client);
    const int width = client.right - client.left;
    const int height = client.bottom - client.top;
    const int pad = 30;
    const int contentWidth = width - 2 * pad;

    if (g_aboutTitleLabel)
        MoveWindow(g_aboutTitleLabel, pad, 16, contentWidth, 35, TRUE);
    if (g_aboutSubtitleLabel)
        MoveWindow(g_aboutSubtitleLabel, pad, 51, contentWidth, 24, TRUE);
    if (g_aboutDescriptionLabel)
        MoveWindow(g_aboutDescriptionLabel, pad + 12, 143, contentWidth - 24, 66, TRUE);
    if (g_aboutVersionLabel)
        MoveWindow(g_aboutVersionLabel, pad + 12, 211, contentWidth - 24, 24, TRUE);

    const int buttonGap = 14;
    const int buttonWidth = (contentWidth - buttonGap) / 2;
    const int buttonY = 252;
    if (g_aboutGithubButton)
        MoveWindow(g_aboutGithubButton, pad, buttonY, buttonWidth, 58, TRUE);
    if (g_aboutTelegramButton)
        MoveWindow(g_aboutTelegramButton, pad + buttonWidth + buttonGap,
                   buttonY, buttonWidth, 58, TRUE);
    if (g_aboutFooterLabel)
        MoveWindow(g_aboutFooterLabel, pad, height - 48, contentWidth, 24, TRUE);
}

static void DrawAboutLinkButton(const DRAWITEMSTRUCT* item)
{
    if (!item || item->CtlType != ODT_BUTTON)
        return;

    const bool github = item->CtlID == ID_ABOUT_GITHUB;
    const bool pressed = (item->itemState & ODS_SELECTED) != 0;
    COLORREF background = github ? RGB(235, 242, 249) : RGB(231, 245, 250);
    if (pressed)
        background = github ? RGB(219, 231, 242) : RGB(214, 237, 246);
    const COLORREF accent = github ? COLOR_NAVY : RGB(34, 158, 217);

    HBRUSH brush = CreateSolidBrush(background);
    HPEN pen = CreatePen(PS_SOLID, 1, github ? RGB(210, 223, 235) : RGB(202, 229, 239));
    HGDIOBJ oldBrush = SelectObject(item->hDC, brush);
    HGDIOBJ oldPen = SelectObject(item->hDC, pen);
    RoundRect(item->hDC, item->rcItem.left + 1, item->rcItem.top + 1,
              item->rcItem.right - 1, item->rcItem.bottom - 1, 12, 12);
    SelectObject(item->hDC, oldBrush);
    SelectObject(item->hDC, oldPen);
    DeleteObject(brush);
    DeleteObject(pen);

    const int centerY = (item->rcItem.top + item->rcItem.bottom) / 2;
    const int circleX = item->rcItem.left + 28;
    HBRUSH circleBrush = CreateSolidBrush(accent);
    HGDIOBJ oldCircleBrush = SelectObject(item->hDC, circleBrush);
    HGDIOBJ oldCirclePen = SelectObject(item->hDC, GetStockObject(NULL_PEN));
    Ellipse(item->hDC, circleX - 15, centerY - 15, circleX + 15, centerY + 15);
    SelectObject(item->hDC, oldCirclePen);
    SelectObject(item->hDC, oldCircleBrush);
    DeleteObject(circleBrush);

    SetBkMode(item->hDC, TRANSPARENT);
    SetTextColor(item->hDC, RGB(255, 255, 255));
    RECT badgeRect = { circleX - 13, centerY - 11, circleX + 13, centerY + 11 };
    DrawTextW(item->hDC, github ? L"GH" : L"TG", -1, &badgeRect,
              DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    wchar_t text[128] = { 0 };
    GetWindowTextW(item->hwndItem, text, 127);
    SetTextColor(item->hDC, COLOR_NAVY);
    RECT textRect = item->rcItem;
    textRect.left += 54;
    textRect.right -= 10;
    DrawTextW(item->hDC, text, -1, &textRect,
              DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_RTLREADING | DT_END_ELLIPSIS);

    if (item->itemState & ODS_FOCUS)
    {
        RECT focusRect = item->rcItem;
        InflateRect(&focusRect, -5, -5);
        DrawFocusRect(item->hDC, &focusRect);
    }
}

static void OpenExternalLink(HWND owner, const wchar_t* url)
{
    HINSTANCE result = ShellExecuteW(owner, L"open", url, NULL, NULL, SW_SHOWNORMAL);
    if (reinterpret_cast<INT_PTR>(result) <= 32)
    {
        MessageBoxW(owner,
                    L"باز کردن پیوند ناموفق بود. مرورگر پیش‌فرض ویندوز را بررسی کنید.",
                    L"خطا در باز کردن پیوند", MB_OK | MB_ICONERROR);
    }
}

static LRESULT CALLBACK AboutWindowProc(
    HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        g_aboutWindow = window;
        g_aboutTitleLabel = CreateLabel(window, L"مدیریت کارها", SS_RIGHT, 711);
        g_aboutSubtitleLabel = CreateLabel(
            window, L"ابزاری سبک برای نظم‌دادن به کارهای روزمره", SS_RIGHT, 712);
        g_aboutDescriptionLabel = CreateLabel(
            window,
            L"یک برنامهٔ رایگان و متن‌باز برای دسته‌بندی و اولویت‌بندی کارها. "
            L"برای مشاهدهٔ کد منبع، پیشنهاد تغییرات یا دنبال‌کردن اخبار پروژه، از پیوندهای زیر استفاده کنید.",
            SS_RIGHT, 713);
        g_aboutVersionLabel = CreateLabel(
            window, L"نسخهٔ ۱.۳.۰  •  C++ و Native Win32 API", SS_RIGHT, 714);
        g_aboutFooterLabel = CreateLabel(
            window, L"از حمایت و مشارکت شما در توسعهٔ پروژه سپاسگزارم.", SS_CENTER, 715);

        g_aboutGithubButton = CreateWindowExW(
            0, L"BUTTON", L"مشاهدهٔ مخزن GitHub",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
            0, 0, 10, 10, window,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_ABOUT_GITHUB)),
            g_instance, NULL);
        g_aboutTelegramButton = CreateWindowExW(
            0, L"BUTTON", L"ورود به کانال تلگرام",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
            0, 0, 10, 10, window,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_ABOUT_TELEGRAM)),
            g_instance, NULL);

        if (g_aboutTitleLabel && g_boldFont)
            SendMessageW(g_aboutTitleLabel, WM_SETFONT, reinterpret_cast<WPARAM>(g_boldFont), TRUE);
        if (g_aboutVersionLabel && g_font)
            SendMessageW(g_aboutVersionLabel, WM_SETFONT, reinterpret_cast<WPARAM>(g_font), TRUE);
        if (g_aboutFooterLabel && g_font)
            SendMessageW(g_aboutFooterLabel, WM_SETFONT, reinterpret_cast<WPARAM>(g_font), TRUE);
        if (g_aboutGithubButton && g_boldFont)
            SendMessageW(g_aboutGithubButton, WM_SETFONT, reinterpret_cast<WPARAM>(g_boldFont), TRUE);
        if (g_aboutTelegramButton && g_boldFont)
            SendMessageW(g_aboutTelegramButton, WM_SETFONT, reinterpret_cast<WPARAM>(g_boldFont), TRUE);

        LayoutAboutControls(window);
        return 0;

    case WM_SIZE:
        LayoutAboutControls(window);
        return 0;

    case WM_ERASEBKGND:
    {
        RECT rect;
        GetClientRect(window, &rect);
        FillRect(reinterpret_cast<HDC>(wParam), &rect, g_backgroundBrush);
        return 1;
    }

    case WM_PAINT:
    {
        PAINTSTRUCT paint;
        HDC dc = BeginPaint(window, &paint);
        RECT client;
        GetClientRect(window, &client);
        RECT header = { 0, 0, client.right, 104 };
        FillRect(dc, &header, g_headerBrush);
        HBRUSH accentBrush = CreateSolidBrush(COLOR_GREEN);
        RECT accent = { 0, 0, 6, 104 };
        FillRect(dc, &accent, accentBrush);
        DeleteObject(accentBrush);

        HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(225, 233, 239));
        HGDIOBJ oldPen = SelectObject(dc, borderPen);
        HGDIOBJ oldBrush = SelectObject(dc, g_whiteBrush);
        RoundRect(dc, 18, 120, client.right - 18, client.bottom - 16, 14, 14);
        SelectObject(dc, oldBrush);
        SelectObject(dc, oldPen);
        DeleteObject(borderPen);
        EndPaint(window, &paint);
        return 0;
    }

    case WM_CTLCOLORSTATIC:
    {
        HDC dc = reinterpret_cast<HDC>(wParam);
        HWND control = reinterpret_cast<HWND>(lParam);
        SetBkMode(dc, TRANSPARENT);
        if (control == g_aboutTitleLabel)
        {
            SetTextColor(dc, RGB(255, 255, 255));
            return reinterpret_cast<LRESULT>(g_headerBrush);
        }
        if (control == g_aboutSubtitleLabel)
        {
            SetTextColor(dc, RGB(205, 222, 234));
            return reinterpret_cast<LRESULT>(g_headerBrush);
        }
        if (control == g_aboutVersionLabel)
            SetTextColor(dc, COLOR_GREEN);
        else if (control == g_aboutFooterLabel)
            SetTextColor(dc, COLOR_MUTED);
        else
            SetTextColor(dc, COLOR_TEXT);
        return reinterpret_cast<LRESULT>(g_whiteBrush);
    }

    case WM_DRAWITEM:
    {
        const DRAWITEMSTRUCT* item = reinterpret_cast<const DRAWITEMSTRUCT*>(lParam);
        if (item && (item->CtlID == ID_ABOUT_GITHUB || item->CtlID == ID_ABOUT_TELEGRAM))
        {
            DrawAboutLinkButton(item);
            return TRUE;
        }
        break;
    }

    case WM_COMMAND:
        if (HIWORD(wParam) == BN_CLICKED)
        {
            if (LOWORD(wParam) == ID_ABOUT_GITHUB)
                OpenExternalLink(window, L"https://github.com/AloneMaster7");
            else if (LOWORD(wParam) == ID_ABOUT_TELEGRAM)
                OpenExternalLink(window, L"https://t.me/AISpecialists_ir");
            return 0;
        }
        break;

    case WM_CLOSE:
        DestroyWindow(window);
        return 0;

    case WM_DESTROY:
        if (g_aboutWindow == window)
            g_aboutWindow = NULL;
        g_aboutTitleLabel = NULL;
        g_aboutSubtitleLabel = NULL;
        g_aboutDescriptionLabel = NULL;
        g_aboutVersionLabel = NULL;
        g_aboutFooterLabel = NULL;
        g_aboutGithubButton = NULL;
        g_aboutTelegramButton = NULL;
        return 0;
    }

    return DefWindowProcW(window, message, wParam, lParam);
}

static void OpenAboutWindow(HWND owner)
{
    if (g_aboutWindow && IsWindow(g_aboutWindow))
    {
        ShowWindow(g_aboutWindow, SW_SHOWNORMAL);
        SetForegroundWindow(g_aboutWindow);
        return;
    }

    RECT ownerRect;
    GetWindowRect(owner, &ownerRect);
    const int windowWidth = 560;
    const int windowHeight = 470;
    const int x = ownerRect.left + ((ownerRect.right - ownerRect.left) - windowWidth) / 2;
    const int y = ownerRect.top + ((ownerRect.bottom - ownerRect.top) - windowHeight) / 2;

    HWND aboutWindow = CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_RTLREADING,
        L"NativeTaskManagerAbout",
        L"درباره برنامه",
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        x, y, windowWidth, windowHeight,
        owner, NULL, g_instance, NULL);

    if (!aboutWindow)
    {
        MessageBoxW(owner, L"باز کردن پنجرهٔ دربارهٔ برنامه ناموفق بود.",
                    L"خطا", MB_OK | MB_ICONERROR);
        return;
    }

    if (g_appIconBig)
        SendMessageW(aboutWindow, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(g_appIconBig));
    if (g_appIconSmall)
        SendMessageW(aboutWindow, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(g_appIconSmall));
    ShowWindow(aboutWindow, SW_SHOWNORMAL);
    UpdateWindow(aboutWindow);
}

static bool ConfirmExitIfDirty(HWND owner)
{
    if (!g_dirty)
        return true;

    const int answer = MessageBoxW(
        owner,
        L"تغییرات ذخیره نشده‌اند.\nمی‌خواهید پیش از خروج ذخیره شوند؟",
        L"خروج از برنامه",
        MB_YESNOCANCEL | MB_ICONQUESTION | MB_DEFBUTTON1);

    if (answer == IDCANCEL)
        return false;
    if (answer == IDYES && !SaveTasks())
    {
        MessageBoxW(
            owner,
            L"ذخیره‌سازی ناموفق بود. برنامه بسته نمی‌شود تا اطلاعات از دست نرود.",
            L"خطا در ذخیره‌سازی", MB_OK | MB_ICONERROR);
        return false;
    }
    return true;
}

static LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (g_taskbarCreatedMessage != 0 && message == g_taskbarCreatedMessage)
    {
        // Explorer may restart and discard notification-area icons.
        g_trayIconAdded = false;
        AddTrayIcon(window);
        return 0;
    }

    switch (message)
    {
    case WM_CREATE:
        g_window = window;
        g_font = CreateFontW(
            -17, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Tahoma");
        g_boldFont = CreateFontW(
            -19, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Tahoma");

        g_backgroundBrush = CreateSolidBrush(COLOR_BG);
        g_headerBrush = CreateSolidBrush(COLOR_NAVY);
        g_whiteBrush = CreateSolidBrush(RGB(255, 255, 255));

        CreateControls(window);
        BuildDataPath();
        if (!LoadTasks())
            g_loadError = true;
        RefreshList();
        AddTrayIcon(window);
        return 0;

    case WM_SIZE:
        if (wParam == SIZE_MAXIMIZED)
            g_mainWasMaximized = true;
        else if (wParam == SIZE_RESTORED)
            g_mainWasMaximized = false;

        if (wParam == SIZE_MINIMIZED && g_trayIconAdded)
        {
            ShowWindow(window, SW_HIDE);
            ShowTrayNotice(window);
            return 0;
        }
        LayoutControls(window);
        return 0;

    case WM_TRAY_CALLBACK:
        if (lParam == WM_LBUTTONUP || lParam == WM_LBUTTONDBLCLK)
            ShowMainWindow(window);
        else if (lParam == WM_RBUTTONUP || lParam == WM_CONTEXTMENU)
            ShowTrayContextMenu(window);
        return 0;

    case WM_GETMINMAXINFO:
    {
        MINMAXINFO* info = reinterpret_cast<MINMAXINFO*>(lParam);
        info->ptMinTrackSize.x = 800;
        info->ptMinTrackSize.y = 600;
        return 0;
    }

    case WM_ERASEBKGND:
    {
        RECT rect;
        GetClientRect(window, &rect);
        FillRect(reinterpret_cast<HDC>(wParam), &rect, g_backgroundBrush);
        return 1;
    }

    case WM_PAINT:
    {
        PAINTSTRUCT paint;
        HDC dc = BeginPaint(window, &paint);
        RECT client;
        GetClientRect(window, &client);

        RECT header = { 0, 0, client.right, 88 };
        FillRect(dc, &header, g_headerBrush);

        HBRUSH accentBrush = CreateSolidBrush(COLOR_GREEN);
        RECT accent = { 0, 0, 6, 88 };
        FillRect(dc, &accent, accentBrush);
        DeleteObject(accentBrush);

        HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(225, 233, 239));
        HGDIOBJ oldPen = SelectObject(dc, borderPen);
        HGDIOBJ oldBrush = SelectObject(dc, g_whiteBrush);

        RoundRect(dc, 18, 98, client.right - 18, 179, 12, 12);
        RoundRect(dc, 18, 204, client.right - 18, client.bottom - 73, 12, 12);

        SelectObject(dc, oldBrush);
        SelectObject(dc, oldPen);
        DeleteObject(borderPen);
        EndPaint(window, &paint);
        return 0;
    }

    case WM_CTLCOLORSTATIC:
    {
        HDC dc = reinterpret_cast<HDC>(wParam);
        HWND control = reinterpret_cast<HWND>(lParam);
        SetBkMode(dc, TRANSPARENT);

        if (control == g_titleLabel)
        {
            SetTextColor(dc, RGB(255, 255, 255));
            return reinterpret_cast<LRESULT>(g_headerBrush);
        }
        if (control == g_subtitleLabel)
        {
            SetTextColor(dc, RGB(205, 222, 234));
            return reinterpret_cast<LRESULT>(g_headerBrush);
        }
        if (control == g_statusLabel)
            SetTextColor(dc, COLOR_MUTED);
        else if (control == g_listCaption)
            SetTextColor(dc, COLOR_TEXT);
        else
            SetTextColor(dc, COLOR_MUTED);

        return reinterpret_cast<LRESULT>(g_backgroundBrush);
    }

    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    {
        HDC dc = reinterpret_cast<HDC>(wParam);
        SetTextColor(dc, COLOR_TEXT);
        SetBkColor(dc, RGB(255, 255, 255));
        return reinterpret_cast<LRESULT>(g_whiteBrush);
    }

    case WM_DRAWITEM:
        DrawActionButton(reinterpret_cast<const DRAWITEMSTRUCT*>(lParam));
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case ID_ADD:
            if (HIWORD(wParam) == BN_CLICKED)
                AddTask();
            return 0;
        case ID_DELETE:
            if (HIWORD(wParam) == BN_CLICKED)
                DeleteSelectedTask();
            return 0;
        case ID_SAVE:
            if (HIWORD(wParam) == BN_CLICKED)
                SaveAndReport();
            return 0;
        case ID_SETTINGS:
            if (HIWORD(wParam) == BN_CLICKED)
                OpenSettingsWindow(window);
            return 0;
        case ID_ABOUT:
            if (HIWORD(wParam) == BN_CLICKED)
                OpenAboutWindow(window);
            return 0;
        case ID_TRAY_OPEN:
            ShowMainWindow(window);
            return 0;
        case ID_TRAY_ABOUT:
            ShowMainWindow(window);
            OpenAboutWindow(window);
            return 0;
        case ID_TRAY_EXIT:
            if (ConfirmExitIfDirty(window))
                DestroyWindow(window);
            return 0;
        default:
            break;
        }
        break;

    case WM_NOTIFY:
    {
        LPNMHDR header = reinterpret_cast<LPNMHDR>(lParam);
        if (header->idFrom == ID_LIST && header->code == NM_DBLCLK)
        {
            LPNMITEMACTIVATE activate = reinterpret_cast<LPNMITEMACTIVATE>(lParam);
            if (activate->iItem >= 0)
                OpenEditTaskWindow(activate->iItem);
            return 0;
        }
        if (header->idFrom == ID_LIST && header->code == NM_CUSTOMDRAW)
            return HandleListCustomDraw(lParam);
        if (header->idFrom == ID_LIST && header->code == LVN_ITEMCHANGED)
        {
            UpdateDeleteButton();
            return 0;
        }
        if (header->idFrom == ID_LIST && header->code == LVN_KEYDOWN)
        {
            LPNMLVKEYDOWN key = reinterpret_cast<LPNMLVKEYDOWN>(lParam);
            if (key->wVKey == VK_DELETE)
                DeleteSelectedTask();
            return 0;
        }
        break;
    }

    case WM_CLOSE:
        // The close button hides the main window but keeps the app running in
        // the notification area. Use the tray menu's Exit command to terminate.
        if (g_trayIconAdded)
        {
            ShowWindow(window, SW_HIDE);
            ShowTrayNotice(window);
            return 0;
        }
        if (ConfirmExitIfDirty(window))
            DestroyWindow(window);
        return 0;

    case WM_DESTROY:
        g_mainShuttingDown = true;
        RemoveTrayIcon(window);
        if (g_aboutWindow && IsWindow(g_aboutWindow))
            DestroyWindow(g_aboutWindow);
        if (g_editWindow && IsWindow(g_editWindow))
            DestroyWindow(g_editWindow);
        if (g_settingsWindow && IsWindow(g_settingsWindow))
            DestroyWindow(g_settingsWindow);
        if (g_font) DeleteObject(g_font);
        if (g_boldFont) DeleteObject(g_boldFont);
        if (g_backgroundBrush) DeleteObject(g_backgroundBrush);
        if (g_headerBrush) DeleteObject(g_headerBrush);
        if (g_whiteBrush) DeleteObject(g_whiteBrush);
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(window, message, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int showCommand)
{
    g_instance = instance;
    LoadApplicationIcons(instance);
    if (!g_appIconBig || !g_appIconSmall)
    {
        DestroyApplicationIcons();
        MessageBoxW(NULL,
                    L"بارگذاری آیکون icon.ico ناموفق بود. مطمئن شوید فایل icon.ico کنار فایل اجرایی قرار دارد و هنگام کامپایل به منابع برنامه اضافه شده است.",
                    L"خطا در بارگذاری آیکون", MB_OK | MB_ICONERROR);
        return 1;
    }
    g_taskbarCreatedMessage = RegisterWindowMessageW(L"TaskbarCreated");

    INITCOMMONCONTROLSEX commonControls;
    commonControls.dwSize = sizeof(commonControls);
    commonControls.dwICC = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&commonControls);

    WNDCLASSEXW windowClass;
    ZeroMemory(&windowClass, sizeof(windowClass));
    windowClass.cbSize = sizeof(windowClass);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursorW(NULL, IDC_ARROW);
    windowClass.hIcon = g_appIconBig;
    windowClass.hIconSm = g_appIconSmall;
    windowClass.hbrBackground = NULL;
    windowClass.lpszClassName = L"NativeTaskManagerWindow";

    if (!RegisterClassExW(&windowClass))
    {
        MessageBoxW(NULL, L"ثبت کلاس پنجره ناموفق بود.", L"خطا", MB_OK | MB_ICONERROR);
        DestroyApplicationIcons();
        return 1;
    }

    WNDCLASSEXW settingsClass;
    ZeroMemory(&settingsClass, sizeof(settingsClass));
    settingsClass.cbSize = sizeof(settingsClass);
    settingsClass.style = CS_HREDRAW | CS_VREDRAW;
    settingsClass.lpfnWndProc = SettingsWindowProc;
    settingsClass.hInstance = instance;
    settingsClass.hCursor = LoadCursorW(NULL, IDC_ARROW);
    settingsClass.hIcon = g_appIconBig;
    settingsClass.hIconSm = g_appIconSmall;
    settingsClass.hbrBackground = NULL;
    settingsClass.lpszClassName = L"NativeTaskManagerSettings";

    if (!RegisterClassExW(&settingsClass) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
    {
        MessageBoxW(NULL, L"ثبت کلاس پنجره تنظیمات ناموفق بود.", L"خطا", MB_OK | MB_ICONERROR);
        DestroyApplicationIcons();
        return 1;
    }

    WNDCLASSEXW editClass;
    ZeroMemory(&editClass, sizeof(editClass));
    editClass.cbSize = sizeof(editClass);
    editClass.style = CS_HREDRAW | CS_VREDRAW;
    editClass.lpfnWndProc = EditWindowProc;
    editClass.hInstance = instance;
    editClass.hCursor = LoadCursorW(NULL, IDC_ARROW);
    editClass.hIcon = g_appIconBig;
    editClass.hIconSm = g_appIconSmall;
    editClass.hbrBackground = NULL;
    editClass.lpszClassName = L"NativeTaskManagerEdit";

    if (!RegisterClassExW(&editClass) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
    {
        MessageBoxW(NULL, L"ثبت کلاس پنجره ویرایش ناموفق بود.", L"خطا", MB_OK | MB_ICONERROR);
        DestroyApplicationIcons();
        return 1;
    }

    WNDCLASSEXW aboutClass;
    ZeroMemory(&aboutClass, sizeof(aboutClass));
    aboutClass.cbSize = sizeof(aboutClass);
    aboutClass.style = CS_HREDRAW | CS_VREDRAW;
    aboutClass.lpfnWndProc = AboutWindowProc;
    aboutClass.hInstance = instance;
    aboutClass.hCursor = LoadCursorW(NULL, IDC_ARROW);
    aboutClass.hIcon = g_appIconBig;
    aboutClass.hIconSm = g_appIconSmall;
    aboutClass.hbrBackground = NULL;
    aboutClass.lpszClassName = L"NativeTaskManagerAbout";

    if (!RegisterClassExW(&aboutClass) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
    {
        MessageBoxW(NULL, L"ثبت کلاس پنجره دربارهٔ برنامه ناموفق بود.",
                    L"خطا", MB_OK | MB_ICONERROR);
        DestroyApplicationIcons();
        return 1;
    }

    // RTL is applied to text reading only. Do not use WS_EX_LAYOUTRTL here,
    // because it mirrors the positions and visual layout of all child controls.
    HWND window = CreateWindowExW(
        WS_EX_RTLREADING,
        windowClass.lpszClassName,
        L"مدیریت کارها",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        1050, 760,
        NULL, NULL, instance, NULL);

    if (!window)
    {
        MessageBoxW(NULL, L"ساخت پنجره برنامه ناموفق بود.", L"خطا", MB_OK | MB_ICONERROR);
        DestroyApplicationIcons();
        return 1;
    }

    SendMessageW(window, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(g_appIconBig));
    SendMessageW(window, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(g_appIconSmall));
    ShowWindow(window, showCommand);
    UpdateWindow(window);

    MSG message;
    while (GetMessageW(&message, NULL, 0, 0) > 0)
    {
        // Give the edit popup normal dialog keyboard behavior (Tab navigation
        // and default-button handling) while it is open.
        if (g_editWindow && IsDialogMessageW(g_editWindow, &message))
            continue;

        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    const int exitCode = static_cast<int>(message.wParam);
    DestroyApplicationIcons();
    return exitCode;
}
