#include <Windows.h>

#define ABS(val) if (val < 0) val *= -1;

constexpr UINT VK_C = 0x43;

void ShowLastErrorMessage()
{
    LPTSTR messageBuffer = NULL;
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, GetLastError(), LANG_USER_DEFAULT, (LPTSTR)&messageBuffer, 0, NULL);

    MessageBox(NULL, messageBuffer, L"CenterActiveWindow - Error", MB_OK | MB_ICONERROR);
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
    if (RegisterHotKey(NULL, 1, MOD_WIN | MOD_ALT | MOD_NOREPEAT, VK_C) == 0)
    {
        ShowLastErrorMessage();
        return -1;
    }

    MSG msg = {};
    BOOL bRet;
    while ((bRet = GetMessage(&msg, NULL, 0, 0)) != 0)
    {
        if (bRet == -1)
        {
            ShowLastErrorMessage();
            return -1;
        }

        if (msg.message == WM_HOTKEY)
        {
            if (!OnHotkeyPressed())
            {
                ShowLastErrorMessage();
                return -1;
            }
        }
    }

    return 0;
}
