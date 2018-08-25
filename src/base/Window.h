#ifndef __WIDGET_H__
#define __WIDGET_H__

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#pragma warning(disable: 4996)

class Window {
public:
    HWND getHWnd() const { return _hSelf; }

protected:
    HWND _hSelf = NULL;
    static HINSTANCE s_hInstance;

    WNDPROC _defaultProc = nullptr;
};

#endif
