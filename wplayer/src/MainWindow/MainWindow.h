#ifndef __MAIN_WINDOW_H__
#define __MAIN_WINDOW_H__

#include "../base/Window.h"
#include "../base/TrackBar.h"
#include "../PlayerControls/PlayListView.h"
#include "../DesktopWindow/DesktopWindow.h"
#include "../AudioSupport/MciPlayer.h"

class MainWindow : public Window {
public:
    int run(HINSTANCE hInstance, int iCmdShow);

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT runProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    void initWidgets();
    void initMenu();
    void onCommand(WPARAM wParam);
    void onInitMenuPopup(HMENU hMenu);
    bool loadFile();
    bool onPlay();
    bool pause();
    bool resume();
    bool stop();
    bool playSpecifiedFile(LPCWSTR fileName);
    void refreshPlayerControls(DWORD_PTR curTime);
    void refreshCurrentTime(LONG_PTR pos);
    void setupProgress();
    void toggleMute();
    bool onTimer();

    HMENU _hSettingMenu;
    HMENU _hPlayModeMenu;
    HMENU _hNotifyMenu;

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
    MciPlayer _player;
    bool _opened = false;
    bool _playing = false;
    bool _muting = false;
    int _volume = 0;
    DWORD_PTR _audioLength = 0;
};

#endif
