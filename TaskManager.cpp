#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <commctrl.h>
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
        g_tasks.swap(loaded);

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

    const int selected = ListView_GetNextItem(g_list, -1, LVNI_SELECTED);
    EnableWindow(g_deleteButton, selected >= 0);
}

static void RefreshList()
{
    if (!g_list)
        return;

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
            const wchar_t* categoryText = CategoryDisplayName(g_tasks[i].category);
            ListView_SetItemText(g_list, row, 1, const_cast<LPWSTR>(categoryText));
        }
    }

    UpdateDeleteButton();
    UpdateStatus();
    InvalidateRect(g_list, NULL, TRUE);
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
    const int statusWidth = deleteX - pad - actionGap;

    MoveWindow(g_saveButton, saveX, buttonY, saveWidth, 38, TRUE);
    MoveWindow(g_deleteButton, deleteX, buttonY, deleteWidth, 38, TRUE);
    MoveWindow(g_statusLabel, pad, height - 53, statusWidth, 28, TRUE);

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
    g_tasks.insert(g_tasks.begin(), task);

    MarkDirty();
    RefreshList();
    SetWindowTextW(g_taskEdit, L"");
    SetFocus(g_taskEdit);
}

static void DeleteSelectedTask()
{
    const int selected = ListView_GetNextItem(g_list, -1, LVNI_SELECTED);
    if (selected < 0 || static_cast<size_t>(selected) >= g_tasks.size())
        return;

    const int answer = MessageBoxW(
        g_window,
        L"این کار حذف شود؟\nبا زدن دکمه ذخیره، حذف آن برای همیشه ثبت می‌شود.",
        L"حذف کار",
        MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);

    if (answer != IDYES)
        return;

    g_tasks.erase(g_tasks.begin() + selected);
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
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
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
    column.fmt = LVCFMT_RIGHT;
    column.pszText = const_cast<LPWSTR>(L"عنوان کار");
    column.cx = 600;
    ListView_InsertColumn(g_list, 0, &column);

    column.pszText = const_cast<LPWSTR>(L"دسته‌بندی");
    column.cx = 105;
    ListView_InsertColumn(g_list, 1, &column);

    g_deleteButton = CreateActionButton(window, L"حذف کار انتخاب‌شده", ID_DELETE);
    g_saveButton = CreateActionButton(window, L"ذخیره", ID_SAVE);
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

    if (draw->nmcd.dwDrawStage == CDDS_ITEMPREPAINT)
        return CDRF_NOTIFYSUBITEMDRAW;

    if (draw->nmcd.dwDrawStage == (CDDS_ITEMPREPAINT | CDDS_SUBITEM))
    {
        const size_t index = static_cast<size_t>(draw->nmcd.dwItemSpec);
        if (index < g_tasks.size() && !(draw->nmcd.uItemState & CDIS_SELECTED))
        {
            if (draw->iSubItem == 1)
            {
                draw->clrText = CategoryTextColor(g_tasks[index].category);
                draw->clrTextBk = CategoryBackColor(g_tasks[index].category);
            }
            else
            {
                draw->clrText = COLOR_TEXT;
                draw->clrTextBk = RGB(255, 255, 255);
            }
        }
        return CDRF_NEWFONT;
    }

    return CDRF_DODEFAULT;
}

static LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
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
        return 0;

    case WM_SIZE:
        LayoutControls(window);
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
        default:
            break;
        }
        break;

    case WM_NOTIFY:
    {
        LPNMHDR header = reinterpret_cast<LPNMHDR>(lParam);
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
        if (g_dirty)
        {
            const int answer = MessageBoxW(
                window,
                L"تغییرات ذخیره نشده‌اند.\nمی‌خواهید پیش از خروج ذخیره شوند؟",
                L"خروج از برنامه",
                MB_YESNOCANCEL | MB_ICONQUESTION | MB_DEFBUTTON1);

            if (answer == IDCANCEL)
                return 0;
            if (answer == IDYES)
            {
                if (!SaveTasks())
                {
                    MessageBoxW(
                        window,
                        L"ذخیره‌سازی ناموفق بود. برنامه بسته نمی‌شود تا اطلاعات از دست نرود.",
                        L"خطا در ذخیره‌سازی",
                        MB_OK | MB_ICONERROR);
                    return 0;
                }
            }
        }
        DestroyWindow(window);
        return 0;

    case WM_DESTROY:
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
    windowClass.hIcon = LoadIconW(NULL, IDI_APPLICATION);
    windowClass.hbrBackground = NULL;
    windowClass.lpszClassName = L"NativeTaskManagerWindow";

    if (!RegisterClassExW(&windowClass))
    {
        MessageBoxW(NULL, L"ثبت کلاس پنجره ناموفق بود.", L"خطا", MB_OK | MB_ICONERROR);
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
        return 1;
    }

    ShowWindow(window, showCommand);
    UpdateWindow(window);

    MSG message;
    while (GetMessageW(&message, NULL, 0, 0) > 0)
    {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    return static_cast<int>(message.wParam);
}
