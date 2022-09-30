#include <Windows.h>

#define WM_NOTIFYICON (WM_USER + 0x100)

#define EXIT_IF_ERROR(r) {\
    if (r == 0)\
    {\
        ShowLastErrorMessage();\
        return -1;\
    }\
}

#define ABS(val) if (val < 0) val *= -1;

constexpr unsigned int MENU_ITEM_ID = 1;

constexpr UINT VK_C = 0x43;

NOTIFYICONDATA* pNiData;

HMENU hNotifyIconMenu = NULL;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void ShowLastErrorMessage()
{
    LPTSTR messageBuffer = NULL;
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, GetLastError(), LANG_USER_DEFAULT, (LPTSTR)&messageBuffer, 0, NULL);

    MessageBox(NULL, messageBuffer, L"CenterActiveWindow - Error", MB_OK | MB_ICONERROR);
}

bool GetPathToExe(WCHAR buffer[MAX_PATH])
{
    DWORD len = GetModuleFileName(NULL, buffer, MAX_PATH);
    if (len == 0)
    {
        return false;
    }

    return true;
}

HWND CreateMessageWindow(HINSTANCE hInstance)
{
    const WCHAR CLASS_NAME[] = L"CenterActiveWindow Class";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    return CreateWindowEx(0, CLASS_NAME, NULL, 0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, HWND_MESSAGE, NULL, hInstance, NULL);
}

bool CreateNotifyIcon(HWND hWnd, HINSTANCE hInstance, WCHAR pathToIcon[MAX_PATH])
{
    NOTIFYICONDATA niData = {};
    niData.cbSize = sizeof(NOTIFYICONDATA);
    niData.hWnd = hWnd;
    niData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    niData.uID = 1;
    niData.uCallbackMessage = WM_NOTIFYICON;

    HICON icon = ExtractIcon(hInstance, pathToIcon, 0);
    niData.hIcon = icon;

    WCHAR tooltip[] = L"Center Active Window - Win + Alt + C";
    int count = sizeof(tooltip) / sizeof(WCHAR);
    for (int i = 0; i < count; ++i)
    {
        niData.szTip[i] = tooltip[i];
    }

    pNiData = &niData;

    if (Shell_NotifyIcon(NIM_ADD, pNiData) == FALSE)
    {
        return false;
    }

    DestroyIcon(icon);
    return true;
}

HMENU CreateNotifyIconMenu()
{
    HMENU hMenu = CreatePopupMenu();
    InsertMenu(hMenu, -1, MF_BYPOSITION | MF_STRING, MENU_ITEM_ID, L"Exit");
    return hMenu;
}

bool OnHotkeyPressed()
{
    HWND window = GetForegroundWindow();
    if (window == NULL) return true;

    RECT windowRect = {};
    if (GetWindowRect(window, (LPRECT)&windowRect) == 0) return false;

    MONITORINFO monitor = {};
    monitor.cbSize = sizeof(MONITORINFO);
    if (GetMonitorInfo(MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST), &monitor) == 0) return false;

    int xCenter = (monitor.rcWork.left + monitor.rcWork.right) / 2;
    int yCenter = (monitor.rcWork.top + monitor.rcWork.bottom) / 2;

    int wndWidth = windowRect.right - windowRect.left;
    ABS(wndWidth);
    int wndHeight = windowRect.bottom - windowRect.top;
    ABS(wndHeight);

    int newLeft = xCenter - (wndWidth / 2);
    int newTop = yCenter - (wndHeight / 2);

    if (SetWindowPos(window, HWND_TOP, newLeft, newTop, wndWidth, wndHeight, SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER) == 0) return false;

    return true;
}

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ PWSTR pCmdLine, _In_ int nCmdShow)
{
    WCHAR exePath[MAX_PATH];
    EXIT_IF_ERROR(GetPathToExe(exePath));

    HWND hWnd = CreateMessageWindow(hInstance);
    EXIT_IF_ERROR(hWnd);

    EXIT_IF_ERROR(RegisterHotKey(hWnd, 1, MOD_WIN | MOD_ALT | MOD_NOREPEAT, VK_C));

    EXIT_IF_ERROR(CreateNotifyIcon(hWnd, hInstance, exePath));

    hNotifyIconMenu = CreateNotifyIconMenu();
    EXIT_IF_ERROR(hNotifyIconMenu);

    MSG msg = {};
    BOOL bRet;
    while ((bRet = GetMessage(&msg, NULL, 0, 0)) != 0)
    {
        if (bRet == -1)
        {
            ShowLastErrorMessage();
            return -1;
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_HOTKEY:
        if (!OnHotkeyPressed())
        {
            ShowLastErrorMessage();
        }
        break;

    case WM_NOTIFYICON:
        switch (lParam)
        {
        case WM_RBUTTONUP:
        case WM_LBUTTONDBLCLK:
        case WM_CONTEXTMENU:
        case NIN_KEYSELECT:
        case NIN_SELECT:
        {
            POINT pos = {};
            GetCursorPos(&pos);

            // if the foreground window isnt set, the popup menu will not close when clicking outside of it.
            SetForegroundWindow(hwnd);

            TrackPopupMenu(hNotifyIconMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pos.x, pos.y, 0, hwnd, NULL);
        }
        break;

        default:
            break;
        }
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) == MENU_ITEM_ID)
        {
            DestroyWindow(hwnd);
        }
        break;

    case WM_DESTROY:
        DestroyMenu(hNotifyIconMenu);
        Shell_NotifyIcon(NIM_DELETE, pNiData);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
