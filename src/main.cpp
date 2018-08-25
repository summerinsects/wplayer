#if defined _DEBUG
#   define CRTDBG_MAP_ALLOC
#   include <crtdbg.h>
#endif

#include <windows.h>
#include <tchar.h>

#if defined _M_IX86
#   pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' \
        version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
#   pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' \
        version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#   pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' \
        version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#   pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' \
        version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

//#define USE_WIN32_CONSOLE

#ifdef USE_WIN32_CONSOLE
#   include <stdio.h>
#   include <locale.h>
#endif

#pragma comment(linker,"/subsystem:windows")
#pragma warning(disable: 4996)

static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpszCmdLine, int iCmdShow) {

#if defined _DEBUG
    ::_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

#ifdef USE_WIN32_CONSOLE
    ::AllocConsole();
    freopen("CONOUT$", "w", stdout);
    setlocale(LC_CTYPE, ".65501");  // UTF8
#endif

    WNDCLASSW wc;
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = &WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszMenuName = nullptr;
    wc.lpszClassName = L"WPlayerMainWindow";

    if (!::RegisterClassW(&wc) && ::GetLastError() != ERROR_ALREADY_REGISTERED) {
        return 0;
    }

    RECT screenRect;
    ::SystemParametersInfoW(SPI_GETWORKAREA, 0, &screenRect, 0);
    int xScreen = screenRect.right - screenRect.left;
    int yScreen = screenRect.bottom - screenRect.top;

    int width = 300, height = 400;

    HWND hwnd = ::CreateWindowExW(0, wc.lpszClassName, L"wplayer",
        WS_POPUP | WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX,
        (xScreen - width) / 2, (yScreen - height) / 2, width, height,
        NULL, NULL, hInstance, nullptr);

    ::ShowWindow(hwnd, iCmdShow);
    ::UpdateWindow(hwnd);

    MSG msg;
    while (::GetMessageW(&msg, NULL, 0, 0)) {
        ::TranslateMessage(&msg);
        ::DispatchMessageW(&msg);
    }

#ifdef USE_WIN32_CONSOLE
    fclose(stdout);
    ::FreeConsole();
#endif

    return static_cast<int>(msg.wParam);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
        return 0;

    case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = ::BeginPaint(hwnd, &ps);

            RECT rect;
            ::GetClientRect(hwnd, &rect);
            ::FillRect(hdc, &rect, (HBRUSH)(COLOR_BTNFACE + 1));
            ::EndPaint(hwnd, &ps);
        }
        return 0;

    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;

    default:
        break;
    }

    return ::DefWindowProcW(hwnd, message, wParam, lParam);
}
