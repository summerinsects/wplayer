#ifndef __LYRICS_SETTING_DIALOG_H__
#define __LYRICS_SETTING_DIALOG_H__

#include "../base/Window.h"
#include "../DrawSupport/DrawSupport.h"

class LyricsSettingDialog : public Window {
public:
    bool show(HWND hwndOwner, DrawSupport::DrawParam *param);

private:
    HWND _hStaticExample = NULL;

    static INT_PTR CALLBACK DialogProc(HWND hDialog, UINT message, WPARAM wParam, LPARAM lParam);
    INT_PTR runProc(HWND hDialog, UINT message, WPARAM wParam, LPARAM lParam);

    DrawSupport::DrawParam _drawParam;
    HWND _hCurrentList = NULL;

    void init();
    void drawSample(HDC hdc, const RECT &rc);
};

#endif
