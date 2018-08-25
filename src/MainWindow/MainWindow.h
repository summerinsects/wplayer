#ifndef __MAIN_WINDOW_H__
#define __MAIN_WINDOW_H__

#include "../base/Window.h"
#include "../base/TrackBar.h"
#include "../PlayerControls/PlayListView.h"
#include "../DesktopWindow/DesktopWindow.h"

class MainWindow : public Window {
public:
    int run(HINSTANCE hInstance, int iCmdShow);

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT runProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    void initWidgets();
    void initMenu();
    void onCommand(WPARAM wParam);
    bool loadFile();

    HWND _hCurrentTime;
    HWND _hTotalTime;
    HWND _hPlayButton;
    HWND _hPrevButton;
    HWND _hNextButton;
    HWND _hStopButton;
    HWND _hMuteButton;

    TrackBar _volumeTrack;
    TrackBar _progressTrack;
    PlayListView _listView;
    DesktopWindow _desktopWindow;
};

#endif
