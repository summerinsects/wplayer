﻿#include "TrackBar.h"
#include <CommCtrl.h>

bool TrackBar::init(HWND hPerent, const POINT &pos, const SIZE &size) {
    _hSelf = ::CreateWindowExW(0, TRACKBAR_CLASSW, nullptr,
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | TBS_NOTICKS,
        pos.x, pos.y, size.cx, size.cy,
        hPerent, NULL, s_hInstance, nullptr);
    ::SetWindowLongPtrW(_hSelf, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

    SendMessageW(_hSelf, TBM_SETLINESIZE, 0, 0);
    SendMessageW(_hSelf, TBM_SETPAGESIZE, 0, 0);

    _defaultProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtrW(_hSelf, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&TrackBar::TrackBarProc)));

    return _hSelf != NULL;
}

LRESULT CALLBACK TrackBar::TrackBarProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    TrackBar *thiz = reinterpret_cast<TrackBar *>(::GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    return thiz->runProc(hwnd, message, wParam, lParam);
}

LRESULT TrackBar::runProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {

    if (message == WM_LBUTTONDOWN) {
        // 以下逻辑实现功能：当点击在轨道，但未在滑块上时，将滑块设置到点击处

        RECT thumbRect;
        ::SendMessageW(hwnd, TBM_GETTHUMBRECT, 0, reinterpret_cast<LPARAM>(&thumbRect));  // 获取滑块矩形

        POINT point = { LOWORD(lParam), HIWORD(lParam) };
        if (point.y >= thumbRect.top && point.y <= thumbRect.bottom
            && (point.x < thumbRect.left || point.x > thumbRect.right)) {  // y坐标在滑块矩形内且x坐标不在滑块矩形内

            RECT channelRect;
            ::SendMessageW(hwnd, TBM_GETCHANNELRECT, 0, reinterpret_cast<LPARAM>(&channelRect));  // 获取轨道矩形

            if (point.x >= channelRect.left && point.x <= channelRect.right) {  // x坐标在轨道矩形内
                LONG thumbWidth = thumbRect.right - thumbRect.left;  // 滑块矩形宽
                LONG channelWidth = channelRect.right - channelRect.left;  // 轨道矩形宽

                LONG offset = point.x - channelRect.left - (thumbWidth >> 1);  // 计算偏移

                LRESULT min = ::SendMessageW(hwnd, TBM_GETRANGEMIN, 0, 0);
                LRESULT max = ::SendMessageW(hwnd, TBM_GETRANGEMAX, 0, 0);
                LRESULT pos = (max - min) * offset / (channelWidth - thumbWidth);  // 计算位置
                ::SendMessageW(hwnd, TBM_SETPOS, static_cast<WPARAM>(TRUE), static_cast<LPARAM>(pos));

                if (_onPosChanged) {
                    _onPosChanged(this, pos);
                }
            }
        }
    }

    return ::CallWindowProcW(_defaultProc, hwnd, message, wParam, lParam);
}

void TrackBar::onHScroll(WPARAM wParam, LPARAM lParam) {
    UNREFERENCED_PARAMETER(lParam);

    switch (LOWORD(wParam)) {
    case TB_THUMBTRACK:  // 拖动
        _isTracking = true;
        if (_onTracking) {
            //_onTracking(this, HIWORD(wParam));  // 这个只能支持16位
            _onTracking(this, static_cast<LONG_PTR>(::SendMessageW(_hSelf, TBM_GETPOS, 0, 0)));
        }
        break;
    case TB_ENDTRACK:  // 拖动结束
        if (!_isTracking) {
            break;
        }
        _isTracking = false;
        if (_onPosChanged) {
            _onPosChanged(this, static_cast<LONG_PTR>(::SendMessageW(_hSelf, TBM_GETPOS, 0, 0)));
        }
        break;
    default:
        break;
    }
}

