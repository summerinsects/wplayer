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

#include "MainWindow/MainWindow.h"

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpszCmdLine, int iCmdShow) {

#if defined _DEBUG
    ::_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

#ifdef USE_WIN32_CONSOLE
    ::AllocConsole();
    freopen("CONOUT$", "w", stdout);
    setlocale(LC_CTYPE, ".65501");  // UTF8
#endif

    MainWindow mw;
    int ret = mw.run(hInstance, iCmdShow);

#ifdef USE_WIN32_CONSOLE
    fclose(stdout);
    ::FreeConsole();
#endif

    return ret;
}
