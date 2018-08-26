#ifndef __COMMON_H__
#define __COMMON_H__

#include "Window.h"

class Common final : public Window {
public:
    static HWND createStatic(HWND hPerent, const POINT &pos, const SIZE &size, LPCWSTR title);
    static HWND createButton(HWND hPerent, const POINT &pos, const SIZE &size, LPCWSTR title, HMENU hMenu);
    static HWND createCheckBox(HWND hPerent, const POINT &pos, const SIZE &size, LPCWSTR title, HMENU hMenu);
};

#endif
