#ifndef __LYRICS_EDITOR_DIALOG_H__
#define __LYRICS_EDITOR_DIALOG_H__

#include "../base/Window.h"

class LyricsEditorDialog : public Window {
public:
    void show(HWND hwndOwner);

private:
    static INT_PTR CALLBACK DialogProc(HWND hDialog, UINT message, WPARAM wParam, LPARAM lParam);
    INT_PTR runProc(HWND hDialog, UINT message, WPARAM wParam, LPARAM lParam);

    HWND _hStatic;
    HWND _hListView;
    HWND _hRichEdit;
    void init();
};

#endif
