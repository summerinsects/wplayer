#include "Common.h"

HWND Common::createStatic(HWND hPerent, const POINT &pos, const SIZE &size, LPCWSTR title) {
    return ::CreateWindowExW(0, L"static", title,
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
        pos.x, pos.y, size.cx, size.cy,
        hPerent, NULL, s_hInstance, nullptr);
}

HWND Common::createButton(HWND hPerent, const POINT &pos, const SIZE &size, LPCWSTR title, HMENU hMenu) {
    return ::CreateWindowExW(0, L"button", title,
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
        pos.x, pos.y, size.cx, size.cy,
        hPerent, hMenu, s_hInstance, nullptr);
}

HWND Common::createCheckBox(HWND hPerent, const POINT &pos, const SIZE &size, LPCWSTR title, HMENU hMenu) {
    return ::CreateWindowExW(0, L"button", title,
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | BS_CHECKBOX,
        pos.x, pos.y, size.cx, size.cy,
        hPerent, hMenu, s_hInstance, nullptr);
}
