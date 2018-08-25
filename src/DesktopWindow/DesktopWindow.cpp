#include "DesktopWindow.h"

bool DesktopWindow::init() {
    WNDCLASSW wc;

    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = &DesktopWindow::LyricsWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = s_hInstance;
    wc.hIcon = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_SIZEALL);
    wc.hbrBackground = NULL;
    wc.lpszMenuName = nullptr;
    wc.lpszClassName = L"DesktopLyricsWindow";

    if (!::RegisterClassW(&wc) && ::GetLastError() != ERROR_ALREADY_REGISTERED) {
        return false;
    }

    RECT screenRect;
    int x, y, width = 900, height = 150;
    ::SystemParametersInfoW(SPI_GETWORKAREA, 0, &screenRect, 0);
    x = (screenRect.right - screenRect.left - width) >> 1;
    y = screenRect.bottom - screenRect.top - height;

    _hSelf = ::CreateWindowExW(WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT,
        wc.lpszClassName, nullptr,
        WS_POPUP | WS_VISIBLE | WS_CLIPSIBLINGS,
        x, y, width, height, NULL, NULL, s_hInstance, this);

    wc.lpfnWndProc = &DesktopWindow::ToolWndProc;
    wc.hbrBackground = (HBRUSH)::GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = L"DesktopToolWindow";

    if (!::RegisterClassW(&wc) && ::GetLastError() != ERROR_ALREADY_REGISTERED) {
        return false;
    }

    _hWndTool = ::CreateWindowExW(WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
        wc.lpszClassName, nullptr,
        WS_POPUP | WS_CLIPSIBLINGS | WS_THICKFRAME,
        x, y, width, height, NULL, NULL, s_hInstance, this);

    ::SetLayeredWindowAttributes(_hWndTool, 0, 40, LWA_ALPHA);

    _mouseEvent.cbSize = sizeof(TRACKMOUSEEVENT);
    _mouseEvent.dwFlags = 0;
    _mouseEvent.hwndTrack = NULL;
    _mouseEvent.dwHoverTime = HOVER_DEFAULT;

    ::ShowWindow(_hSelf, SW_SHOW);
    ::ShowWindow(_hWndTool, SW_SHOW);

    return (_hSelf != NULL && _hWndTool != NULL);
}

LRESULT CALLBACK DesktopWindow::LyricsWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_CREATE) {
        ::SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>((LPCREATESTRUCTW(lParam))->lpCreateParams));
        return 0;
    }

    DesktopWindow *thiz = reinterpret_cast<DesktopWindow *>(::GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    return thiz->runLyricsProc(hwnd, message, wParam, lParam);
}

LRESULT DesktopWindow::runLyricsProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_MOUSEMOVE:
        ::ShowWindow(_hWndTool, SW_SHOWNORMAL);
        return 0;

    case WM_CLOSE:
        ::ShowWindow(hwnd, SW_HIDE);
        return 0;

    case WM_DESTROY:
        return 0;

    default:
        break;
    }

    return ::DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT CALLBACK DesktopWindow::ToolWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_CREATE) {
        ::SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>((LPCREATESTRUCTW(lParam))->lpCreateParams));
        return 0;
    }

    DesktopWindow *thiz = reinterpret_cast<DesktopWindow *>(::GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    return thiz->runToolProc(hwnd, message, wParam, lParam);
}

LRESULT DesktopWindow::runToolProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    //case WM_NCHITTEST: {
    //    ::ShowWindow(hwnd, SW_SHOWNORMAL);
    //    LRESULT ret = DefWindowProc(hwnd, message, wParam, lParam);
    //    return (ret == HTCAPTION) ? HTBORDER : ret;
    //    return ret;
    //}

    //case WM_NCCALCSIZE: {
    //    LPNCCALCSIZE_PARAMS lpNCSizeParams = (LPNCCALCSIZE_PARAMS)lParam;
    //    if (wParam) {
    //        lpNCSizeParams->rgrc[2] = lpNCSizeParams->rgrc[1];
    //        lpNCSizeParams->rgrc[1] = lpNCSizeParams->rgrc[0];
    //    }
    //    return TRUE;
    //}

    case WM_LBUTTONDOWN:
        return ::DefWindowProcW(hwnd, WM_SYSCOMMAND, (WPARAM)(SC_MOVE | HTCAPTION), 0);

    //case WM_NCACTIVATE:
    //    if (!wParam)
    //        ::ShowWindow(hwnd, SW_SHOWNORMAL);
    //    return ::DefWindowProcW(hwnd, message, wParam, lParam);
    
    //case WM_SYSCOMMAND: {
    //    LRESULT ret = ::DefWindowProcW(hwnd, message, wParam, lParam);
    //    ::ShowWindow(hwnd, SW_SHOWNORMAL);  // 避免点到边框上闪烁
    //    return ret;
    //}

    case WM_NCMOUSEMOVE:
        if (_trackedFlag == -1) {  // 取消监测客户区
            //printf("cancel track c\n");
            _mouseEvent.hwndTrack = hwnd;
            _mouseEvent.dwFlags = TME_LEAVE | TME_CANCEL;
            ::TrackMouseEvent(&_mouseEvent);
            _trackedFlag = 0;
        }
        if (_trackedFlag == 0) {  // 监测非客户区
            //printf("start track c\n");
            _mouseEvent.hwndTrack = hwnd;
            _mouseEvent.dwFlags = TME_LEAVE | TME_NONCLIENT;
            ::TrackMouseEvent(&_mouseEvent);
            ::SystemParametersInfoW(SPI_SETDRAGFULLWINDOWS, TRUE, nullptr, 0);
            _trackedFlag = 1;
        }
        return 0;

    case WM_MOUSEMOVE:
        if (_trackedFlag == 1) {  // 取消监测非客户区
            //printf("cancel track nc\n");
            _mouseEvent.hwndTrack = hwnd;
            _mouseEvent.dwFlags = TME_LEAVE | TME_NONCLIENT | TME_CANCEL;
            ::TrackMouseEvent(&_mouseEvent);
            _trackedFlag = 0;
        }
        if (_trackedFlag == 0) {  // 监测客户区
            //printf("start track c\n");
            _mouseEvent.hwndTrack = hwnd;
            _mouseEvent.dwFlags = TME_LEAVE;
            ::TrackMouseEvent(&_mouseEvent);
            ::SystemParametersInfoW(SPI_SETDRAGFULLWINDOWS, TRUE, nullptr, 0);
            _trackedFlag = -1;
        }
        return 0;

    case WM_MOUSELEAVE:  // 离开客户区
        // 监测非客户区
        //printf("start track nc\n");
        _mouseEvent.hwndTrack = hwnd;
        _mouseEvent.dwFlags = TME_LEAVE | TME_NONCLIENT;
        ::TrackMouseEvent(&_mouseEvent);
        ::SystemParametersInfoW(SPI_SETDRAGFULLWINDOWS, TRUE, nullptr, 0);
        _trackedFlag = 1;
        return 0;

    case WM_NCMOUSELEAVE:  // 离开非客户区
        //printf("end track nc\n");
        ::ShowWindow(hwnd, SW_HIDE);
        ::SystemParametersInfoW(SPI_SETDRAGFULLWINDOWS, FALSE, nullptr, 0);
        _trackedFlag = 0;
        return 0;

    case WM_MOVING:  // 移动
    case WM_SIZING: {  // 缩放
        ::ShowWindow(hwnd, SW_SHOWNORMAL);

        // 使歌词窗口随之移动/缩放
        LPCRECT pRect = (LPCRECT)lParam;
        ::MoveWindow(_hSelf, pRect->left, pRect->top, pRect->right - pRect->left, pRect->bottom - pRect->top, FALSE);
    }
    return TRUE;

    case WM_KILLFOCUS:
        ::ShowWindow(hwnd, SW_HIDE);
        return 0;

    case WM_GETMINMAXINFO: {
        // 限制最小尺寸
        LPMINMAXINFO pMinMaxInfo = (LPMINMAXINFO)lParam;

        ::ShowWindow(hwnd, SW_SHOWNORMAL);

        pMinMaxInfo->ptMinTrackSize.x = 420;
        pMinMaxInfo->ptMinTrackSize.y = 65;
        pMinMaxInfo->ptMaxTrackSize.y = 290;
    }
    return 0;

    case WM_CLOSE:
        ::ShowWindow(hwnd, SW_HIDE);
        return 0;

    case WM_DESTROY:
        return 0;

    default:
        break;
    }

    return ::DefWindowProcW(hwnd, message, wParam, lParam);
}
