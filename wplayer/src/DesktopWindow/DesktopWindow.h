#ifndef __DESKTOP_WINDOW_H__
#define __DESKTOP_WINDOW_H__

#include "../base/Window.h"

class DesktopWindow : public Window {
public:
    bool init();

    bool openMatchedLyrics(LPCWSTR fileName);
    void refreshLyrics(int time);

private:
    HWND _hWndTool = NULL;

    static LRESULT CALLBACK LyricsWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK ToolWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    LRESULT CALLBACK runLyricsProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT CALLBACK runToolProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    TRACKMOUSEEVENT _mouseEvent;
    int _trackedFlag = 0;  // -1 客户区 0 无 1 非客客户区
};

#endif
